// @implements SPEC-TL-TRANSITIONS
// @spec Transition authoring
#include <tela/transitions.hpp>
#include <iostream>
#include <fstream>

int main(int argc,char** argv){
    if(argc!=2)return 1;const std::filesystem::path path=argv[1];
    try{
        tela::Transitions data;data.set({"id","Screen A","Screen B","flag == \"ready\"","GlobalObjectId-V1-2"});
        data.save(path);tela::Transitions read;read.load(path);if(read.entries()!=data.entries())return 2;
        auto changed=read.entries()[0];changed.condition="coins >= 5";read.set(changed);read.save(path);data.load(path);if(data.entries()[0].condition!=changed.condition)return 3;
        {std::ofstream broken(path);broken<<"TELA_TRANSITIONS 9\n";}
        try{data.load(path);return 4;}catch(const std::invalid_argument&){}
        if(data.entries()[0].condition!=changed.condition)return 5;
        // A leftover pending file must be reported, never silently overwritten,
        // and must leave both the destination and the pending file untouched.
        auto pending=path;pending+=".pending";
        {std::ofstream stray(pending);stray<<"unrecovered\n";}
        try{data.save(path);return 7;}catch(const std::runtime_error&){}
        {std::ifstream check(pending);std::string line;
         if(!std::getline(check,line)||line!="unrecovered")return 8;}
        std::filesystem::remove(pending);
        // Recovery restores normal saving.
        data.save(path);
        std::filesystem::remove(path);return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';std::error_code error;std::filesystem::remove(path,error);
        auto pending=path;pending+=".pending";std::filesystem::remove(pending,error);return 6;}
}
