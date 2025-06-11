#include "cleaner.h"
#include "logger.h"
#include "utils.h"

#include <filesystem>
#include <system_error>
#include <thread>
#include <vector>
#include <algorithm>
#include <cctype>

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

Cleaner::Cleaner(const Config &config) : config(config) {
    buildTargetPaths();
}

/// Формирование списка целевых директорий для очистки
void Cleaner::buildTargetPaths() {
    if (config.targetOS == OS_TYPE::WINDOWS || config.targetOS == OS_TYPE::AUTO) {
        std::vector<std::string> winPaths = {
            "%TEMP%",
            "%TMP%",
            "C:\\Windows\\Temp",
            "C:\\Windows\\Prefetch"
        };
        for (auto p : winPaths) {
            if (config.wsl) {
                p = transformPathForWSL(p);
            }
            targetPaths.push_back(p);
        }
    }
    
    if (config.targetOS == OS_TYPE::LINUX || config.targetOS == OS_TYPE::AUTO) {
        targetPaths.push_back("/tmp");
        targetPaths.push_back("/var/tmp");
    }
    
    if (config.cleanWindows) {
        std::vector<std::string> extraWinPaths = {
            "C:\\Windows\\SoftwareDistribution\\Download",
            "C:\\Windows\\Panther",
            "C:\\Windows\\WER"
        };
        for (auto p : extraWinPaths) {
            if (config.wsl) {
                p = transformPathForWSL(p);
            }
            targetPaths.push_back(p);
        }
    }
    
    for (const auto &path : config.additionalPaths) {
        targetPaths.push_back(path);
    }
}

/// Подсчет количества файлов, папок и общего размера перед удалением
std::tuple<size_t, size_t, double> Cleaner::countItemsToDelete() {
    size_t fileCount = 0, dirCount = 0;
    uintmax_t totalSize = 0;

    for (const auto &path : targetPaths) {
        if (!pathExists(path)) continue;
        if (path.find("systemd-private") != std::string::npos) continue;

        try {
            for (auto it = fs::recursive_directory_iterator(
                         path, fs::directory_options::skip_permission_denied);
                 it != fs::recursive_directory_iterator(); ++it) {
                const fs::path &p = it->path();
                if (p.string().find("systemd-private") != std::string::npos) {
                    it.disable_recursion_pending();
                    continue;
                }
                if (!config.includeHidden && p.filename().string().front() == '.')
                    continue;

                if (it->is_directory()) {
                    dirCount++;
                } else if (it->is_regular_file()) {
                    fileCount++;
                    totalSize += it->file_size();
                }
            }
        } catch (const fs::filesystem_error &e) {
            LOG_WARNING("Отказ в доступе к " + path + ": " + e.what());
        }
    }

    return {fileCount, dirCount, static_cast<double>(totalSize) / (1024 * 1024)};
}


/// Запуск процесса очистки
void Cleaner::run() {
    LOG_INFO("Запуск очистки по следующим путям:");
    for (const auto &path : targetPaths) {
        LOG_INFO(" -> " + path);
    }

    for (const auto &path : targetPaths) {
        processPath(path);
    }
}

/// Рекурсивная обработка одного пути
/// Рекурсивная обработка одного пути
void Cleaner::processPath(const std::string &path) {
    if (!pathExists(path)) {
        LOG_DEBUG("Путь не существует: " + path);
        return;
    }
    
    // Исключаем systemd-private каталоги
    if (path.find("systemd-private") != std::string::npos) {
        LOG_DEBUG("Пропущен защищённый системный путь: " + path);
        return;
    }
    
    try {
        std::vector<fs::path> entries;
        for (auto it = fs::recursive_directory_iterator(
                     path, fs::directory_options::skip_permission_denied);
             it != fs::recursive_directory_iterator(); ++it) {
            const fs::path &p = it->path();
            if (p.string().find("systemd-private") != std::string::npos) {
                it.disable_recursion_pending();
                continue;
            }
            if (!config.includeHidden && p.filename().string().front() == '.')
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
        // Обработка ошибок доступа, например, "Permission denied"
        if (std::string(e.what()).find("Permission denied") != std::string::npos) {
            LOG_WARNING("Отказ в доступе к " + path + ". Пропускаем.");
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
    } else {
        LOG_INFO("Удалено: " + path);
    }
}
