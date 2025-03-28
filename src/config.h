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
    bool verbose = false;          // Подробный вывод
    bool dryRun = false;           // Режим симуляции
    bool cleanWindows = false;     // Флаг принудительной очистки Windows-директорий (при запуске под WSL)
    bool includeHidden = false;    // Обрабатывать скрытые файлы/папки
    OS_TYPE targetOS = OS_TYPE::AUTO; // Целевая ОС (AUTO, WINDOWS или LINUX)
    std::vector<std::string> additionalPaths; // Дополнительные пути для очистки
};

/// Функция для парсинга аргументов командной строки
Config parseArguments(int argc, char* argv[]);

/// Функция для загрузки конфигурации из файла (пример: config.cfg)
bool loadConfigFromFile(const std::string &filePath, Config &config);

#endif // CONFIG_H
