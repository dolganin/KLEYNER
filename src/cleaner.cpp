#include "cleaner.h"
#include "logger.h"
#include "utils.h"

#include <filesystem>
#include <system_error>
#include <vector>
#include <algorithm>
#include <cctype>
#include <set>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <iostream>

namespace fs = std::filesystem;

// Вспомогательная функция для преобразования Windows-пути в WSL-формат
static std::string transformPathForWSL(const std::string &path) {
    if (path.size() >= 3 && std::isalpha(path[0]) && path[1] == ':') {
        char driveLetter = std::tolower(path[0]);
        std::string rest = path.substr(2);
        std::replace(rest.begin(), rest.end(), '\\', '/');
        return "/mnt/" + std::string(1, driveLetter) + rest;
    }
    return path;
}

static std::string toLowerStr(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}

static bool hasWildcard(const std::string &s) {
    return s.find('*') != std::string::npos || s.find('?') != std::string::npos;
}

static bool wildcardMatch(const std::string &text, const std::string &pattern) {
    size_t t = 0;
    size_t p = 0;
    size_t star = std::string::npos;
    size_t match = 0;
    while (t < text.size()) {
        if (p < pattern.size() && (pattern[p] == '?' || pattern[p] == text[t])) {
            ++t;
            ++p;
        } else if (p < pattern.size() && pattern[p] == '*') {
            star = p++;
            match = t;
        } else if (star != std::string::npos) {
            p = star + 1;
            t = ++match;
        } else {
            return false;
        }
    }
    while (p < pattern.size() && pattern[p] == '*') ++p;
    return p == pattern.size();
}

static std::string normalizeSeparators(std::string path) {
    std::replace(path.begin(), path.end(), '\\', '/');
    return path;
}

static std::vector<std::string> splitSegments(const std::string &path, size_t start) {
    std::vector<std::string> segs;
    std::string current;
    for (size_t i = start; i < path.size(); ++i) {
        char c = path[i];
        if (c == '/') {
            if (!current.empty()) {
                segs.push_back(current);
                current.clear();
            }
        } else {
            current.push_back(c);
        }
    }
    if (!current.empty()) segs.push_back(current);
    return segs;
}

static void expandGlobRecursive(const fs::path &base,
                                const std::vector<std::string> &segs,
                                size_t index,
                                std::vector<std::string> &out) {
    if (index >= segs.size()) {
        out.push_back(base.string());
        return;
    }
    const std::string &seg = segs[index];
    bool wildcard = hasWildcard(seg);
    if (!wildcard) {
        fs::path next = base / seg;
        std::error_code ec;
        if (!fs::exists(next, ec)) return;
        if (index + 1 < segs.size() && !fs::is_directory(next, ec)) return;
        expandGlobRecursive(next, segs, index + 1, out);
        return;
    }
    std::error_code ec;
    if (!fs::exists(base, ec) || !fs::is_directory(base, ec)) return;
    for (auto it = fs::directory_iterator(base, fs::directory_options::skip_permission_denied);
         it != fs::directory_iterator(); ++it) {
        std::string name = it->path().filename().string();
        if (!wildcardMatch(name, seg)) continue;
        fs::path next = it->path();
        if (index + 1 < segs.size() && !it->is_directory(ec)) continue;
        expandGlobRecursive(next, segs, index + 1, out);
    }
}

static std::vector<std::string> expandGlob(const std::string &pattern) {
    std::vector<std::string> results;
    std::string norm = normalizeSeparators(pattern);
    fs::path base;
    size_t start = 0;
    if (norm.size() >= 3 && std::isalpha(static_cast<unsigned char>(norm[0])) &&
        norm[1] == ':' && norm[2] == '/') {
        base = norm.substr(0, 3);
        start = 3;
    } else if (!norm.empty() && norm[0] == '/') {
        base = "/";
        start = 1;
    } else {
        base = ".";
        start = 0;
    }
    std::vector<std::string> segs = splitSegments(norm, start);
    expandGlobRecursive(base, segs, 0, results);
    std::set<std::string> unique(results.begin(), results.end());
    return std::vector<std::string>(unique.begin(), unique.end());
}

