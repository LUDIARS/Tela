// @spec SPEC-TL-TRANSITIONS
#include <tela/transitions.hpp>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace tela {
namespace {
void validate(const Transition& t) {
    for (auto s : {&t.id,&t.source,&t.destination,&t.condition,&t.object_id})
        if(s->size()>4096 || s->find_first_of("\r\n\0",0,3)!=std::string::npos)
            throw std::invalid_argument("Transition fields must be bounded single-line strings");
    if(t.id.empty() || t.source.empty() || t.destination.empty() || t.object_id.empty())
        throw std::invalid_argument("Transition ID, source, destination and object ID are required");
}
}
void Transitions::set(Transition value) {
    validate(value);
    auto it=std::find_if(entries_.begin(),entries_.end(),[&](auto& e){return e.id==value.id;});
    if(it!=entries_.end())*it=std::move(value);
    else {if(entries_.size()>=4096)throw std::invalid_argument("Too many transitions");entries_.push_back(std::move(value));}
}
void Transitions::erase(const std::string& id){std::erase_if(entries_,[&](auto& e){return e.id==id;});}
void Transitions::load(const std::filesystem::path& path) {
    if(std::filesystem::file_size(path)>4*1024*1024)throw std::invalid_argument("Transition file exceeds 4 MiB");
    std::ifstream input(path,std::ios::binary); std::string line;
    if(!input || !std::getline(input,line) || line!="TELA_TRANSITIONS 1")throw std::invalid_argument("Unsupported transition file");
    Transitions next;
    while(std::getline(input,line)) {
        if(line.empty())continue;
        std::istringstream row(line);Transition t;
        if(!(row>>std::quoted(t.id)>>std::quoted(t.source)>>std::quoted(t.destination)>>std::quoted(t.condition)>>std::quoted(t.object_id)))throw std::invalid_argument("Invalid transition record");
        row>>std::ws;if(!row.eof())throw std::invalid_argument("Trailing transition data");
        if(std::any_of(next.entries_.begin(),next.entries_.end(),[&](auto& e){return e.id==t.id;}))throw std::invalid_argument("Duplicate transition ID");
        next.set(std::move(t));
    }
    if(input.bad())throw std::runtime_error("Transition read failed");
    entries_=std::move(next.entries_);
}
void Transitions::save(const std::filesystem::path& path) const {
    auto temporary=path;temporary += ".pending";
    if(std::filesystem::exists(temporary))throw std::runtime_error("Pending transition save exists; resolve it before overwriting");
    {
        std::ofstream output(temporary,std::ios::binary|std::ios::trunc);
        if(!output)throw std::runtime_error("Cannot open transition save");
        output<<"TELA_TRANSITIONS 1\n";
        for(auto& t:entries_){validate(t);output<<std::quoted(t.id)<<' '<<std::quoted(t.source)<<' '<<std::quoted(t.destination)<<' '<<std::quoted(t.condition)<<' '<<std::quoted(t.object_id)<<'\n';}
        output.flush();if(!output)throw std::runtime_error("Transition save failed; previous file retained");
    }
#ifdef _WIN32
    if(!MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
        throw std::runtime_error("Cannot commit transition save; previous file retained");
#else
    std::filesystem::rename(temporary,path);
#endif
}
}
