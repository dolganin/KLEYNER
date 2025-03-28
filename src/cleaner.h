#ifndef CLEANER_H
#define CLEANER_H

#include "config.h"
#include <string>
#include <vector>
#include <tuple>

/// Класс, реализующий логику очистки
class Cleaner {
public:
    explicit Cleaner(const Config &config);
    
    /// Запуск процесса очистки
    void run();

    /// Подсчет количества файлов, папок и общего размера перед удалением
    std::tuple<size_t, size_t, double> countItemsToDelete();
    
private:
    Config config;
    std::vector<std::string> targetPaths;
    
    /// Формирование списка путей для очистки на основе конфигурации
    void buildTargetPaths();
    
    /// Рекурсивная обработка одного пути
    void processPath(const std::string &path);
    
    /// Удаление файла или директории (с учётом dry-run)
    void deleteEntry(const std::string &path);
};

#endif // CLEANER_H