static std::string formatSize(uintmax_t bytes) {
    const double mb = 1024.0 * 1024.0;
    const double gb = mb * 1024.0;
    std::ostringstream out;
    out.setf(std::ios::fixed);
    out << std::setprecision(2);
    if (bytes >= static_cast<uintmax_t>(gb)) {
        out << (bytes / gb) << " GB";
    } else {
        out << (bytes / mb) << " MB";
    }
    return out.str();
}

static bool commandExistsLocal(const std::string &cmd) {
#ifdef _WIN32
    std::string check = "where " + cmd + " >nul 2>&1";
#else
    std::string check = "command -v " + cmd + " >/dev/null 2>&1";
#endif
    return std::system(check.c_str()) == 0;
}

#ifndef _WIN32
static std::string shellEscape(const std::string &input) {
    std::string out;
    out.reserve(input.size() + 2);
    out.push_back('\'');
    for (char c : input) {
        if (c == '\'') {
            out.append("'\\''");
        } else {
            out.push_back(c);
        }
    }
    out.push_back('\'');
    return out;
}
#endif

Cleaner::Cleaner(const Config &config) : config(config) {
    buildTargetPaths();
}

static std::vector<PathEntry> defaultWindowsEntries() {
    return {
        {"temp", "%TEMP%"},
        {"tmp", "%TMP%"},
        {"windows_temp", "C:\\Windows\\Temp"},
        {"prefetch", "C:\\Windows\\Prefetch"},
        {"chrome_cache", "%LocalAppData%\\Google\\Chrome\\User Data\\Default\\Cache"},
        {"firefox_cache", "%AppData%\\Mozilla\\Firefox\\Profiles\\*\\cache2"},
        {"edge_cache", "%LocalAppData%\\Microsoft\\Edge\\User Data\\Default\\Cache"},
        {"thumbcache", "%LocalAppData%\\Microsoft\\Windows\\Explorer\\thumbcache_*.db"},
        {"nuget_http_cache", "%LocalAppData%\\NuGet\\v3-cache"},
        {"nuget_global_packages", "%UserProfile%\\.nuget\\packages"},
        {"nuget_temp", "%TEMP%\\NuGetScratch"},
        {"nuget_plugins_cache", "%LocalAppData%\\NuGet\\plugins-cache"},
        {"npm_cache", "%LocalAppData%\\npm-cache"},
        {"yarn_cache", "%LocalAppData%\\Yarn\\Cache"},
        {"pnpm_store", "%LocalAppData%\\pnpm\\store"},
        {"pip_cache", "%LocalAppData%\\pip\\Cache"},
        {"go_build_cache", "%LocalAppData%\\go-build"},
        {"go_mod_cache", "%UserProfile%\\go\\pkg\\mod"},
        {"cargo_registry", "%UserProfile%\\.cargo\\registry"},
        {"cargo_git", "%UserProfile%\\.cargo\\git"},
        {"gradle_cache", "%UserProfile%\\.gradle\\caches"},
        {"gradle_wrapper_dists", "%UserProfile%\\.gradle\\wrapper\\dists"},
        {"gradle_daemon", "%UserProfile%\\.gradle\\daemon"},
        {"gradle_jdks", "%UserProfile%\\.gradle\\jdks"},
        {"maven_repo", "%UserProfile%\\.m2\\repository"},
        {"docker_cache", "%UserProfile%\\.docker\\cache"},
        {"docker_images", "%ProgramData%\\DockerDesktop\\image\\overlay2"},
        {"docker_desktop", "%LocalAppData%\\Docker"},
        {"downloads", "%UserProfile%\\Downloads"},
        {"windows_update_cleanup", "C:\\Windows\\SoftwareDistribution\\Download"},
        {"windows_update_logs", "C:\\Windows\\Panther"},
        {"windows_error_reporting", "C:\\Windows\\WER"}
    };
}

