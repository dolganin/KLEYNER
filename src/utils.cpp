#include "utils.h"
#include <filesystem>
#include <vector>
#include <cstdlib>

namespace fs = std::filesystem;

/// Разворачиваем переменные окружения вида %VAR% и тильду
std::string expandPath(const std::string &path) {
    std::string result;
    for (size_t i = 0; i < path.size(); ) {
        if (path[i] == '%' && path.find('%', i + 1) != std::string::npos) {
            size_t end = path.find('%', i + 1);
            std::string var = path.substr(i + 1, end - i - 1);
            const char *val = std::getenv(var.c_str());
            if (val)
                result += val;
            else
                result += "%" + var + "%";
            i = end + 1;
            continue;
        }
        if (path[i] == '~' && (i == 0 || path[i-1] == '/')) {
            const char *home = std::getenv("HOME");
            if (home) result += home;
            ++i;
            continue;
        }
        result += path[i++];
    }
    return result;
}

bool pathExists(const std::string &path) {
    return fs::exists(expandPath(path));
}

std::vector<fs::path> listFiles(const std::string &directory) {
    std::vector<fs::path> files;
    std::string dir = expandPath(directory);
    if (!fs::exists(dir)) return files;

    for (const auto &entry : fs::directory_iterator(dir)) {
        files.push_back(entry.path());
    }
    return files;
}

std::uintmax_t directorySize(const fs::path &path) {
    std::uintmax_t size = 0;
    for (auto it = fs::recursive_directory_iterator(
                 path, fs::directory_options::skip_permission_denied);
         it != fs::recursive_directory_iterator(); ++it) {
        std::error_code ec;
        if (it->is_regular_file(ec)) {
            size += it->file_size(ec);
        }
    }
    return size;
}
