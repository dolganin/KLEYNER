#include "config.h"
#include "logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>

// Вспомогательная функция для обрезки пробелов
static std::string trim(const std::string& s) {
    auto start = s.begin();
    while (start != s.end() && std::isspace(*start)) start++;
    auto end = s.end();
    do { end--; } while (std::distance(start, end) > 0 && std::isspace(*end));
    return std::string(start, end + 1);
}

/// Парсинг аргументов командной строки
Config parseArguments(int argc, char* argv[]) {
    Config config;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--verbose" || arg == "-v") {
            config.verbose = true;
        } else if (arg == "--dry-run") {
            config.dryRun = true;
        } else if (arg == "--clean-windows") {
            config.cleanWindows = true;
        } else if (arg == "--include-hidden") {
            config.includeHidden = true;
        } else if (arg == "--os") {
            if (i + 1 < argc) {
                std::string osArg = argv[++i];
                if (osArg == "win" || osArg == "windows")
                    config.targetOS = OS_TYPE::WINDOWS;
                else if (osArg == "linux")
                    config.targetOS = OS_TYPE::LINUX;
                else
                    config.targetOS = OS_TYPE::AUTO;
            }
        }
        // Можно добавить дополнительные параметры по необходимости
    }
    return config;
}

/// Загрузка конфигурации из файла формата .cfg (пример INI-подобного)
bool loadConfigFromFile(const std::string &filePath, Config &config) {
    std::ifstream inFile(filePath);
    if (!inFile) {
        LOG_ERROR("Не удалось открыть файл: " + filePath);
        return false;
    }

    std::string line;
    std::string currentSection;
    while (std::getline(inFile, line)) {
        // Удаляем комментарии и пустые строки
        auto commentPos = line.find('#');
        if (commentPos != std::string::npos)
            line = line.substr(0, commentPos);
        line = trim(line);
        if (line.empty()) continue;

        // Проверяем, если это секция
        if (line.front() == '[' && line.back() == ']') {
            currentSection = line.substr(1, line.size() - 2);
            continue;
        }

        // Парсим ключ=значение
        auto delimiterPos = line.find('=');
        if (delimiterPos == std::string::npos)
            continue;
        std::string key = trim(line.substr(0, delimiterPos));
        std::string value = trim(line.substr(delimiterPos + 1));

        // Обрабатываем общие параметры
        if (currentSection == "General") {
            if (key == "verbose")
                config.verbose = (value == "true");
            else if (key == "dry_run")
                config.dryRun = (value == "true");
            else if (key == "os") {
                if (value == "win" || value == "windows")
                    config.targetOS = OS_TYPE::WINDOWS;
                else if (value == "linux")
                    config.targetOS = OS_TYPE::LINUX;
                else
                    config.targetOS = OS_TYPE::AUTO;
            }
        }
        // Здесь можно расширять обработку других секций (Windows, Linux и т.п.)
    }
    return true;
}
