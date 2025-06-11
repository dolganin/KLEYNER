#include "config.h"
#include "logger.h"
#include "cleaner.h"

#include <iostream>
#include <fstream>
#include <tuple>

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
    
    LOG_INFO("Работа утилиты завершена");
    return 0;
}