static std::vector<PathEntry> defaultLinuxEntries() {
    return {
        {"tmp", "/tmp"},
        {"var_tmp", "/var/tmp"},
        {"apt_cache", "/var/cache/apt/archives"},
        {"yum_cache", "/var/cache/yum"},
        {"pip_cache", "~/.cache/pip"},
        {"npm_cache", "~/.npm"},
        {"nuget_global_packages", "~/.nuget/packages"},
        {"nuget_http_cache", "~/.local/share/NuGet/v3-cache"},
        {"nuget_temp", "/tmp/NuGetScratch*"},
        {"nuget_plugins_cache", "~/.local/share/NuGet/plugins-cache"},
        {"yarn_cache", "~/.cache/yarn"},
        {"pnpm_store", "~/.pnpm-store"},
        {"pnpm_store_alt", "~/.local/share/pnpm/store"},
        {"go_build_cache", "~/.cache/go-build"},
        {"go_mod_cache", "~/go/pkg/mod"},
        {"cargo_registry", "~/.cargo/registry"},
        {"cargo_git", "~/.cargo/git"},
        {"gradle_cache", "~/.gradle/caches"},
        {"gradle_wrapper_dists", "~/.gradle/wrapper/dists"},
        {"gradle_daemon", "~/.gradle/daemon"},
        {"gradle_jdks", "~/.gradle/jdks"},
        {"maven_repo", "~/.m2/repository"},
        {"ccache", "~/.cache/ccache"},
        {"user_cache", "~/.cache"},
        {"downloads", "~/Downloads"},
        {"docker_cache", "~/.docker/cache"},
        {"docker_images", "/var/lib/docker/overlay2"},
        {"containerd", "/var/lib/containerd"},
        {"old_logs", "/var/log/*.log.1"},
        {"old_logs_gz", "/var/log/*.gz"},
        {"journal_logs", "/var/log/journal"},
        {"trash", "~/.local/share/Trash"}
    };
}

static bool isWindowsSystemEntry(const PathEntry &entry) {
    std::string key = toLowerStr(entry.key);
    std::string val = toLowerStr(entry.value);
    if (key.rfind("windows_", 0) == 0) return true;
    if (val.find("c:\\windows") != std::string::npos) return true;
    if (val.find("/windows") != std::string::npos && val.find("/mnt/") != std::string::npos) return true;
    return false;
}

void Cleaner::buildTargetPaths() {
    targets.clear();
    bool includeWindows = config.targetOS == OS_TYPE::WINDOWS || config.targetOS == OS_TYPE::BOTH;
    bool includeLinux = config.targetOS == OS_TYPE::LINUX || config.targetOS == OS_TYPE::BOTH;
    if (config.targetOS == OS_TYPE::AUTO) {
#ifdef _WIN32
        includeWindows = true;
#else
        includeLinux = true;
#endif
    }

    std::vector<PathEntry> winEntries = config.windowsPaths.empty()
        ? defaultWindowsEntries()
        : config.windowsPaths;
    std::vector<PathEntry> linuxEntries = config.linuxPaths.empty()
        ? defaultLinuxEntries()
        : config.linuxPaths;

    if (includeWindows) {
        for (const auto &entry : winEntries) {
            if (!config.cleanWindows && isWindowsSystemEntry(entry)) continue;
            addTargetGroup("Windows", entry, true);
        }
    }

    if (includeLinux) {
        for (const auto &entry : linuxEntries) {
            addTargetGroup("Linux", entry, false);
        }
    }

    for (const auto &entry : config.commonPaths) {
        addTargetGroup("Common", entry, false);
    }

    for (const auto &path : config.additionalPaths) {
        PathEntry entry;
        entry.key = "extra";
        entry.value = path;
        addTargetGroup("Extra", entry, false);
    }
}

