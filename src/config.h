#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>

// Возможные типы целевой ОС
enum class OS_TYPE {
    AUTO,
    WINDOWS,
    LINUX
};

// Структура конфигурации для утилиты
struct Config {
    bool verbose = false;           // Подробный вывод
    bool dryRun = false;            // Режим симуляции (ничего не удаляется, только лог)
    bool cleanWindows = false;      // Флаг принудительной очистки Windows-директорий (в WSL)
    bool includeHidden = false;     // Обрабатывать скрытые файлы/папки
    bool wsl = false;               // Флаг WSL (если true, меняем пути на /mnt/c/... и т.д.)
    OS_TYPE targetOS = OS_TYPE::AUTO; // Целевая ОС (AUTO, WINDOWS или LINUX)
    std::vector<std::string> additionalPaths; // Дополнительные пути для очистки
    std::string configFile = "configs/basic.cfg"; // Путь к конфигурационному файлу
};

/// Функция для парсинга аргументов командной строки
Config parseArguments(int argc, char* argv[]);

/// Функция для загрузки конфигурации из файла (пример: config.cfg)
bool loadConfigFromFile(const std::string &filePath, Config &config);

#endif // CONFIG_H
