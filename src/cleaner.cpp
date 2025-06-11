#include "cleaner.h"
#include <iostream>

namespace fs = std::filesystem;

std::vector<fs::path> default_candidate_paths(){
#ifdef _WIN32
    char* tmp=getenv("TEMP");
    std::vector<fs::path> p;
    if(tmp) p.emplace_back(tmp);
    char* user=getenv("USERPROFILE");
    if(user) p.emplace_back(std::string(user)+"/AppData/Local/Temp");
    p.emplace_back("C:/Windows/Temp");
    p.emplace_back("C:/Windows/Logs");
    return p;
#else
    std::vector<fs::path> p = {"/tmp","/var/tmp",fs::path(getenv("HOME"))/".cache", "/var/cache/apt/archives"};
    return p;
#endif
}

static uintmax_t remove_entry(const fs::path& p,bool dry){
    uintmax_t freed=0;
    std::error_code ec;
    if(fs::is_regular_file(p,ec)){
        freed=fs::file_size(p,ec);
        if(!dry) fs::remove(p,ec);
    }else if(fs::is_directory(p,ec)){
        for(auto& entry: fs::recursive_directory_iterator(p, fs::directory_options::skip_permission_denied)){
            freed+=remove_entry(entry.path(),dry);
        }
        if(!dry) fs::remove_all(p,ec);
    }
    return freed;
}

uintmax_t clean_paths(const std::vector<fs::path>& paths,const CleanOptions& opt){
    uintmax_t total=0;
    for(auto& p: paths){
        total+=remove_entry(p,opt.dry_run);
    }
    return total;
}
