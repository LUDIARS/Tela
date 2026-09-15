#include "overlay_options.hpp"
#include <string_view>
#include <stdexcept>
// @spec Overlay lifecycle
// @spec Spec view
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
        else if(arg=="--seconds"){result.seconds=std::stoi(value);if(result.seconds<1||result.seconds>3600)throw std::invalid_argument("seconds must be 1..3600");}
        else throw std::invalid_argument("Unknown option");
    }
    if(result.font.empty())throw std::invalid_argument("--font <TrueType file> is required");
    // One content source per session: a read-only view never silently edits a transition file.
    if(!result.sceneOverlay.empty()&&(result.probe||result.transitionsGiven))throw std::invalid_argument("--scene-overlay cannot be combined with --probe or --transitions");
    if(!result.specView.empty()&&(result.probe||result.transitionsGiven||!result.sceneOverlay.empty()))throw std::invalid_argument("--spec-view cannot be combined with --probe, --transitions or --scene-overlay");
    // --probe already owns the probe target window, and the transition editor needs the Unity anchors.
    if(result.attachProbeTarget&&(result.probe||(result.specView.empty()&&result.sceneOverlay.empty())))throw std::invalid_argument("--attach probe-target requires --spec-view or --scene-overlay");
    return result;
}
