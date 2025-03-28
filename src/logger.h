#ifndef LOGGER_H
#define LOGGER_H

#include <string>

// Уровни логирования
enum LogLevel {
    INFO,
    DEBUG,
    ERROR
};

/// Инициализация логгера с учетом подробного режима (verbose)
void initLogger(bool verbose);

/// Логирование информационных сообщений
void LOG_INFO(const std::string &msg);

/// Логирование отладочных сообщений (выводятся, если включен verbose)
void LOG_DEBUG(const std::string &msg);

/// Логирование сообщений об ошибках
void LOG_ERROR(const std::string &msg);

#endif // LOGGER_H
