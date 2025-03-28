#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <filesystem>

/// Проверка существования файла или директории
bool pathExists(const std::string &path);

/// Получение списка файлов в заданной директории
std::vector<std::filesystem::path> listFiles(const std::string &directory);

#endif // UTILS_H