#include "overlay_options.hpp"
#include <string_view>
#include <stdexcept>
// @spec Overlay lifecycle
Options options(int argc,char** argv){
    Options result;
    for(int i=1;i<argc;++i){std::string_view arg=argv[i];
        if(arg=="--probe"){result.probe=true;continue;}
        if(i+1>=argc)throw std::invalid_argument("Missing option value");
        std::string value=argv[++i];
        if(arg=="--font")result.font=value;else if(arg=="--pipe")result.pipe=value;else if(arg=="--transitions"){result.file=value;result.transitionsGiven=true;}else if(arg=="--report")result.report=value;
        else if(arg=="--scene-overlay")result.sceneOverlay=value;
        else if(arg=="--seconds"){result.seconds=std::stoi(value);if(result.seconds<1||result.seconds>3600)throw std::invalid_argument("seconds must be 1..3600");}
        else throw std::invalid_argument("Unknown option");
    }
    if(result.font.empty())throw std::invalid_argument("--font <TrueType file> is required");
    // One content source per session: a scene overlay never silently edits a transition file.
    if(!result.sceneOverlay.empty()&&(result.probe||result.transitionsGiven))throw std::invalid_argument("--scene-overlay cannot be combined with --probe or --transitions");
    return result;
}
