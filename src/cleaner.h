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

    void printPlan();
    
private:
    struct TargetGroup {
        std::string scope;
        std::string name;
        std::string pattern;
        std::vector<std::string> paths;
    };

    Config config;
    std::vector<TargetGroup> targets;
    std::vector<std::string> deniedPaths;
    
    /// Формирование списка путей для очистки на основе конфигурации
    void buildTargetPaths();
    
    /// Рекурсивная обработка одного пути
    void processPath(const std::string &path);
    
    /// Удаление файла или директории (с учётом dry-run)
    void deleteEntry(const std::string &path);

    void addTargetGroup(const std::string &scope, const PathEntry &entry, bool windowsPath);
    std::vector<std::string> resolvePattern(const std::string &path) const;
    void addDeniedPath(const std::string &path);
};

#endif // CLEANER_H
