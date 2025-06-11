#include "scanner.h"
#include <algorithm>

namespace fs = std::filesystem;

static bool is_excluded(const fs::path& p, const std::vector<fs::path>& ex){
    for(const auto& e: ex){
        if(p.string().rfind(e.string(),0)==0)
            return true;
    }
    return false;
}

std::map<fs::path, uintmax_t>
scan_directory_sizes(const fs::path& root, const std::vector<fs::path>& exclude){
    std::map<fs::path,uintmax_t> sizes;
    for(auto it=fs::recursive_directory_iterator(root, fs::directory_options::skip_permission_denied);
        it!=fs::recursive_directory_iterator(); ++it){
        if(is_excluded(it->path(), exclude)){
            it.disable_recursion_pending();
            continue;
        }
        if(it->is_regular_file()){
            uintmax_t s = 0;
            std::error_code ec;
            s = it->file_size(ec);
            if(ec) s = 0;
            fs::path cur = it->path().parent_path();
            while(cur.string().rfind(root.string(),0)==0){
                sizes[cur]+=s;
                if(cur==root) break;
                cur = cur.parent_path();
            }
        }
    }
    return sizes;
}