/// Подсчет количества файлов, папок и общего размера перед удалением
std::tuple<size_t, size_t, double> Cleaner::countItemsToDelete() {
    size_t fileCount = 0, dirCount = 0;
    uintmax_t totalSize = 0;

    for (const auto &group : targets) {
        for (const auto &path : group.paths) {
            if (!pathExists(path)) continue;
            if (path.find("systemd-private") != std::string::npos) continue;

            std::error_code ec;
            fs::path root(path);
            if (fs::is_regular_file(root, ec)) {
                std::string name = root.filename().string();
                if (!config.includeHidden && !name.empty() && name.front() == '.') continue;
                fileCount++;
                totalSize += fs::file_size(root, ec);
                continue;
            }

            try {
                for (auto it = fs::recursive_directory_iterator(
                             path, fs::directory_options::skip_permission_denied);
                     it != fs::recursive_directory_iterator(); ++it) {
                    const fs::path &p = it->path();
                    if (p.string().find("systemd-private") != std::string::npos) {
                        it.disable_recursion_pending();
                        continue;
                    }
                    std::string name = p.filename().string();
                    if (!config.includeHidden && !name.empty() && name.front() == '.')
                        continue;

                std::error_code ec;
                if (it->is_directory(ec)) {
                    if (!ec) dirCount++;
                } else if (it->is_regular_file(ec)) {
                    if (!ec) {
                        fileCount++;
                        std::error_code sizeEc;
                        totalSize += it->file_size(sizeEc);
                    }
                }
            }
            } catch (const fs::filesystem_error &e) {
                LOG_WARNING("Отказ в доступе к " + path + ": " + e.what());
            }
        }
    }

    return {fileCount, dirCount, static_cast<double>(totalSize) / (1024 * 1024)};
}


/// Запуск процесса очистки
void Cleaner::run() {
    deniedPaths.clear();
    LOG_INFO("Запуск очистки:");
    for (const auto &group : targets) {
        LOG_INFO(" -> " + group.scope + " / " + group.name);
        if (config.verbose) {
            for (const auto &path : group.paths) {
                LOG_INFO("    " + path);
            }
        }
    }

    for (const auto &group : targets) {
        for (const auto &path : group.paths) {
            processPath(path);
        }
    }

    if (!deniedPaths.empty()) {
        LOG_WARNING("Не удалось очистить " + std::to_string(deniedPaths.size()) + " путей из-за прав доступа.");
        if (config.verbose) {
            for (const auto &p : deniedPaths) {
                LOG_WARNING("    " + p);
            }
        }
#ifndef _WIN32
        if (config.allowSudo && !config.dryRun) {
            if (!commandExistsLocal("sudo")) {
                LOG_WARNING("sudo не найден");
                return;
            }
            std::cout << "Повторить удаление с sudo для этих путей? (y/n): ";
            std::string answer;
            std::getline(std::cin, answer);
            if (answer == "y" || answer == "Y") {
                for (const auto &p : deniedPaths) {
                    std::string cmd = "sudo rm -rf -- " + shellEscape(p);
                    int code = std::system(cmd.c_str());
                    if (code != 0) {
                        LOG_WARNING("sudo удаление не удалось: " + p);
                    } else {
                        LOG_INFO("sudo удалено: " + p);
                    }
                }
            } else {
                LOG_INFO("sudo очистка отменена пользователем.");
            }
        }
#else
        (void)commandExistsLocal;
#endif
    }
}

/// Рекурсивная обработка одного пути
/// Рекурсивная обработка одного пути
void Cleaner::processPath(const std::string &path) {
    if (!pathExists(path)) {
        LOG_DEBUG("Путь не существует: " + path);
        return;
    }
    
    if (path.find("systemd-private") != std::string::npos) {
        LOG_DEBUG("Пропущен защищённый системный путь: " + path);
        return;
    }
    
    try {
        std::error_code ec;
        if (fs::is_regular_file(path, ec)) {
            if (config.dryRun) {
                LOG_INFO("[Dry Run] Будет удалено: " + path);
            } else {
                deleteEntry(path);
            }
            return;
        }

        std::vector<fs::path> entries;
        for (auto it = fs::recursive_directory_iterator(
                     path, fs::directory_options::skip_permission_denied);
             it != fs::recursive_directory_iterator(); ++it) {
            const fs::path &p = it->path();
            if (p.string().find("systemd-private") != std::string::npos) {
                it.disable_recursion_pending();
                continue;
            }
            std::string name = p.filename().string();
            if (!config.includeHidden && !name.empty() && name.front() == '.')
                continue;

            entries.push_back(p);
        }

        for (auto it = entries.rbegin(); it != entries.rend(); ++it) {
            std::string entryPath = it->string();
            if (config.dryRun) {
                LOG_INFO("[Dry Run] Будет удалено: " + entryPath);
            } else {
                deleteEntry(entryPath);
            }
        }
    } catch (const fs::filesystem_error &e) {
        if (e.code() == std::make_error_code(std::errc::permission_denied) ||
            std::string(e.what()).find("Permission denied") != std::string::npos) {
            LOG_WARNING("Отказ в доступе к " + path + ". Пропускаем.");
            addDeniedPath(path);
        } else {
            LOG_ERROR("Ошибка доступа к " + path + ": " + e.what());
        }
    }
}


