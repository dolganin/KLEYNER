#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <filesystem>

/// Разворачивание переменных окружения и тильды в пути
std::string expandPath(const std::string &path);

/// Проверка существования файла или директории
bool pathExists(const std::string &path);

/// Получение списка файлов в заданной директории
std::vector<std::filesystem::path> listFiles(const std::string &directory);

/// Подсчёт размера директории (в байтах)
std::uintmax_t directorySize(const std::filesystem::path &path);

bool isWSL();

#endif // UTILS_H
