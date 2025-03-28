#include "logger.h"
#include <iostream>
#include <chrono>
#include <ctime>

// Глобальная переменная для уровня логирования
static bool g_verbose = false;

/// Инициализация логгера
void initLogger(bool verbose) {
    g_verbose = verbose;
}

/// Вспомогательная функция для получения текущей временной метки
// Вспомогательная функция для получения текущей временной метки
static std::string currentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    char buffer[100];

    std::tm* tm = std::localtime(&now_time);
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm); // Форматируем как "2025-03-29 12:34:56"
    
    return std::string(buffer);
}


void LOG_INFO(const std::string &msg) {
    std::cout << "[" << currentTimestamp() << "][INFO] " << msg << std::endl;
}

void LOG_DEBUG(const std::string &msg) {
    if (g_verbose)
        std::cout << "[" << currentTimestamp() << "][DEBUG] " << msg << std::endl;
}

void LOG_ERROR(const std::string &msg) {
    std::cerr << "[" << currentTimestamp() << "][ERROR] " << msg << std::endl;
}

void LOG_WARNING(const std::string &msg) {
    std::string logMessage = "[" + currentTimestamp() + "][WARNING] " + msg;
    std::cout << logMessage << std::endl;
}