/// Удаление файла или директории
void Cleaner::deleteEntry(const std::string &path) {
    std::error_code ec;
    if (fs::is_directory(path))
        fs::remove_all(path, ec);
    else
        fs::remove(path, ec);

    if (ec) {
        LOG_WARNING("Ошибка удаления " + path + ": " + ec.message());
        addDeniedPath(path);
    } else {
        LOG_INFO("Удалено: " + path);
    }
}

void Cleaner::printPlan() {
    LOG_INFO("План очистки:");
    struct Stat {
        size_t index;
        uintmax_t bytes;
    };
    std::vector<Stat> stats;
    stats.reserve(targets.size());
    std::vector<std::vector<uintmax_t>> sizes;
    sizes.resize(targets.size());
    uintmax_t totalBytes = 0;
    for (size_t i = 0; i < targets.size(); ++i) {
        const auto &group = targets[i];
        if (group.paths.empty()) continue;
        uintmax_t groupBytes = 0;
        sizes[i].reserve(group.paths.size());
        for (const auto &path : group.paths) {
            uintmax_t bytes = directorySize(path);
            sizes[i].push_back(bytes);
            groupBytes += bytes;
        }
        if (groupBytes == 0) continue;
        totalBytes += groupBytes;
        stats.push_back({i, groupBytes});
    }
    std::sort(stats.begin(), stats.end(),
              [](const Stat &a, const Stat &b) { return a.bytes > b.bytes; });

    for (const auto &stat : stats) {
        const auto &group = targets[stat.index];
        size_t nonZeroCount = 0;
        for (uintmax_t bytes : sizes[stat.index]) {
            if (bytes > 0) nonZeroCount++;
        }
        std::string line = group.scope + " / " + group.name + " - " +
            std::to_string(nonZeroCount) + " путей, " + formatSize(stat.bytes);
        LOG_INFO(line);
        if (config.verbose) {
            for (size_t i = 0; i < group.paths.size(); ++i) {
                uintmax_t bytes = sizes[stat.index][i];
                if (bytes == 0) continue;
                LOG_INFO("    " + group.paths[i] + " - " + formatSize(bytes));
            }
        }
    }
    LOG_INFO("Итого: " + formatSize(totalBytes));
}

void Cleaner::addTargetGroup(const std::string &scope, const PathEntry &entry, bool windowsPath) {
    std::string p = entry.value;
    if (windowsPath && config.wsl) {
        p = transformPathForWSL(p);
    }
    p = expandPath(p);
    if (p.empty() || p.find('%') != std::string::npos) return;

    TargetGroup group;
    group.scope = scope;
    group.name = entry.key;
    group.pattern = p;
    group.paths = resolvePattern(p);
    targets.push_back(std::move(group));
}

std::vector<std::string> Cleaner::resolvePattern(const std::string &path) const {
    std::string norm = normalizeSeparators(path);
    if (!hasWildcard(norm)) return {norm};
    return expandGlob(norm);
}

void Cleaner::addDeniedPath(const std::string &path) {
    if (std::find(deniedPaths.begin(), deniedPaths.end(), path) == deniedPaths.end())
        deniedPaths.push_back(path);
}
