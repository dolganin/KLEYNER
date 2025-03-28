#include "cleaner.h"
#include "logger.h"
#include "utils.h"

#include <filesystem>
#include <thread>
#include <vector>

// Для удобства пространства имён std::filesystem
namespace fs = std::filesystem;

Cleaner::Cleaner(const Config &config) : config(config) {
    buildTargetPaths();
}

/// Формирование списка целевых директорий для очистки
void Cleaner::buildTargetPaths() {
    // Пример: в зависимости от целевой ОС и параметров конфигурации
    if (config.targetOS == OS_TYPE::WINDOWS || config.targetOS == OS_TYPE::AUTO) {
        // Добавляем системные временные каталоги Windows
        targetPaths.push_back("%TEMP%");
        targetPaths.push_back("%TMP%");
        targetPaths.push_back("C:\\Windows\\Temp");
        targetPaths.push_back("C:\\Windows\\Prefetch");
        // Здесь можно добавить остальные пути из конфига (например, кэши браузеров, логи и т.д.)
    }
    if (config.targetOS == OS_TYPE::LINUX || config.targetOS == OS_TYPE::AUTO) {
        // Добавляем каталоги для Linux/WSL
        targetPaths.push_back("/tmp");
        targetPaths.push_back("/var/tmp");
        // Добавляем кэш пакетных менеджеров и прочие пути
    }
    // Если указан флаг cleanWindows (например, при запуске под WSL)
    if (config.cleanWindows) {
        targetPaths.push_back("C:\\Windows\\SoftwareDistribution\\Download");
        targetPaths.push_back("C:\\Windows\\Panther");
        targetPaths.push_back("C:\\Windows\\WER");
    }
    // Добавляем дополнительные пути, если они заданы в конфиге
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
    
    // Простая реализация с последовательной обработкой (параллелизм можно добавить с использованием std::thread)
    for (const auto &path : targetPaths) {
        processPath(path);
    }
}

/// Рекурсивная обработка одного пути
void Cleaner::processPath(const std::string &path) {
    // Преобразуем строку пути в реальный путь с помощью утилитарных функций
    if (!pathExists(path)) {
        LOG_DEBUG("Путь не существует: " + path);
        return;
    }
    
    // Используем std::filesystem для рекурсивного обхода
    try {
        for (auto &entry : fs::recursive_directory_iterator(path)) {
            std::string entryPath = entry.path().string();
            // Если не включаем скрытые файлы, можно добавить фильтрацию по атрибуту
            if (!config.includeHidden && entry.path().filename().string().front() == '.')
                continue;
            
            // При dry-run выводим информацию, иначе удаляем
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

/// Удаление файла или директории (учитывая dry-run)
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
