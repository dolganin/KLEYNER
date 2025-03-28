#include "config.h"
#include "logger.h"
#include "cleaner.h"

#include <iostream>

int main(int argc, char* argv[]) {
    // Парсим параметры командной строки, в том числе --config и --wsl
    Config config = parseArguments(argc, argv);
    
    // Если путь к конфигу не указан, используем значение по умолчанию
    std::string configFile = config.configFile.empty() ? "configs/basic.cfg" : config.configFile;
    
    if (!loadConfigFromFile(configFile, config)) {
        LOG_ERROR("Не удалось загрузить файл конфигурации " + configFile + ", продолжаем с параметрами по умолчанию.");
    }
    
    // Инициализируем логирование согласно настройке verbose
    initLogger(config.verbose);
    LOG_INFO("Запуск утилиты очистки");
    
    // Создаём объект очистки и запускаем процесс
    Cleaner cleaner(config);
    cleaner.run();
    
    LOG_INFO("Работа утилиты завершена");
    return 0;
}
