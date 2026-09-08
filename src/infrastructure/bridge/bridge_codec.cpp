// @spec SPEC-TL-BRIDGE
#include <tela/bridge.hpp>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace tela {
namespace {
struct Writer {
    std::vector<unsigned char> bytes;
    void integer(std::uint64_t n, unsigned size) { for(unsigned i=0;i<size;++i) bytes.push_back(static_cast<unsigned char>(n>>(8*i))); }
    void text(const std::string& s) {
        if(s.size()>4096) throw std::invalid_argument("Bridge string too long");
        integer(s.size(),2); bytes.insert(bytes.end(),s.begin(),s.end());
    }
};
struct Reader {
    std::span<const unsigned char> bytes;
    std::size_t cursor{};
    std::uint64_t integer(unsigned size) {
        if(size>bytes.size()-cursor) throw std::invalid_argument("Truncated bridge message");
        std::uint64_t result=0; for(unsigned i=0;i<size;++i) result|=static_cast<std::uint64_t>(bytes[cursor++])<<(i*8);
        return result;
    }
    std::string text() {
        const auto size=integer(2);
        if(size>4096 || size>bytes.size()-cursor) throw std::invalid_argument("Invalid bridge string length");
        std::string result(reinterpret_cast<const char*>(bytes.data()+cursor),static_cast<size_t>(size)); cursor+=size;
        // Reject malformed UTF-8 at the process boundary, including overlong forms.
        for(size_t i=0;i<result.size();) {
            auto lead=static_cast<unsigned char>(result[i++]);
            if(lead<128) { if(!lead) throw std::invalid_argument("NUL in bridge string"); continue; }
            unsigned count=lead>=0xc2 && lead<=0xdf?1:lead>=0xe0 && lead<=0xef?2:lead>=0xf0 && lead<=0xf4?3:0;
            if(!count || count>result.size()-i) throw std::invalid_argument("Invalid bridge UTF-8");
            std::uint32_t code=lead&((1u<<(6-count))-1);
            for(unsigned j=0;j<count;++j) { auto c=static_cast<unsigned char>(result[i++]); if((c&0xc0)!=0x80) throw std::invalid_argument("Invalid UTF-8 continuation"); code=(code<<6)|(c&63); }
            if(code<(count==1?0x80u:count==2?0x800u:0x10000u) || code>0x10ffff || (code>=0xd800 && code<=0xdfff)) throw std::invalid_argument("Invalid Unicode scalar");
        }
        return result;
    }
    int coordinate() { auto n=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(integer(4))); if(n < -1000000 || n>1000000) throw std::invalid_argument("Bridge coordinate out of bounds"); return n; }
    bool boolean() { auto n=integer(1); if(n>1) throw std::invalid_argument("Invalid bridge boolean"); return n!=0; }
    unsigned count() { auto n=integer(2); if(n>256) throw std::invalid_argument("Bridge collection too large"); return static_cast<unsigned>(n); }
};
}
std::vector<unsigned char> encode_bridge(const BridgeMessage& m) {
    Writer w; w.integer(0x31574c54,4); w.integer(1,2); w.integer(static_cast<unsigned>(m.kind),2);
    w.integer(m.generation,8); w.integer(m.sequence,8); w.integer(m.revision,8); w.text(m.host); w.text(m.view);
    switch(m.kind) {
    case BridgeKind::hello: w.integer(m.host_window,8); break;
    case BridgeKind::viewport:
        for(auto n:{m.viewport.desktop_x,m.viewport.desktop_y,m.viewport.width,m.viewport.height}) w.integer(static_cast<std::uint32_t>(n),4);
        w.integer(std::bit_cast<std::uint32_t>(m.viewport.dpi_scale),4); w.integer(m.viewport.visible,1); w.integer(m.viewport.focused,1); break;
    case BridgeKind::pointer:
        w.integer(m.pointer.gesture_id,8); w.integer(static_cast<unsigned>(m.pointer.phase),1); w.integer(static_cast<unsigned>(m.pointer.button),1);
        w.integer(static_cast<std::uint32_t>(m.pointer.desktop_x),4); w.integer(static_cast<std::uint32_t>(m.pointer.desktop_y),4);
        w.integer(std::bit_cast<std::uint32_t>(m.pointer.wheel_x),4); w.integer(std::bit_cast<std::uint32_t>(m.pointer.wheel_y),4); w.integer(m.pointer.modifiers,4); break;
    case BridgeKind::selection:
        if(m.selection.size()>256) throw std::invalid_argument("Too many selected objects");
        w.integer(m.selection.size(),2); for(auto& s:m.selection)w.text(s); break;
    case BridgeKind::anchors:
        if(m.anchors.size()>256) throw std::invalid_argument("Too many anchors");
        w.integer(m.anchors.size(),2); for(auto& a:m.anchors){w.text(a.object_id);w.text(a.label);w.integer(static_cast<std::uint32_t>(a.desktop_x),4);w.integer(static_cast<std::uint32_t>(a.desktop_y),4);w.integer(a.visible,1);} break;
    case BridgeKind::heartbeat: break;
    default: throw std::invalid_argument("Unknown bridge kind");
    }
    if(w.bytes.size()>bridge_max_frame) throw std::invalid_argument("Bridge frame too large");
    return std::move(w.bytes);
}
BridgeMessage decode_bridge(std::span<const unsigned char> bytes) {
    if(bytes.size()>bridge_max_frame) throw std::invalid_argument("Bridge frame too large");
    Reader r{bytes};
    if(r.integer(4)!=0x31574c54 || r.integer(2)!=1) throw std::invalid_argument("Unsupported Tela protocol");
    BridgeMessage m; m.kind=static_cast<BridgeKind>(r.integer(2));m.generation=r.integer(8);m.sequence=r.integer(8);m.revision=r.integer(8);m.host=r.text();m.view=r.text();
    if(!m.generation || !m.sequence || m.host.empty() || m.view.empty()) throw std::invalid_argument("Missing bridge identity");
    switch(m.kind) {
    case BridgeKind::hello: m.host_window=r.integer(8); if(!m.host_window)throw std::invalid_argument("Missing host HWND");break;
    case BridgeKind::viewport:
        m.viewport={m.host,m.view,m.revision};m.viewport.desktop_x=r.coordinate();m.viewport.desktop_y=r.coordinate();m.viewport.width=r.coordinate();m.viewport.height=r.coordinate();
        m.viewport.dpi_scale=std::bit_cast<float>(static_cast<std::uint32_t>(r.integer(4)));m.viewport.visible=r.boolean();m.viewport.focused=r.boolean();break;
    case BridgeKind::pointer:
        m.pointer.sequence=m.sequence;m.pointer.viewport_revision=m.revision;m.pointer.gesture_id=r.integer(8);
        m.pointer.phase=static_cast<PointerPhase>(r.integer(1));m.pointer.button=static_cast<PointerButton>(r.integer(1));
        if(m.pointer.phase>PointerPhase::wheel || m.pointer.button>PointerButton::middle) throw std::invalid_argument("Invalid pointer enum");
        m.pointer.desktop_x=r.coordinate();m.pointer.desktop_y=r.coordinate();m.pointer.wheel_x=std::bit_cast<float>(static_cast<std::uint32_t>(r.integer(4)));m.pointer.wheel_y=std::bit_cast<float>(static_cast<std::uint32_t>(r.integer(4)));m.pointer.modifiers=static_cast<std::uint32_t>(r.integer(4));
        if(!std::isfinite(m.pointer.wheel_x)||!std::isfinite(m.pointer.wheel_y)) throw std::invalid_argument("Invalid wheel delta");break;
    case BridgeKind::selection: {auto count=r.count();for(unsigned i=0;i<count;++i)m.selection.push_back(r.text());break;}
    case BridgeKind::anchors: {auto count=r.count();for(unsigned i=0;i<count;++i){Anchor a;a.object_id=r.text();a.label=r.text();a.desktop_x=r.coordinate();a.desktop_y=r.coordinate();a.visible=r.boolean();m.anchors.push_back(std::move(a));}break;}
    case BridgeKind::heartbeat:break;
    default:throw std::invalid_argument("Unknown bridge kind");
    }
    if(r.cursor!=bytes.size())throw std::invalid_argument("Trailing bridge payload");
    return m;
}
}
