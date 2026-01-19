#include "config.h"
#include "logger.h"
#include "cleaner.h"
#include "utils.h"

#include <iostream>
#include <fstream>
#include <tuple>
#include <filesystem>
#include <vector>
#include <cstdlib>

// Функция для вывода пиксель-арта
void printPixelArt(const std::string& filePath) {
    std::ifstream artFile(filePath);
    if (!artFile) {
        LOG_WARNING("Не удалось загрузить ASCII-арт из " + filePath);
        return;
    }
    
    std::string line;
    while (std::getline(artFile, line)) {
        std::cout << line << std::endl;
    }
}

namespace fs = std::filesystem;

static std::vector<std::string> getPythonEnvRoots() {
    std::vector<std::string> roots;
#ifdef _WIN32
    roots = {
        "%USERPROFILE%\\.virtualenvs",
        "%USERPROFILE%\\Anaconda3\\envs",
        "%USERPROFILE%\\Miniconda3\\envs",
        "%USERPROFILE%\\.conda\\envs"
    };
#else
    roots = {
        "~/.virtualenvs",
        "~/anaconda3/envs",
        "~/miniconda3/envs",
        "~/.conda/envs",
        "~/.pyenv/versions"
    };
#endif
    return roots;
}

static std::vector<std::string> findPythonEnvironments() {
    std::vector<std::string> envs;
    for (auto r : getPythonEnvRoots()) {
        r = expandPath(r);
        if (!fs::exists(r)) continue;
        for (const auto &entry : fs::directory_iterator(r)) {
            if (entry.is_directory()) envs.push_back(entry.path().string());
        }
    }
    return envs;
}

static void handlePythonEnvironments(const Config &config) {
    LOG_INFO("Проверка Python-окружений...");
    const double threshold = 3.0; // GB
    for (const auto &env : findPythonEnvironments()) {
        auto size = directorySize(env);
        double sizeGb = static_cast<double>(size) / (1024 * 1024 * 1024);
        if (sizeGb < threshold) continue;

        std::cout << "Обнаружено Python окружение: " << env
                  << " (" << sizeGb << " GB). Удалить? (y/n): ";
        std::string ans;
        std::getline(std::cin, ans);
        if (ans == "y" || ans == "Y") {
            if (config.dryRun) {
                LOG_INFO("[Dry Run] Будет удалено окружение: " + env);
            } else {
                std::error_code ec;
                fs::remove_all(env, ec);
                if (ec)
                    LOG_WARNING("Ошибка удаления " + env + ": " + ec.message());
                else
                    LOG_INFO("Удалено окружение: " + env);
            }
        }
    }
}

static bool commandExists(const std::string &cmd) {
#ifdef _WIN32
    std::string check = "where " + cmd + " >nul 2>&1";
#else
    std::string check = "command -v " + cmd + " >/dev/null 2>&1";
#endif
    return std::system(check.c_str()) == 0;
}

static bool runCommand(const std::string &cmd, const Config &config) {
    if (config.dryRun) {
        LOG_INFO("[Dry Run] Команда: " + cmd);
        return true;
    }
    LOG_INFO("Команда: " + cmd);
    int code = std::system(cmd.c_str());
    if (code != 0) {
        LOG_WARNING("Команда завершилась с ошибкой: " + cmd);
        return false;
    }
    return true;
}

