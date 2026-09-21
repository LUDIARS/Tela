// @implements SPEC-TL-RUNTIME
// @spec Runtime
#include <tela/desktop_placement.hpp>
#include <tela/runtime.hpp>
#include <tela/transitions.hpp>
#ifdef TELA_CONSUMER_WINDOWS
#include <tela/windows_desktop_overlay.hpp>
#include <string_view>
#endif
int main(int argc, char** argv){
    tela::Runtime runtime;tela::Document document;document.button("consumer","Orbis / Iter");runtime.document(std::move(document));
    tela::DesktopPlacement placement;placement.width=320;placement.height=120;
    if(tela::place_on_desktop(placement,{{0,0,1920,1040,1.0f,true}}).width!=320)return 1;
#ifdef TELA_CONSUMER_WINDOWS
    // Linking is the packaging check; a window only opens when asked for with a font.
    if(argc>2&&std::string_view(argv[1])=="--desktop-overlay"){
        tela::PictorSurface renderer(argv[2]);
        tela::WindowsDesktopOverlay overlay(runtime,renderer,{placement});
        overlay.synchronize();
    }
#else
    (void)argc;(void)argv;
#endif
    return 0;
}
