#include "config.h"
#include "logger.h"
#include "cleaner.h"
#include "utils.h"

#include <iostream>
#include <fstream>
#include <tuple>
#include <filesystem>
#include <vector>

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

static void pruneDockerImages() {
    LOG_INFO("Запуск docker image prune...");
    int code = std::system("docker image prune -a -f > /dev/null 2>&1");
    if (code != 0) {
        LOG_WARNING("Не удалось выполнить docker image prune");
    }
}

int main(int argc, char* argv[]) {
    // Вывод версии и ASCII-арта
    std::cout << "KLEYNER Utility v1.0" << std::endl;
    printPixelArt("media/art.txt");

    // Парсим параметры командной строки
    Config config = parseArguments(argc, argv);
    
    // Если путь к конфигу не указан, используем значение по умолчанию
    std::string configFile = config.configFile.empty() ? "configs/basic.cfg" : config.configFile;
    
    if (!loadConfigFromFile(configFile, config)) {
        LOG_ERROR("Не удалось загрузить файл конфигурации " + configFile + ", продолжаем с параметрами по умолчанию.");
    }
    
    // Инициализируем логирование согласно настройке verbose
    initLogger(config.verbose);
    LOG_INFO("Запуск утилиты очистки");
    
    // Создаём объект очистки
    Cleaner cleaner(config);

    // Подсчитываем количество файлов, папок и общий размер
    auto [numFiles, numDirs, totalSize] = cleaner.countItemsToDelete();

    // Выводим информацию о том, что будет удалено
    LOG_INFO("Будет удалено:");
    LOG_INFO("Файлов: " + std::to_string(numFiles));
    LOG_INFO("Папок: " + std::to_string(numDirs));
    LOG_INFO("Общий размер: " + std::to_string(totalSize) + " MB");

    cleaner.printSizeTree();

    // Запрашиваем подтверждение от пользователя перед удалением
    std::string confirmation;
    std::cout << "Вы уверены, что хотите продолжить удаление? (y/n): ";
    std::getline(std::cin, confirmation);
    
    if (confirmation == "y" || confirmation == "Y") {
        // Запускаем процесс очистки
        cleaner.run();
        LOG_INFO("Очистка завершена.");
    } else {
        LOG_INFO("Очистка отменена пользователем.");
    }

    // Дополнительные действия
    handlePythonEnvironments(config);
    pruneDockerImages();

    LOG_INFO("Работа утилиты завершена");
    return 0;
}
