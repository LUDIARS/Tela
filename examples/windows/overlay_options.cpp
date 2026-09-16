#include "overlay_options.hpp"
#include <string_view>
#include <stdexcept>
// @spec Overlay lifecycle
// @spec Spec view
// @spec Overlay placement
Options options(int argc,char** argv){
    Options result;
    for(int i=1;i<argc;++i){std::string_view arg=argv[i];
        if(arg=="--probe"){result.probe=true;continue;}
        if(i+1>=argc)throw std::invalid_argument("Missing option value");
        std::string value=argv[++i];
        if(arg=="--font")result.font=value;else if(arg=="--pipe")result.pipe=value;else if(arg=="--transitions"){result.file=value;result.transitionsGiven=true;}else if(arg=="--report")result.report=value;
        else if(arg=="--scene-overlay")result.sceneOverlay=value;
        else if(arg=="--spec-view")result.specView=value;
        // Attaching is independent of the content: the same declaration can sit on the Unity
        // host through the bridge, or on the separate probe target window for a standalone run.
        else if(arg=="--attach"){if(value=="probe-target")result.attachProbeTarget=true;else if(value!="unity")throw std::invalid_argument("--attach must be unity or probe-target");}
        else if(arg=="--place")result.placement=tela::placement_from_name(value);
        // The host supplies the typeface; the size is chosen per run because the same file is
        // read on hosts of very different sizes.
        else if(arg=="--font-size"){result.fontSize=std::stoi(value);if(result.fontSize<8||result.fontSize>96)throw std::invalid_argument("font-size must be 8..96");}
        else if(arg=="--seconds"){result.seconds=std::stoi(value);if(result.seconds<1||result.seconds>3600)throw std::invalid_argument("seconds must be 1..3600");}
        else throw std::invalid_argument("Unknown option");
    }
    if(result.font.empty())throw std::invalid_argument("--font <TrueType file> is required");
    // One content source per session: a read-only view never silently edits a transition file.
    if(!result.sceneOverlay.empty()&&(result.probe||result.transitionsGiven))throw std::invalid_argument("--scene-overlay cannot be combined with --probe or --transitions");
    if(!result.specView.empty()&&(result.probe||result.transitionsGiven||!result.sceneOverlay.empty()))throw std::invalid_argument("--spec-view cannot be combined with --probe, --transitions or --scene-overlay");
    // --probe already owns the probe target window, and the transition editor needs the Unity anchors.
    if(result.attachProbeTarget&&(result.probe||(result.specView.empty()&&result.sceneOverlay.empty())))throw std::invalid_argument("--attach probe-target requires --spec-view or --scene-overlay");
    // Only the attached adapter owns its viewport; through the bridge Unity decides the geometry.
    if(result.placement!=tela::Placement::inside&&!result.attachProbeTarget)throw std::invalid_argument("--place other than inside requires --attach probe-target");
    return result;
}
