#ifndef _WIN32
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>

__attribute__((constructor))
static void ensure_root(int argc, char** argv){
    if(geteuid()!=0){
        fprintf(stderr,"Re-running with sudo...\n");
        std::vector<char*> args(argv, argv+argc);
        args.insert(args.begin(), const_cast<char*>("sudo"));
        args.push_back(nullptr);
        execvp("sudo", args.data());
        perror("execvp");
        exit(1);
    }
}
#endif
