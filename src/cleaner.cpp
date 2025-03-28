#include "cleaner.h"
#include "logger.h"
#include "utils.h"

#include <filesystem>
#include <thread>
#include <vector>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

// Вспомогательная функция для преобразования Windows-пути в WSL-формат.
// Например: "C:\\Windows\\Temp" -> "/mnt/c/Windows/Temp"
static std::string transformPathForWSL(const std::string &path) {
    if (path.size() >= 3 && std::isalpha(path[0]) && path[1] == ':') {
        char driveLetter = std::tolower(path[0]);
        std::string rest = path.substr(2); // Убираем "C:".
        // Заменяем обратные слеши на прямые.
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
    // Если целевая ОС Windows или AUTO, добавляем Windows пути
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
    
    // Если целевая ОС Linux или AUTO, добавляем Linux пути
    if (config.targetOS == OS_TYPE::LINUX || config.targetOS == OS_TYPE::AUTO) {
        targetPaths.push_back("/tmp");
        targetPaths.push_back("/var/tmp");
    }
    
    // Если включён флаг cleanWindows (например, при запуске под WSL), добавляем дополнительные Windows пути
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
    
    // Добавляем дополнительные пути из конфигурационного файла
    for (const auto &path : config.additionalPaths) {
        targetPaths.push_back(path);
    }
}

/// Запуск процесса очистки
void Cleaner::run() {
    LOG_INFO("Запуск очистки по следующим путям:");
    for (const auto &path : targetPaths) {
        LOG_INFO(" -> " + path);
    }
    
    // Последовательная обработка (параллельность можно добавить позже)
    for (const auto &path : targetPaths) {
        processPath(path);
    }
}

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
        for (auto &entry : fs::recursive_directory_iterator(path)) {
            std::string entryPath = entry.path().string();
            // Фильтруем скрытые файлы, если соответствующий флаг не включён
            if (!config.includeHidden && entry.path().filename().string().front() == '.')
                continue;
            
            // В режиме dry-run выводим, что будет удалено, иначе производим удаление
            if (config.dryRun) {
                LOG_INFO("[Dry Run] Будет удалено: " + entryPath);
            } else {
                deleteEntry(entryPath);
            }
        }
    } catch (const fs::filesystem_error &e) {
        LOG_ERROR("Ошибка доступа к " + path + ": " + e.what());
    }
}

/// Удаление файла или директории (с учётом dry-run)
void Cleaner::deleteEntry(const std::string &path) {
    try {
        if (fs::is_directory(path))
            fs::remove_all(path);
        else
            fs::remove(path);
        LOG_INFO("Удалено: " + path);
    } catch (const fs::filesystem_error &e) {
        LOG_ERROR("Ошибка удаления " + path + ": " + e.what());
    }
}
