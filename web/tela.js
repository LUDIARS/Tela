// Tela web renderer (SPEC-TL-WEB). Draws WebSession frames on a canvas 2D context and sends
// pointer, keyboard, committed text (IME included), focus and size changes back.
// No dependencies. The embedder carries the strings: `send` gets each input message, and every
// message from WebSession goes to `view.receive`.
//
//   const view = Tela.attach(canvas, { send: (message) => socket.send(message) });
//   socket.onmessage = (event) => view.receive(event.data);
//   view.attachStream("remote-play", mediaStream); // shown where a video element is placed
(function (global) {
  'use strict';

  const PROTOCOL_VERSION = 1;
  const NAMED_KEYS = new Set(['Tab', 'Enter', 'Escape', 'Backspace', 'Delete', 'ArrowLeft', 'ArrowRight',
    'ArrowUp', 'ArrowDown', 'Home', 'End', 'PageUp', 'PageDown']);
  const BUTTONS = ['primary', 'middle', 'secondary'];

  const css = (c, alpha) => `rgba(${c[0]},${c[1]},${c[2]},${(alpha === undefined ? c[3] : alpha) / 255})`;
  const inner = (e) => ({
    x: e.bounds[0] + e.padding, y: e.bounds[1] + e.padding,
    width: Math.max(0, e.bounds[2] - 2 * e.padding), height: Math.max(0, e.bounds[3] - 2 * e.padding),
  });
  const modifiers = (event) => (event.shiftKey ? 1 : 0) | (event.ctrlKey ? 2 : 0) | (event.altKey ? 4 : 0) | (event.metaKey ? 8 : 0);

  function decodeImage(message) {
    const bytes = atob(message.rgba);
    const pixels = new Uint8ClampedArray(bytes.length);
    for (let i = 0; i < bytes.length; ++i) pixels[i] = bytes.charCodeAt(i);
    const surface = document.createElement('canvas');
    surface.width = message.width;
    surface.height = message.height;
    surface.getContext('2d').putImageData(new ImageData(pixels, message.width, message.height), 0, 0);
    return surface;
  }

  class TelaView {
    constructor(canvas, options) {
      if (!canvas || !options || typeof options.send !== 'function') throw new Error('Tela.attach needs a canvas and options.send');
      this.canvas = canvas;
      this.context = canvas.getContext('2d');
      this.send = (message) => options.send(JSON.stringify(message));
      this.fontFamily = options.fontFamily || 'system-ui, "Segoe UI", "Hiragino Sans", "Noto Sans JP", sans-serif';
      this.frame = null;
      this.images = new Map();
      this.streams = new Map();   // key -> MediaStream
      this.videos = new Map();    // key -> <video>
      this.dirty = false;
      this.listeners = [];
      this.createInput();
      this.listen();
      this.observeSize();
    }

    // ---- messages from WebSession ----
    receive(message) {
      const data = typeof message === 'string' ? JSON.parse(message) : message;
      if (data.type === 'image') { this.images.set(data.id, decodeImage(data)); this.invalidate(); return; }
      if (data.type !== 'frame') return;
      if (data.version !== PROTOCOL_VERSION) throw new Error(`Unsupported Tela frame version ${data.version}`);
      if (this.frame && data.revision <= this.frame.revision) return;
      this.frame = data;
      this.invalidate();
    }
    attachStream(key, stream) { this.streams.set(key, stream); this.invalidate(); }
    detachStream(key) { this.streams.delete(key); this.invalidate(); }
    dispose() {
      for (const [target, type, handler, opts] of this.listeners) target.removeEventListener(type, handler, opts);
      if (this.resizeObserver) this.resizeObserver.disconnect();
      for (const video of this.videos.values()) video.remove();
      this.input.remove();
      this.videos.clear();
    }

    // ---- drawing ----
    invalidate() {
      if (this.dirty) return;
      this.dirty = true;
      requestAnimationFrame(() => { this.dirty = false; this.draw(); });
    }
    font(e) { return `${this.frame.theme.font_size * e.text_scale}px ${this.fontFamily}`; }
    lineHeight(e) { return this.frame.theme.line_height * e.text_scale; }
    // Glyphs sit in the middle of their row; rows follow the same line height as the native surface.
    text(value, x, y, e, color) {
      const ctx = this.context;
      ctx.font = this.font(e);
      ctx.fillStyle = color;
      ctx.textBaseline = 'middle';
      ctx.fillText(value, x, y + this.lineHeight(e) / 2);
    }
    measure(value, e) { this.context.font = this.font(e); return this.context.measureText(value).width; }
    // Wraps inside the width, up to the element's reserved rows, like the native label.
    wrapped(value, e, color) {
      const area = inner(e);
      const rows = [];
      for (const paragraph of value.split('\n')) {
        let line = '';
        for (const ch of Array.from(paragraph)) {
          const next = line + ch;
          if (line && this.measure(next, e) > area.width) {
            const space = line.lastIndexOf(' ');
            if (space > 0 && ch !== ' ') { rows.push(line.slice(0, space)); line = line.slice(space + 1) + ch; }
            else { rows.push(line); line = ch === ' ' ? '' : ch; }
          } else line = next;
        }
        rows.push(line);
      }
      const lh = this.lineHeight(e);
      rows.slice(0, Math.max(1, e.lines)).forEach((row, i) => this.text(row, area.x, area.y + i * lh, e, color));
    }
    draw() {
      const frame = this.frame;
      const ctx = this.context;
      const dpr = window.devicePixelRatio || 1;
      ctx.setTransform(1, 0, 0, 1, 0, 0);
      ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
      if (!frame) return;
      ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
      this.caretRect = null;
      const shownVideos = new Set();
      for (const e of frame.elements) {
        ctx.save();
        ctx.beginPath();
        ctx.rect(e.clip[0], e.clip[1], e.clip[2], e.clip[3]);
        ctx.clip();
        this.element(e, shownVideos);
        ctx.restore();
      }
      for (const [key, video] of this.videos) if (!shownVideos.has(key)) { video.remove(); this.videos.delete(key); }
      this.placeInput();
    }
    element(e, shownVideos) {
      const t = this.frame.theme;
      const ctx = this.context;
      const fill = (r, color) => { ctx.fillStyle = color; ctx.fillRect(r[0], r[1], r[2], r[3]); };
      switch (e.kind) {
        case 'panel': fill(e.bounds, css(t.panel)); break;
        case 'button': fill(e.bounds, css(e.pressed ? t.pressed : t.button)); break;
        case 'canvas': this.shapes(e); break;
        case 'text_field': case 'text_area': this.field(e); return;
        case 'slider': this.slider(e); return;
        case 'list_item': this.listItem(e); return;
        case 'separator': fill([e.bounds[0], e.bounds[1] + e.bounds[3] / 2, e.bounds[2], 1], css(t.muted, 140)); return;
        case 'status': this.status(e); return;
        case 'scroll': this.scrollBar(e); return;
        case 'video': this.video(e, shownVideos); return;
        default: break;
      }
      if (e.label) this.wrapped(e.label, e, css(t.text));
    }
    field(e) {
      const t = this.frame.theme;
      const ctx = this.context;
      const focused = this.frame.focus === e.id;
      const area = inner(e);
      ctx.fillStyle = css(t.field);
      ctx.fillRect(e.bounds[0], e.bounds[1], e.bounds[2], e.bounds[3]);
      if (focused) { ctx.strokeStyle = css(t.accent); ctx.lineWidth = 1; ctx.strokeRect(e.bounds[0] + .5, e.bounds[1] + .5, e.bounds[2] - 1, e.bounds[3] - 1); }
      // Same rule as field_view: no soft wrap, shift keeps the caret visible, first row follows the caret.
      const placeholder = !e.value && !!e.label;
      const chars = Array.from(placeholder ? e.label : e.value);
      const caret = placeholder ? 0 : Math.min(e.caret, chars.length);
      const anchor = placeholder ? 0 : Math.min(e.anchor, chars.length);
      const lines = [];
      let start = 0;
      for (let i = 0; i <= chars.length; ++i) if (i === chars.length || chars[i] === '\n') { lines.push([start, i]); start = i + 1; }
      let caretLine = lines.findIndex(([a, b]) => caret >= a && caret <= b);
      if (caretLine < 0) caretLine = lines.length - 1;
      const rows = e.kind === 'text_area' ? Math.max(1, e.lines) : 1;
      const first = Math.max(0, caretLine + 1 - rows);
      const lh = this.lineHeight(e);
      const slice = (a, b) => chars.slice(a, b).join('');
      const caretX = this.measure(slice(lines[caretLine][0], caret), e);
      const shift = Math.max(0, caretX + 1 - area.width);
      ctx.save();
      ctx.beginPath();
      ctx.rect(area.x, area.y, area.width, area.height);
      ctx.clip();
      const low = Math.min(caret, anchor), high = Math.max(caret, anchor);
      for (let i = first; i < lines.length && i < first + rows; ++i) {
        const [a, b] = lines[i];
        const x = area.x - shift, y = area.y + (i - first) * lh;
        if (focused && low < high && Math.max(a, low) < Math.min(b, high)) {
          const x0 = this.measure(slice(a, Math.max(a, low)), e), x1 = this.measure(slice(a, Math.min(b, high)), e);
          ctx.fillStyle = css(t.accent, 110);
          ctx.fillRect(x + x0, y, x1 - x0, lh);
        }
        this.text(slice(a, b), x, y, e, css(placeholder ? t.muted : t.text));
      }
      if (focused) {
        const width = Math.max(1, t.font_size * e.text_scale / 12);
        ctx.fillStyle = css(t.text);
        this.caretRect = { x: area.x - shift + caretX, y: area.y + (caretLine - first) * lh, height: lh };
        ctx.fillRect(this.caretRect.x, this.caretRect.y, width, lh);
      }
      ctx.restore();
    }
    slider(e) {
      const t = this.frame.theme;
      const ctx = this.context;
      const area = inner(e);
      const lh = this.lineHeight(e);
      const track = Math.max(2, lh / 6);
      const y = area.y + (area.height - track) / 2;
      const knobX = area.x + area.width * e.fraction;
      ctx.fillStyle = css(t.muted, 120); ctx.fillRect(area.x, y, area.width, track);
      ctx.fillStyle = css(t.accent); ctx.fillRect(area.x, y, knobX - area.x, track);
      const knob = Math.max(6, lh / 2.5), height = Math.min(area.height, lh * .8);
      ctx.fillStyle = css(this.frame.focus === e.id || e.pressed ? t.accent : t.text);
      ctx.fillRect(knobX - knob / 2, area.y + (area.height - height) / 2, knob, height);
    }
    listItem(e) {
      const t = this.frame.theme;
      const ctx = this.context;
      if (e.pressed || e.selected) { ctx.fillStyle = css(e.pressed ? t.pressed : t.button); ctx.fillRect(...e.bounds); }
      if (e.selected) { ctx.fillStyle = css(t.accent); ctx.fillRect(e.bounds[0], e.bounds[1], 3, e.bounds[3]); }
      const area = inner(e);
      if (e.label) this.text(e.label, area.x, area.y, e, css(t.text));
      if (e.detail) this.text(e.detail, Math.max(area.x, area.x + area.width - this.measure(e.detail, e)), area.y, e, css(t.muted));
    }
    status(e) {
      const t = this.frame.theme;
      const ctx = this.context;
      const area = inner(e);
      const lh = this.lineHeight(e);
      const dot = t.font_size * e.text_scale * .6;
      ctx.fillStyle = css(t[e.tone === 'neutral' ? 'muted' : e.tone]);
      ctx.beginPath();
      ctx.ellipse(area.x + dot / 2, area.y + lh / 2, dot / 2, dot / 2, 0, 0, Math.PI * 2);
      ctx.fill();
      if (e.label) this.text(e.label, area.x + lh, area.y, e, css(t.text));
    }
    scrollBar(e) {
      const area = inner(e);
      if (e.content_height <= area.height || area.height <= 0) return;
      const lh = this.lineHeight(e);
      const thumb = Math.max(lh, area.height * area.height / e.content_height);
      const y = area.y + (area.height - thumb) * (e.scroll / (e.content_height - area.height));
      this.context.fillStyle = css(this.frame.theme.muted, 160);
      this.context.fillRect(e.bounds[0] + e.bounds[2] - 5, y, 4, thumb);
    }
    video(e, shownVideos) {
      const t = this.frame.theme;
      const ctx = this.context;
      const stream = this.streams.get(e.stream);
      if (!stream) {
        ctx.fillStyle = 'rgba(0,0,0,.78)'; ctx.fillRect(...e.bounds);
        ctx.strokeStyle = css(t.muted); ctx.lineWidth = 1;
        ctx.strokeRect(e.bounds[0] + .5, e.bounds[1] + .5, e.bounds[2] - 1, e.bounds[3] - 1);
        if (e.label) this.wrapped(e.label, e, css(t.text));
        return;
      }
      shownVideos.add(e.stream);
      let video = this.videos.get(e.stream);
      if (!video) {
        video = document.createElement('video');
        video.autoplay = true; video.muted = true; video.playsInline = true;
        Object.assign(video.style, { position: 'absolute', objectFit: 'contain', background: '#000', pointerEvents: 'none' });
        this.canvas.parentElement.appendChild(video);
        this.videos.set(e.stream, video);
      }
      if (video.srcObject !== stream) video.srcObject = stream;
      // Placed over the visible part of the element; input still goes through the canvas.
      const box = this.canvas.getBoundingClientRect(), parent = this.canvas.parentElement.getBoundingClientRect();
      Object.assign(video.style, {
        left: `${box.left - parent.left + e.clip[0]}px`, top: `${box.top - parent.top + e.clip[1]}px`,
        width: `${e.clip[2]}px`, height: `${e.clip[3]}px`,
      });
    }
    shapes(e) {
      const ctx = this.context;
      const ox = e.bounds[0], oy = e.bounds[1];
      for (const s of e.shapes) {
        ctx.save();
        ctx.translate(ox, oy);
        ctx.globalAlpha = s.opacity === undefined ? 1 : s.opacity;
        const [x, y, w, h] = s.bounds;
        if (s.kind === 'image') {
          const image = s.image && this.images.get(s.image);
          if (image) ctx.drawImage(image, x, y, w, h);
          ctx.restore();
          continue;
        }
        ctx.beginPath();
        if (s.kind === 'rectangle') {
          if (s.radius && ctx.roundRect) ctx.roundRect(x, y, w, h, Math.min(s.radius, w / 2, h / 2)); else ctx.rect(x, y, w, h);
        } else if (s.kind === 'ellipse') {
          ctx.ellipse(x + w / 2, y + h / 2, w / 2, h / 2, 0, 0, Math.PI * 2);
        } else if (s.kind === 'polyline') {
          s.points.forEach(([px, py], i) => (i ? ctx.lineTo(px, py) : ctx.moveTo(px, py)));
        }
        if (s.kind !== 'polyline') {
          if (s.gradient) {
            const g = ctx.createLinearGradient(s.gradient.start[0], s.gradient.start[1], s.gradient.end[0], s.gradient.end[1]);
            g.addColorStop(0, css(s.gradient.from)); g.addColorStop(1, css(s.gradient.to));
            ctx.fillStyle = g; ctx.fill();
          } else if (s.fill) { ctx.fillStyle = css(s.fill); ctx.fill(); }
        }
        if (s.stroke) {
          ctx.strokeStyle = css(s.stroke.color);
          ctx.lineWidth = s.stroke.width;
          ctx.lineJoin = 'round'; ctx.lineCap = 'round';
          ctx.setLineDash(s.stroke.dash ? [s.stroke.dash, s.stroke.gap] : []);
          if (s.stroke.glow) { ctx.shadowColor = css(s.stroke.glow.color); ctx.shadowBlur = s.stroke.glow.radius; }
          ctx.stroke();
        }
        ctx.restore();
      }
    }

    // ---- input ----
    on(target, type, handler, opts) { target.addEventListener(type, handler, opts); this.listeners.push([target, type, handler, opts]); }
    point(event) {
      const box = this.canvas.getBoundingClientRect();
      return { x: event.clientX - box.left, y: event.clientY - box.top };
    }
    createInput() {
      // A hidden textarea receives keys, paste and IME composition; only committed text is sent.
      const input = document.createElement('textarea');
      input.setAttribute('autocomplete', 'off');
      input.setAttribute('autocapitalize', 'off');
      input.setAttribute('spellcheck', 'false');
      input.setAttribute('aria-hidden', 'true');
      Object.assign(input.style, {
        position: 'absolute', opacity: '0', width: '1px', height: '1em', padding: '0', border: '0',
        resize: 'none', overflow: 'hidden', whiteSpace: 'pre', pointerEvents: 'none', left: '0', top: '0',
      });
      const parent = this.canvas.parentElement;
      if (getComputedStyle(parent).position === 'static') parent.style.position = 'relative';
      parent.appendChild(input);
      this.input = input;
    }
    // Keeps the IME candidate window next to the caret.
    placeInput() {
      const caret = this.frame && this.frame.focus ? this.caretRect : null;
      if (!caret) return;
      const box = this.canvas.getBoundingClientRect(), parent = this.canvas.parentElement.getBoundingClientRect();
      this.input.style.left = `${box.left - parent.left + caret.x}px`;
      this.input.style.top = `${box.top - parent.top + caret.y}px`;
      this.input.style.height = `${caret.height}px`;
    }
    commitText() {
      const text = this.input.value;
      this.input.value = '';
      if (text) this.send({ type: 'text', text });
    }
    listen() {
      const canvas = this.canvas, input = this.input;
      canvas.style.touchAction = 'none';
      const pointer = (phase) => (event) => {
        const { x, y } = this.point(event);
        const message = { type: 'pointer', phase, x, y, modifiers: modifiers(event), button: 'none' };
        if (phase === 'down' || phase === 'up') message.button = BUTTONS[event.button] || 'none';
        if (phase === 'down') { canvas.setPointerCapture(event.pointerId); event.preventDefault(); input.focus({ preventScroll: true }); }
        this.send(message);
      };
      this.on(canvas, 'pointerdown', pointer('down'));
      this.on(canvas, 'pointermove', pointer('move'));
      this.on(canvas, 'pointerup', pointer('up'));
      this.on(canvas, 'pointercancel', pointer('cancel'));
      this.on(canvas, 'wheel', (event) => {
        event.preventDefault();
        // Notches: about 100 CSS pixels or 3 lines per notch; positive turns away from the user.
        const unit = event.deltaMode === 0 ? 100 : event.deltaMode === 1 ? 3 : 1;
        const { x, y } = this.point(event);
        const clamp = (v) => Math.max(-100, Math.min(100, v));
        this.send({ type: 'pointer', phase: 'wheel', x, y, wheel_x: clamp(event.deltaX / unit), wheel_y: clamp(-event.deltaY / unit), modifiers: modifiers(event) });
      }, { passive: false });
      this.on(input, 'keydown', (event) => {
        if (event.isComposing || event.keyCode === 229) return; // the IME owns this key
        const command = event.ctrlKey || event.metaKey;
        let key = null;
        if (NAMED_KEYS.has(event.key)) key = event.key;
        else if (command && (event.key === 'a' || event.key === 'A')) key = 'a';
        if (!key) return;
        event.preventDefault();
        this.send({ type: 'key', key, pressed: true, modifiers: modifiers(event) });
      });
      this.on(input, 'input', (event) => { if (!event.isComposing) this.commitText(); });
      this.on(input, 'compositionend', () => this.commitText());
      this.on(window, 'focus', () => this.send({ type: 'focus', focused: true }));
      this.on(window, 'blur', () => this.send({ type: 'focus', focused: false }));
    }
    observeSize() {
      const report = () => {
        const box = this.canvas.getBoundingClientRect();
        const dpr = window.devicePixelRatio || 1;
        this.canvas.width = Math.max(0, Math.round(box.width * dpr));
        this.canvas.height = Math.max(0, Math.round(box.height * dpr));
        this.send({ type: 'resize', width: box.width, height: box.height, dpi: Math.min(8, Math.max(.25, dpr)) });
        this.invalidate();
      };
      this.resizeObserver = new ResizeObserver(report);
      this.resizeObserver.observe(this.canvas);
      report();
    }
  }

  global.Tela = { attach: (canvas, options) => new TelaView(canvas, options), View: TelaView, PROTOCOL_VERSION };
})(typeof window !== 'undefined' ? window : globalThis);
