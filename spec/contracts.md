---
x-anatomia:
  kind: specification
  id: tela-core-contract
---
# Tela core contracts

## Declaration [id: SPEC-TL-DECLARATION]

IDs are nonempty and unique across one document, independent of visible text.
Panel children reference their parent ID. Failed panel declarations roll back
their entire subtree and restore the parent scope. Static annotations default to
passthrough; actionable buttons default to exclusive input. Declaration does not
execute application actions or start rendering.

## Runtime [id: SPEC-TL-RUNTIME]

The runtime retains press/activation state by stable element ID and binds the
latest declaration's action. Removing an active action cancels its gesture.
Panel bounds clip both paint and input. Themes and geometry invalidate layout;
identical declarations may update callbacks without submitting another frame.
Logical layout pixels are converted to physical pixels once by the host adapter.

## Input ownership [id: SPEC-TL-INPUT]

Passthrough and shared regions never take physical input through native hit
windows. Exclusive input uses native capture. The owner remains fixed from down
through up/cancel; release outside the original button does not execute it.
Shared input arrives as a host observation and is not injected into the OS.
Duplicate sequence, foreign source and stale-revision events cannot activate an
action twice. Disconnect, target loss and focus loss cancel active gestures.

## Overlay lifecycle [id: SPEC-TL-OVERLAY]

The overlay follows the physical viewport and renders only when invalidated and
visible. A hidden, minimized, destroyed or unrelated-background target suppresses
presentation. Restore invalidates the stored surface. Native windows, capture,
GDI surfaces and IPC handles are released by their owners. The current backend
is explicit Pictor CPU bitmap composition, not transparent Vulkan presentation.

The optional MacOS adapter follows the coordinate conversion and native ownership
rules in [SPEC-TL-MACOS](macos-composition.md). macOS global points are scaled with
the target display's backing scale for each viewport revision; anchor producers
must use that same basis. Native Mac acceptance remains required.

## Unity bridge [id: SPEC-TL-BRIDGE]

The wire protocol carries protocol version, host/view identity, monotonic
sequence and viewport revision with every event. Screen coordinates use physical
desktop pixels; negative desktop origins are valid. Per-view selection uses stable
Unity object IDs. Drop stale geometry revisions and duplicate input sequences.
Scope streams to a connection generation so reconnects cannot revive old gestures.
Connection loss cancels captures and hides the overlay. Use authenticated local IPC,
bounded queues and coalesce moves without dropping down/up/cancel ordering.

The Unity sample defers its initial hello until the selected Scene's GUI callback
runs with that Scene focused and a Unity-owned foreground root window. Clicking
Connect in a floating bridge window is not sufficient evidence of Scene ownership.
A changed root requires explicit reconnect (new transport generation). The sample
conservatively hides when the selected Scene loses focus, including Inspector or
another Unity tab, and repaints once when focus returns. It does not periodically
force Scene repaints to infer visibility. A bounded one-second heartbeat remains
necessary for the native three-second transport timeout; suspension does not mean
all Editor callbacks or transport maintenance stop. The pipe worker owns its wait
event and disposes it in finally, including eventual exit after a bounded join.

The C++ structs are in-process value types, not a packed ABI or wire format.
Serialization, current-user authentication, bounds checks and version rejection
are implemented by the optional Windows pipe adapter; see `bridge-protocol.md`.

## Transition authoring [id: SPEC-TL-TRANSITIONS]

Transition IDs, source, destination and object IDs must be nonempty. Each field
is a bounded single-line UTF-8 string; condition may be empty (unconditional).
Persistence uses `TELA_TRANSITIONS 1` followed by five space-separated,
double-quoted fields (ID, source, destination, condition, object ID), with quote
and backslash escaped by backslash, LF lines and no BOM. It is single-writer
application storage, not a packed C++ ABI or a Pf database substitute.
Invalid loads leave the in-memory model unchanged. Save writes a sibling pending
file before replacing the destination. A leftover pending file is reported and
requires recovery; it is not silently overwritten. Conditions are stored as
specification text and are never evaluated as code.

## Scene overlay [id: SPEC-TL-SCENE-OVERLAY]

