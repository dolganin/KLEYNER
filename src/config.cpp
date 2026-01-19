#include "config.h"
#include "logger.h"
#include "utils.h"
#include <fstream>
#include <algorithm>
#include <cctype>

/// Вспомогательная функция для обрезки пробелов
static std::string trim(const std::string& s) {
    if (s.empty()) return "";
    size_t first = 0;
    while (first < s.size() && std::isspace(static_cast<unsigned char>(s[first]))) first++;
    if (first == s.size()) return "";
    size_t last = s.size() - 1;
    while (last > first && std::isspace(static_cast<unsigned char>(s[last]))) last--;
    return s.substr(first, last - first + 1);
}

static std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}

static bool parseBool(const std::string &value) {
    std::string v = toLower(value);
    return v == "true" || v == "1" || v == "yes" || v == "on";
}

static std::vector<std::string> splitList(const std::string &value) {
    std::vector<std::string> parts;
    size_t start = 0;
    while (start < value.size()) {
        size_t comma = value.find(',', start);
        std::string part = (comma == std::string::npos)
            ? value.substr(start)
            : value.substr(start, comma - start);
        part = trim(part);
        if (!part.empty()) parts.push_back(part);
        if (comma == std::string::npos) break;
        start = comma + 1;
    }
    return parts;
}

static std::string stripComment(const std::string &line) {
    size_t hashPos = line.find('#');
    size_t semiPos = line.find(';');
    size_t cut = std::string::npos;
    if (hashPos != std::string::npos) cut = hashPos;
    if (semiPos != std::string::npos) cut = (cut == std::string::npos) ? semiPos : std::min(cut, semiPos);
    if (cut == std::string::npos) return line;
    return line.substr(0, cut);
}

static OS_TYPE parseOsValue(const std::string &value) {
    std::string v = toLower(value);
    if (v == "win" || v == "windows") return OS_TYPE::WINDOWS;
    if (v == "linux") return OS_TYPE::LINUX;
    if (v == "both" || v == "all") return OS_TYPE::BOTH;
    return OS_TYPE::AUTO;
}

/// Парсинг аргументов командной строки
Config parseArguments(int argc, char* argv[]) {
    Config config;
    config.wsl = isWSL();

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
        } else if (arg == "--wsl") {
            config.wsl = true;
            config.wslSet = true;
        } else if (arg == "--no-wsl") {
            config.wsl = false;
            config.wslSet = true;
        } else if (arg == "--allow-sudo" || arg == "--sudo") {
            config.allowSudo = true;
        } else if (arg == "--cli-clean") {
            config.cliClean = true;
        } else if (arg == "--docker-prune") {
            config.dockerPrune = true;
        } else if (arg == "--docker-prune-all") {
            config.dockerPrune = true;
            config.dockerPruneAll = true;
        } else if (arg == "--docker-prune-volumes") {
            config.dockerPrune = true;
            config.dockerPruneVolumes = true;
        } else if (arg == "--os") {
            if (i + 1 < argc) {
                std::string osArg = argv[++i];
                config.targetOS = parseOsValue(osArg);
            }
        } else if (arg == "--config") {
            if (i + 1 < argc) {
                config.configFile = argv[++i];
            }
        }
    }

    return config;
}

/// Загрузка конфигурации из файла (INI-подобный формат)
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
        line = stripComment(line);
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
        std::vector<std::string> values = splitList(value);

        // Обрабатываем общие параметры
        if (currentSection == "General") {
            if (key == "verbose")
                config.verbose = parseBool(value);
            else if (key == "dry_run")
                config.dryRun = parseBool(value);
            else if (key == "wsl") {
                config.wsl = parseBool(value);
                config.wslSet = true;
            } else if (key == "clean_windows")
                config.cleanWindows = parseBool(value);
            else if (key == "include_hidden")
                config.includeHidden = parseBool(value);
            else if (key == "os")
                config.targetOS = parseOsValue(value);
            else if (key == "allow_sudo")
                config.allowSudo = parseBool(value);
            else if (key == "cli_clean")
                config.cliClean = parseBool(value);
            else if (key == "docker_prune")
                config.dockerPrune = parseBool(value);
            else if (key == "docker_prune_all") {
                config.dockerPruneAll = parseBool(value);
                if (config.dockerPruneAll) config.dockerPrune = true;
            } else if (key == "docker_prune_volumes") {
                config.dockerPruneVolumes = parseBool(value);
                if (config.dockerPruneVolumes) config.dockerPrune = true;
            }
        } else if (currentSection == "Windows") {
            for (const auto &v : values) {
                if (!v.empty()) config.windowsPaths.push_back({key, v});
            }
        } else if (currentSection == "Linux") {
            for (const auto &v : values) {
                if (!v.empty()) config.linuxPaths.push_back({key, v});
            }
        } else if (currentSection == "Common") {
            for (const auto &v : values) {
                if (!v.empty()) config.commonPaths.push_back({key, v});
            }
        } else if (currentSection == "Paths") {
            for (const auto &v : values) {
                if (!v.empty()) config.additionalPaths.push_back(v);
            }
        }
    }

    return true;
}
