// @spec SPEC-TL-WEB
#include <tela/web_session.hpp>
#include "../infrastructure/web/frame_encoder.hpp"
#include "../infrastructure/web/image_registry.hpp"
#include "../infrastructure/web/input_decoder.hpp"
#include <cmath>
#include <stdexcept>
#include <variant>

// @implements SPEC-TL-WEB
// @spec Web rendering
namespace tela {
struct WebSession::Impl {
    Runtime& runtime;
    WebLimits limits;
    web::ImageRegistry images;
    std::string error, last_body;
    std::uint64_t revision{}, viewport_revision{}, pointer_sequence{}, gesture{}, key_sequence{};
    bool focused{true}, sized{};

    Impl(Runtime& r, WebLimits l) : runtime(r), limits(l), images(l.image_bytes) {}

    bool fail(std::string reason) { error = std::move(reason); return false; }
    bool apply_viewport(Viewport next) {
        next.revision = ++viewport_revision;
        try { runtime.viewport(std::move(next)); }
        catch (const std::invalid_argument& e) { return fail(e.what()); }
        return true;
    }
    bool operator()(const web::WebResize& r) {
        Viewport next = runtime.viewport();
        next.host_id = "web"; next.view_id = "page";
        next.desktop_x = next.desktop_y = 0;
        next.width = static_cast<int>(std::lround(r.width * r.dpi));
        next.height = static_cast<int>(std::lround(r.height * r.dpi));
        next.dpi_scale = r.dpi;
        next.visible = next.width > 0 && next.height > 0;
        next.focused = focused;
        if (!apply_viewport(std::move(next))) return false;
        sized = true;
        return true;
    }
    bool operator()(const web::WebFocus& f) {
        focused = f.focused;
        if (!sized) return true;
        auto next = runtime.viewport();
        next.focused = focused;
        return apply_viewport(std::move(next));
    }
    bool operator()(const web::WebPointer& p) {
        if (!sized) return fail("pointer before resize");
        const auto& view = runtime.viewport();
        HostPointerEvent event;
        event.sequence = ++pointer_sequence;
        event.viewport_revision = view.revision;
        if (p.phase == PointerPhase::down) ++gesture;
        event.gesture_id = gesture;
        event.phase = p.phase;
        event.button = p.button;
        event.desktop_x = static_cast<int>(std::lround(p.x * view.dpi_scale));
        event.desktop_y = static_cast<int>(std::lround(p.y * view.dpi_scale));
        event.wheel_x = p.wheel_x;
        event.wheel_y = p.wheel_y;
        event.modifiers = p.modifiers;
        // The page owns the whole canvas, as a native view owns its client area.
        runtime.pointer(event, InputSource::native);
        return true;
    }
    bool operator()(const web::WebKey& k) {
        if (!sized) return fail("key before resize");
        runtime.key({++key_sequence, k.key, k.pressed, k.modifiers});
        return true;
    }
    bool operator()(const web::WebText& t) {
        if (!sized) return fail("text before resize");
        runtime.text({++key_sequence, t.text});
        return true;
    }
};

WebSession::WebSession(Runtime& runtime, WebLimits limits) : impl_(std::make_unique<Impl>(runtime, limits)) {
    if (!limits.frame_bytes || !limits.elements || !limits.input_bytes || !limits.text_bytes)
        throw std::invalid_argument("Tela web limits must be positive");
}
WebSession::~WebSession() = default;
bool WebSession::receive(std::string_view message) {
    impl_->error.clear();
    auto decoded = web::decode_input(message, impl_->limits);
    if (!decoded.input) return impl_->fail(std::move(decoded.error));
    return std::visit(*impl_, *decoded.input);
}
const std::string& WebSession::last_error() const noexcept { return impl_->error; }
std::vector<std::string> WebSession::poll() {
    auto& runtime = impl_->runtime;
    if (!impl_->sized || !runtime.needs_frame()) return {};
    impl_->images.prune();
    auto frame = web::encode_frame(runtime, impl_->images, impl_->limits);
    auto messages = impl_->images.take_pending();
    runtime.frame_presented();
    // Only what the page would draw differently is sent; a relayout to the same result is not.
    if (frame.body == impl_->last_body) return messages;
    impl_->last_body = frame.body;
    messages.push_back(web::frame_header(++impl_->revision) + frame.body);
    return messages;
}
void WebSession::disconnect() {
    impl_->runtime.disconnect();
    impl_->images.reset();
    impl_->last_body.clear();
    impl_->sized = false;
    impl_->focused = true;
}
std::uint64_t WebSession::revision() const noexcept { return impl_->revision; }
}
