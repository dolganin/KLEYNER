#include "config.h"
#include "logger.h"
#include "cleaner.h"

#include <iostream>

int main(int argc, char* argv[]) {
    // Парсим параметры командной строки и загружаем конфигурационный файл (config.cfg)
    Config config = parseArguments(argc, argv);
    if (!loadConfigFromFile("configs/basic.cfg", config)) {
        LOG_ERROR("Не удалось загрузить файл конфигурации, продолжаем с параметрами по умолчанию.");
    }

    // Инициализируем логирование
    initLogger(config.verbose);
    LOG_INFO("Запуск утилиты очистки");

    // Создаём объект очистки и запускаем процесс
    Cleaner cleaner(config);
    cleaner.run();

    LOG_INFO("Работа утилиты завершена");
    return 0;
}