Pf exports one frame of a scene and the scenes layered over it as
`TELA_SCENE_OVERLAY 1`: LF lines, no BOM, then `frame "name" width height`,
`scene "id" "name" visible` (0 or 1, bottom to top) and
`element "scene id" "id" "kind" "label" x y width height` in the exported frame's
coordinates. Fields use the transition file's quote and backslash escaping. At most
32 scenes, 1024 elements and 4 MiB; sizes are positive and coordinates finite within
±100000. Unknown records, a missing or repeated frame, duplicate IDs, elements of
unknown scenes and trailing data reject the whole file. Tela only reads the file;
Pf remains the source of truth.

The frame is fitted into the logical viewport, keeping its aspect ratio and centered.
Visible scenes declare outlined elements and labels with passthrough input. Every
scene keeps an exclusive toggle button. A toggle changes session state only and is
never written back. Unchanged declaration inputs are not redeclared, and a hidden
host declares nothing.

## Spec view [id: SPEC-TL-SPEC-VIEW]

### Readable fitted cards (2026-09-17)

UX-TL-W2/W5: the user must keep the selected specification and its title visible while
resizing the view. The spec-view application owns the fitted picture and toggle state;
the composition domain owns text metrics, and the Pictor adapter only rasterizes them.
Controls occupy a separate left-hand strip, never the exported picture. Its width is
at most 280 logical pixels or 35% of the viewport. When all 32 groups cannot fit at
normal size, the strip's rows and text shrink together to remain reachable. The rest
of the viewport fits the unchanged Pf picture uniformly, with an upper scale of 64.
This does not change Pf's relative card layout. Small host windows can make text too
small to read; enlarging the window restores it without changing visibility state.

`Layout::text_scale` is a finite multiplier in (0,64], default 1. Text rasterization
and automatic row height apply it once, before host DPI. Explicit heights still win.
Spec cards scale padding, offsets and text together. Title height is bounded by the
card; titles wrap into up to three complete theme lines, with remaining text clipped
at the box boundary (no ellipsis). `--font-size` sets the theme at picture scale 1.
Theme changes invalidate the spec declaration. These rules are identical for native
views and Unity overlays. The source Pf data and other declarations are unchanged.

Pf exports its specification visualization as `TELA_SPEC_VIEW 1`: LF lines, no BOM, then
`view "project" "version" width height`, `group "id" "name" visible` (0 or 1, in the axis'
fixed order) and `card "group id" "code" "title" "status" version x y width height` in the
exported view's coordinates. Fields use the transition file's quote and backslash escaping.
At most 32 groups, 256 cards and 4 MiB; sizes are positive, coordinates finite within
±100000 and the card version a count within 0..1000000. Group IDs and card codes are
nonempty and at most 256 bytes because they become element ID segments; other fields are
at most 4096 bytes. Unknown records, a missing or repeated view header, an empty group
list, duplicate IDs, cards of unknown groups and trailing data reject the whole file.
Tela only reads the file; Pf remains the source of truth.

The view is fitted into the logical viewport, keeping its aspect ratio and centered, when
that fit is scale 1 or more. Otherwise it is paged (below). Visible groups declare outlined cards with a code line (`code status vN`) and a title line,
both with passthrough input. The card fill pre-blends Pf's dark canvas with the group color
at the same ratio Pf uses, so the near-white text reads over a light host as well as a dark
one; the outline keeps the group color. Every group keeps an exclusive toggle button. A toggle
changes session state only and is never written back. Unchanged declaration inputs are not
redeclared, and a hidden host declares nothing. The group palette matches Pf's
`SPEC_VIEW_PALETTE`, so the Pf screen and the Tela overlay draw the same picture.

### Paged tall views (2026-10-25)