static void runCliCleaners(const Config &config) {
    if (!config.cliClean && !config.dockerPrune) return;

    LOG_INFO("CLI очистка:");
    bool ranAny = false;

    if (config.cliClean) {
        if (commandExists("pip")) {
            ranAny |= runCommand("pip cache purge", config);
        } else {
            LOG_INFO("pip не найден");
        }

        if (commandExists("npm")) {
            ranAny |= runCommand("npm cache clean --force", config);
        } else {
            LOG_INFO("npm не найден");
        }

        if (commandExists("yarn")) {
            ranAny |= runCommand("yarn cache clean", config);
        } else {
            LOG_INFO("yarn не найден");
        }

        if (commandExists("pnpm")) {
            ranAny |= runCommand("pnpm store prune", config);
        } else {
            LOG_INFO("pnpm не найден");
        }

        if (commandExists("go")) {
            ranAny |= runCommand("go clean -cache -testcache -modcache -fuzzcache", config);
        } else {
            LOG_INFO("go не найден");
        }

        if (commandExists("dotnet")) {
            ranAny |= runCommand("dotnet nuget locals http-cache --clear", config);
            ranAny |= runCommand("dotnet nuget locals global-packages --clear", config);
            ranAny |= runCommand("dotnet nuget locals temp --clear", config);
            ranAny |= runCommand("dotnet nuget locals plugins-cache --clear", config);
        } else if (commandExists("nuget")) {
            ranAny |= runCommand("nuget locals http-cache -clear", config);
            ranAny |= runCommand("nuget locals global-packages -clear", config);
            ranAny |= runCommand("nuget locals temp -clear", config);
            ranAny |= runCommand("nuget locals plugins-cache -clear", config);
        } else {
            LOG_INFO("dotnet/nuget не найден");
        }
    }

    if (config.dockerPrune) {
        if (commandExists("docker")) {
            std::string cmd = "docker system prune -f";
            if (config.dockerPruneAll) cmd += " -a";
            if (config.dockerPruneVolumes) cmd += " --volumes";
            ranAny |= runCommand(cmd, config);
        } else {
            LOG_INFO("docker не найден");
        }
    }

    if (!ranAny) {
        LOG_INFO("Нет доступных CLI команд для очистки");
    }
}

int main(int argc, char* argv[]) {
    std::cout << "KLEYNER Utility v1.0" << std::endl;
    printPixelArt("media/art.txt");

    Config config = parseArguments(argc, argv);
    
    std::string configFile = config.configFile.empty() ? "configs/basic.cfg" : config.configFile;
    
    if (!loadConfigFromFile(configFile, config)) {
        LOG_ERROR("Не удалось загрузить файл конфигурации " + configFile + ", продолжаем с параметрами по умолчанию.");
    }

    if (!config.wslSet) {
        config.wsl = isWSL();
    }
    if (config.targetOS == OS_TYPE::AUTO) {
#ifdef _WIN32
        config.targetOS = OS_TYPE::WINDOWS;
#else
        config.targetOS = OS_TYPE::LINUX;
#endif
    }
    
    initLogger(config.verbose);
    LOG_INFO("Запуск утилиты очистки");

    std::string mode;
    if (config.targetOS == OS_TYPE::WINDOWS) mode = "Windows";
    else if (config.targetOS == OS_TYPE::LINUX) mode = "Linux";
    else if (config.targetOS == OS_TYPE::BOTH) mode = "Windows + Linux";
    else mode = "Auto";
    LOG_INFO("Режим очистки: " + mode);
    if (config.wsl) LOG_INFO("Обнаружен WSL режим");
    
    Cleaner cleaner(config);

    cleaner.printPlan();
    auto [numFiles, numDirs, totalSize] = cleaner.countItemsToDelete();

    LOG_INFO("Будет удалено:");
    LOG_INFO("Файлов: " + std::to_string(numFiles));
    LOG_INFO("Папок: " + std::to_string(numDirs));
    LOG_INFO("Общий размер: " + std::to_string(totalSize) + " MB");

    std::string confirmation;
    std::cout << "Вы уверены, что хотите продолжить удаление? (y/n): ";
    std::getline(std::cin, confirmation);
    
    if (confirmation == "y" || confirmation == "Y") {
        cleaner.run();
        runCliCleaners(config);
        handlePythonEnvironments(config);
        LOG_INFO("Очистка завершена.");
    } else {
        LOG_INFO("Очистка отменена пользователем.");
    }

    LOG_INFO("Работа утилиты завершена");
    return 0;
}
