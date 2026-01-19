#include "utils.h"
#include <filesystem>
#include <vector>
#include <cstdlib>
#include <fstream>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

/// Разворачиваем переменные окружения вида %VAR% и тильду
std::string expandPath(const std::string &path) {
    std::string result;
    for (size_t i = 0; i < path.size(); ) {
        if (path[i] == '%' && path.find('%', i + 1) != std::string::npos) {
            size_t end = path.find('%', i + 1);
            std::string var = path.substr(i + 1, end - i - 1);
            const char *val = std::getenv(var.c_str());
            std::string upperVar = var;
            std::transform(upperVar.begin(), upperVar.end(), upperVar.begin(),
                           [](unsigned char c) { return std::toupper(c); });
            bool wsl = isWSL();
            bool preferWindows = wsl && (upperVar == "USERPROFILE" ||
                                         upperVar == "LOCALAPPDATA" ||
                                         upperVar == "APPDATA" ||
                                         upperVar == "TEMP" ||
                                         upperVar == "TMP" ||
                                         upperVar == "PROGRAMDATA");
            if (val && !preferWindows) {
                result += val;
            } else if (wsl) {
                std::string winHome;
                const char *envHome = std::getenv("USERPROFILE");
                if (envHome && *envHome) {
                    winHome = envHome;
                } else {
                    fs::path base("/mnt/c/Users");
                    if (fs::exists(base) && fs::is_directory(base)) {
                        const char *user = std::getenv("USER");
                        if (user && *user) {
                            fs::path candidate = base / user;
                            if (fs::exists(candidate))
                                winHome = candidate.string();
                        }
                        if (winHome.empty()) {
                            std::vector<std::string> candidates;
                            for (const auto &entry : fs::directory_iterator(base)) {
                                if (!entry.is_directory()) continue;
                                std::string name = entry.path().filename().string();
                                if (name == "Public" || name == "Default" ||
                                    name == "Default User" || name == "All Users")
                                    continue;
                                candidates.push_back(entry.path().string());
                            }
                            if (candidates.size() == 1)
                                winHome = candidates.front();
                        }
                    }
                }
                if (!winHome.empty()) {
                    if (upperVar == "USERPROFILE") {
                        result += winHome;
                    } else if (upperVar == "LOCALAPPDATA") {
                        result += winHome + "/AppData/Local";
                    } else if (upperVar == "APPDATA") {
                        result += winHome + "/AppData/Roaming";
                    } else if (upperVar == "TEMP" || upperVar == "TMP") {
                        result += winHome + "/AppData/Local/Temp";
                    } else if (upperVar == "PROGRAMDATA") {
                        result += "/mnt/c/ProgramData";
                    } else if (val) {
                        result += val;
                    } else {
                        result += "%" + var + "%";
                    }
                } else {
                    if (val) result += val;
                    else result += "%" + var + "%";
                }
            } else {
                if (val) result += val;
                else result += "%" + var + "%";
            }
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
    std::error_code ec;
    if (!fs::exists(path, ec)) return 0;
    if (fs::is_regular_file(path, ec)) return fs::file_size(path, ec);
    std::uintmax_t size = 0;
    for (auto it = fs::recursive_directory_iterator(
                 path, fs::directory_options::skip_permission_denied);
         it != fs::recursive_directory_iterator(); ++it) {
        if (it->is_regular_file(ec)) {
            size += it->file_size(ec);
        }
    }
    return size;
}

bool isWSL() {
#ifdef _WIN32
    return false;
#else
    const char *env = std::getenv("WSL_DISTRO_NAME");
    if (env && *env) return true;
    env = std::getenv("WSL_INTEROP");
    if (env && *env) return true;
    std::ifstream in("/proc/version");
    if (!in) return false;
    std::string line;
    std::getline(in, line);
    return line.find("Microsoft") != std::string::npos || line.find("WSL") != std::string::npos;
#endif
}