UX-TL-W9: a view that would fit only below scale 1 is not shrunk until its text is
unreadable. It fits the picture width, at most scale 1, starts at the viewport top and is
read page by page; there is no horizontal paging because Pf's layout has a fixed width.
The scroll position is session state of the spec-view application, in exported
coordinates, and is always 0 or a card's top. "Next page" moves to the first card below
the current position that is not shown whole; "Prev page" moves to the earliest card top
(or 0) within one page above, so every card is shown whole on some page; "First" returns
to 0. At the last page and at the top they do nothing. A page is the viewport height
divided by the scale (`spec_view_page`), taken from the declaration the user pressed on.
A paged view declares only whole cards (a card taller than a page shows from its top),
its card canvas covers the viewport height only, and the strip adds a `Cards a-b / n`
(`Cards a / n` when the page holds one card)
line (visible groups' cards in Pf order) and exclusive Prev page / Next page / First
buttons; the strip's shrink-to-fit counts these four rows. Group toggles do not move the
position. A host large enough to fit the whole view at scale 1 or more ignores the
position and shows no page controls. Card input stays passthrough; wheel scrolling is not
provided, because the runtime has no wheel routing and an exclusive card area would take
the host's input inside Unity.

## Overlay placement [id: SPEC-TL-PLACEMENT]

An attached session chooses where its viewport sits with `--place`
(`inside`, `left`, `right`, `above`, `below`; `inside` is the default). `inside` uses the
target's client area, as before. The other placements build the viewport from the content's
own exported size and put it beside the target window, so a view larger than the host is
read at 1:1 instead of being shrunk. A placement with no room in the monitor work area flips
to the opposite side; when neither side fits the viewport is clamped into the work area, and
a view larger than the work area is reduced to it. A spec view's own size includes its
control strip (`spec_view_controls_width`), so its cards are drawn at 1:1 beside the
target; a spec view taller than the work area is then paged rather than shrunk
(SPEC-TL-SPEC-VIEW). A degenerate size or work area falls back
to the host rect. Placements other than `inside` require `--attach probe-target`, because
through the Unity bridge the host decides the geometry. Target visibility, minimization and
loss are handled exactly as for `inside`.

`--font-size <8..96>` sets the theme's text size for the run and scales the line height by
the theme's existing ratio; the typeface stays the host's `--font` file. The size is a
per-run choice because one exported file is read on hosts of very different sizes.

## View host [id: SPEC-TL-VIEW-HOST]

`tela_view --font <ttf> --spec-view <file>` draws a declaration in Tela's own top-level
window instead of overlaying a host, for the case where the target application cannot be
started. Without `--width` / `--height` the window takes the content's exported size, so it
is read at 1:1. `--fullscreen` starts borderless on the window's monitor, F11 toggles it and
Esc closes. `--font-size 8..96` sets the theme text size and scales the line height by the
theme's ratio. A run without a font or without a content source fails instead of showing an
empty window.

The view owns its whole surface and all of its input, so the overlay's passthrough and
exclusive regions do not apply and declared buttons are clickable directly. The viewport is
the client area with the window's DPI, and is only rebuilt when size, DPI, visibility or
focus changed. Unchanged declarations present no frame, a hidden or minimized window draws
nothing, and closing releases presentation and input ownership exactly as a lost host does.

`Layout::lines` is the number of text rows a box reserves (1 by default, at most 64). The
renderer wraps inside the box width, so a single row silently drops everything past the
first break; a caller that wants wrapped text declares the rows it needs.

## Desktop overlay [id: SPEC-TL-DESKTOP-OVERLAY]

A desktop surface presents a declaration on the desktop without a target window. The contract is
shared by `WindowsDesktopOverlay` and `MacOSDesktopOverlay`, which take the same
`DesktopOverlayOptions`. The surface is topmost and never activated, and it is absent from the
OS's window switchers: on Windows unowned tool windows (neither taskbar nor Alt+Tab); on macOS
nonactivating panels above floating windows, on every Space, left out of Mission Control's window
spread, the window cycle and the Window menu (the Dock and Cmd+Tab follow the caller's activation
policy). It shows and takes input through the same composition as the attached overlay of its OS:
one passthrough visual window plus input windows for the exclusive regions, clipped to those
regions' fragments. Everything else passes input through to the windows below. The renderer's
premultiplied bitmap is presented as is: translucency comes from the declaration's own colours
(for example a rounded canvas rectangle with an alpha fill), never from a surface-wide setting.

Placement is the caller's. A `DesktopPlacement` gives a logical size and either a corner of the
primary monitor's work area with logical margins, or an absolute top-left in desktop coordinates.
On Windows those are physical pixels and logical values are converted with the DPI of the monitor
the surface lands on (Per-Monitor V2, independent of the process default). macOS has no desktop-wide
pixel grid, so there the work areas are the displays' `visibleFrame`s in global points measured down
from the primary display's top edge, logical values are points, and the surface is drawn at the
`backingScaleFactor` of the display holding it (`place_on_point_desktop`); the rules below apply
with a DPI scale of 1 per point. An absolute surface belongs to the monitor whose work area
holds the largest share of it when sized with that monitor's DPI; equal shares prefer the primary,
then the order the monitors were listed in. The surface is clamped into that work area. When no
monitor holds any of it, it returns to the same corner of the primary monitor with the same margins
and the frame is marked recovered. A surface larger than the work area is reduced to it. Monitors
with no area or a DPI scale outside 0.25..8 are ignored. A size that is not positive and at most
16384 logical pixels, a margin that is not finite and non-negative, or no usable monitor is rejected
with `std::invalid_argument`.

The element named as the grip moves the surface: dragging its exclusive region moves every window
together without presenting again, and its pointer input never reaches the runtime. On release the
drop is clamped as above, and `placement_at` reports an absolute placement at the settled spot, with
the corner of its monitor that the surface centre is nearest to and the logical distance to it.
Passing that placement back restores the spot; `placement_at` of the reported spot is the reported
placement. The report is delivered from `synchronize()`, never inside a window message; storing it
is the caller's job. A drag whose release the grip never received ends once the primary button is up.

`synchronize()` rereads monitors and DPI every call, rebuilds the viewport only when the frame
changed, and presents only when the declaration changed or the surface was hidden. While no monitor
can be read the surface is hidden. The viewport reports the surface as focused while shown: a surface
that is never activated has no host focus to lose. `hovered()` names the exclusive region under the
pointer. Destruction releases every window, capture and hit region, and the runtime loses the
surface as it loses a host (`disconnect`).

`exclude_from_capture` (default false) keeps every window of the surface, visual and input alike,
out of screen capture, recording and sharing: `SetWindowDisplayAffinity(WDA_EXCLUDEFROMCAPTURE)` on
Windows and `NSWindowSharingNone` on macOS. When Windows cannot exclude a window (before Windows 10
2004) the window is destroyed and `std::runtime_error` is thrown to the caller; the surface never
falls back to `WDA_MONITOR`, which would show a black box in the capture instead of nothing.

## List view [id: SPEC-TL-LIST-VIEW]

Another program writes a list as `TELA_LIST_VIEW 1`: UTF-8 without a BOM, LF lines, the first line
exactly `TELA_LIST_VIEW 1`, then one `title\t<text>`, one `status\t<text>` and 0..256
`row\t<mark>\t<column>...` records with 1..8 columns. Fields are separated by a single tab; the
mark is `normal`, `alert` or `muted`. Values are at most 4096 bytes of canonical UTF-8 without
control characters (CR included), so a writer replaces tabs and line breaks before writing. Blank
lines are skipped and a final LF ends the last line. A missing or repeated title or status, an
unknown record or mark, a row without a column or above 8 columns, more than 256 rows, a file
above 4 MiB and any invalid value reject the whole file; no partial list is used. Tela only reads
the file and does not know what the rows mean; the writer remains the source of truth.

`list_view_document` declares, for a logical frame width and a number of visible rows, a
translucent rounded backdrop (the declaration's own colour), the title, the status line and up to
that many rows, each on one line with its columns joined by three spaces. An alert row draws a red
band behind it and starts with `! `; a muted row starts with `? `, so the marks read without colour.
A row's text is cut at 8192 bytes on a character boundary to stay inside the declaration's label
limit. Rows beyond the frame are not declared and a final `+N more` line counts them. The frame's
height is the header, the visible rows and one row for that line, at the theme's line height. The
grip element named by the frame is the only exclusive element; everything else passes input through.

`tela_list_overlay --font <ttf> --file <list>` floats the list on a desktop overlay
(SPEC-TL-DESKTOP-OVERLAY) and follows the file: at most once a second it compares the file's write
time and size, and reads it again only when they changed. A missing file shows
`Waiting for the list file`; a file that cannot be used shows `List file is not usable: <reason>`
with no rows. The previous rows are never left on screen as if they were current. Reasons that are
not UTF-8 (local code page messages) are replaced by a fixed text. Options: `--corner`
(default top-right), `--width 240..2000` (default 520), `--rows 1..64` (default 12),
`--font-size 8..96`, `--seconds 1..86400` (default: until the process is stopped) and
`--exclude-from-capture`. A run without a font or a file fails instead of showing an empty list.
The placement reported after a drag is printed, not stored.

## Graph view [id: SPEC-TL-GRAPH]

Pf exports its domain relation diagram as `TELA_GRAPH 1`: LF lines, no BOM, then
`graph "project" "title" width height`, `group "id" "name" visible r g b` (0 or 1 and three
0..255 channels, in the diagram's column order), `node "group id" "id" "label" x y width height`
and, for each relation, `edge "id" "from node" "to node" dashed` followed by its
`point "edge id" x y` records in route order. Fields use the transition file's quote and
backslash escaping. At most 8 groups, 128 nodes, 256 edges, 4096 route points and 4 MiB;
sizes are positive and coordinates finite within ±100000. IDs are nonempty and at most 256
bytes because they become element ID segments. Unknown records, a missing or repeated
header, an empty group or node list, duplicate IDs, a node in an unknown group, an edge with
an unknown end, a point before or outside its edge, an edge with fewer than two points and
trailing data reject the whole file. Tela only reads the file; Pf remains the source of truth.

Tela performs no layout: node positions and edge routes come from Pf and are drawn as given,
never re-curved, so the Pf screen and the Tela drawing keep the same shape. The graph is
fitted into the logical viewport, keeping its aspect ratio and centered. Visible groups draw
their nodes outlined in the group colour, filled with `tint` of that colour, and
labelled over the rows the box allows. An edge is drawn only while both of its ends are in
visible groups, solid for membership and dashed for a parent link. Every group keeps an
Every relation is directed, so its end carries an arrowhead taken from the route's last
segment and drawn solid, which keeps the direction readable on a dashed relation too. A route
whose last segment has no length draws no arrowhead. Each relation therefore costs two shapes,
so a canvas holds at most 100 of them.
exclusive toggle; a toggle changes session state only and is never written back. Unchanged
declaration inputs are not redeclared, and a hidden host declares nothing.

`tint(accent)` is the shared blend of Pf's dark canvas with an accent colour, used
wherever Tela draws Pf content without a canvas of its own.

## Controls [id: SPEC-TL-CONTROLS]

Declarations add `text_field` (single line, optional secret and placeholder), `text_area`
(rows = `layout.lines`, lines break only at `\n`), `slider` (fraction clamped to [0,1]),
`list_item` (label, right-aligned detail, selected mark, action), `separator`, `status`
(neutral / good / warning / bad mark) and `scroll` (clips its children and offsets them
vertically; zero height fills the parent's remaining height). Inputs and scrolls default to
exclusive; separators and statuses are passthrough. Values are UTF-8 of at most 16384 bytes;
a text field value has no newline; a non-finite slider fraction is rejected.

Values belong to the application: edits call `on_change(value)`, Enter in a text field calls
`on_submit`, the slider calls `on_set(fraction)`, and the runtime never writes a value itself.
An application redeclares from inside the callback (or before the next input); otherwise the
next edit starts again from the declared value. The runtime keeps only caret, anchor and
scroll offset by element ID, clamps them to the redeclared value / content, and keeps them
across redeclarations. Focus is one control ID held by the runtime: a primary press on a text
field, text area or slider focuses it, a press anywhere else blurs, Tab / Shift+Tab walk the
focusable controls in placed order, Escape blurs, and a control removed by redeclaration loses
focus. Focusing a control inside a scroll scrolls it into view. Disconnect drops focus.

`HostKeyEvent` carries a key, pressed and modifier bits; `HostTextEvent` carries committed
text only (typed characters or an IME's confirmed string). Key and text events share one
increasing sequence; a stale sequence, a hidden or unfocused viewport, more than 4096 bytes or
invalid UTF-8 is ignored. Committed text loses control characters (a text area keeps line
breaks as `\n`). An insertion is cut at a character boundary so the value stays within 16384
bytes. A pointer `wheel` scrolls the innermost scroll under the pointer by three line heights
per notch (positive `wheel_y` moves toward the start), passing to an outer scroll at a limit.
A slider follows its gesture owner's drag even off the track.

What a control shows is decided from the placed result alone (`control_visual`: rectangles,
ellipses and unwrapped text lines in logical pixels and painting order); a backend only draws
those items. Fields follow `field_view`: no soft wrap, a horizontal shift that keeps the
caret visible and a first row that keeps the caret's line visible. Secret fields show one
`•` per character and never expose their bytes to drawing. Hosts set
`Runtime::text_measure` to their font's advances so a click lands the caret where it is drawn.

## Web rendering [id: SPEC-TL-WEB]

`Tela::Web` serves one Runtime to one browser page through `WebSession`, which owns no
transport: the embedder carries strings and decides who may connect. Input messages are JSON
objects of at most 16384 bytes (depth 4, 64 values, strings at most 4096 bytes, no duplicate
keys, strict UTF-8): `pointer` (phase move/down/up/cancel/wheel, button, CSS-pixel x/y within
±65536, wheel within ±100 notches, modifier bits 0..15), `key` (DOM key names Tab, Enter,
Escape, Backspace, Delete, arrows, Home, End, PageUp, PageDown, and `a`), `text`, `resize`
(CSS width/height 0..16384, device ratio 0.25..8) and `focus`. Malformed, over-limit or
pre-resize input is refused with a reason and reaches nothing. Sequences and gesture IDs are
assigned by the session.

`poll` returns image messages the page has not seen (`image`, straight RGBA base64, sent
once per image object, none above 1 MiB of pixels) and then a `frame` message only when the
encoded result differs from the last one sent. A frame carries `version` 1, an increasing
`revision`, CSS size and device ratio, the theme, the focused ID and the placed elements in
painting order with kind, bounds, clip, padding, rows, text scale, input policy, label,
pressed state and kind data (field value, caret and anchor as code point indices, slider
fraction, list detail and selection, status tone, scroll offset and content height, video
stream key, canvas shapes). Elements with an empty clip are omitted. At most 4096 elements
and 2 MiB per frame; what does not fit is left out and `truncated` is true. Secret values
leave as bullets only. Disconnect cancels input and forgets what was sent.

`web/tela.js` (no dependencies) draws frames on a canvas 2D context with browser fonts using
the same control rules, overlays an attached `MediaStream` on `video` elements (native hosts
draw a frame and the label), and sends pointer, wheel, keys, committed text (IME composition
stays in the page) and size changes. Browser fonts differ from the native font, so glyph
positions match only in layout, not per pixel.

## GPU rendering [id: SPEC-TL-GPU]

`Tela::Gpu` draws one Runtime's placed result with Pictor's Vulkan `UIRenderer` inside a render
pass the host owns (`GpuTarget`: Pictor `VulkanContext`, render pass, its color format and the
directory of Pictor's compiled `ui_widget` shaders). The host keeps the window, swapchain and
frame loop; Tela holds no window. `plan_gpu_frame` (`Tela::GpuFrame`, no GPU or Pictor needed)
turns the placed result into physical-pixel steps in painting order: quads (solid, optionally
rounded, with an integer scissor) and layer draws. Panels, buttons, filled rectangles of
`control_visual`, circle marks and plain canvas fills (no stroke, glow, gradient or opacity) are
quads; text, other ellipses and canvas shapes are layer items. Clips admit the pixels whose
top-left corner lies inside the scaled logical clip and fills cover every pixel they touch, as on
the CPU raster; elements with an empty clip produce nothing.

Layer items are rasterized on the CPU with the same Pictor TrueType text (one unwrapped line per
`control_visual` text, wrapped labels inside the padding) and the same shape raster as the CPU
surface, then uploaded as straight-alpha RGBA textures of at least the viewport size and drawn
full size from the top-left. Painting order is exact: a quad that would cover any item already in
the open layer first closes that layer (its draw step precedes the quad); when no layer is left
(default 3, at most 8) the quad is rasterized into the last layer instead. Only layers whose items
or frame size changed are rasterized and uploaded again; an upload waits for its copy. Layer
textures grow to the viewport by recreating the UIRenderer, whose texture slots are never
released. A `*_SRGB` target gets linearized quad colors and sRGB layer textures. Secret field
bytes never reach the plan. `prepare` runs outside the render pass, `record` inside it; the host
draws only when `Runtime::needs_frame()` or its swapchain requires, then calls `frame_presented()`.