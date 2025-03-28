#include "utils.h"
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

bool pathExists(const std::string &path) {
    // Заметим: если используются переменные окружения, их можно разворачивать отдельно
    return fs::exists(path);
}

std::vector<fs::path> listFiles(const std::string &directory) {
    std::vector<fs::path> files;
    if (!fs::exists(directory)) return files;
    
    for (const auto &entry : fs::directory_iterator(directory)) {
        files.push_back(entry.path());
    }
    return files;
}
