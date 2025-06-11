#include "scanner.h"
#include "cleaner.h"
#include "ui.h"
#include <filesystem>
#include <iostream>
#include <vector>
#include <string>
#ifndef _WIN32
#include <unistd.h>
#endif

int main(int argc, char* argv[]) {
    bool dryRun = false;
    bool json = false;
    std::vector<std::filesystem::path> exclude;

    for(int i=1;i<argc;++i){
        std::string arg = argv[i];
        if(arg=="--dry-run") dryRun=true;
        else if(arg=="--json") json=true;
        else if(arg=="--exclude" && i+1<argc){
            std::string ex = argv[++i];
            size_t pos=0;
            while((pos=ex.find(','))!=std::string::npos){
                exclude.push_back(ex.substr(0,pos));
                ex.erase(0,pos+1);
            }
            if(!ex.empty()) exclude.push_back(ex);
        }
        else if(arg=="--self-update") {
            system("git pull");
            system("scripts/build_nix.sh");
            return 0;
        }
        else if(arg=="--help") {
            std::cout<<"Usage: cleaner [--dry-run] [--json] [--exclude p1,p2] [--self-update]"<<std::endl;
            return 0;
        }
    }

#ifdef _WIN32
    // elevation handled via platform_win.cpp
#else
    if(geteuid()!=0){
        std::cerr<<"Needs root. Try sudo."<<std::endl;
        return 1;
    }
#endif

    auto sizes = scan_directory_sizes(std::filesystem::path{"/"}, exclude);
    print_top(sizes, 10, json);

    auto candidates = default_candidate_paths();
    auto freed = clean_paths(candidates, {dryRun});
    std::cerr<<"Freed "<<freed/1024/1024<<" MB"<<std::endl;
    return 0;
}
