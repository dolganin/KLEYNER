#!/bin/bash

# Устанавливаем флаг выхода при ошибке
set -e

# Определяем имя выходного исполняемого файла
OUTPUT_EXECUTABLE="cleaner"
BUILD_DIR="build"
INSTALL_DIR="bin"

# Создаём директорию билда, если её нет
mkdir -p $BUILD_DIR
cd $BUILD_DIR

# Запускаем CMake и сборку
cmake ..
make -j$(nproc)

# Создаём директорию для бинарников, если её нет
mkdir -p ../$INSTALL_DIR

# Копируем исполняемый файл в bin
cp $OUTPUT_EXECUTABLE ../$INSTALL_DIR/

# Возвращаемся назад и удаляем папку билда
cd ..
rm -rf $BUILD_DIR

echo "Сборка завершена. Исполняемый файл находится в $INSTALL_DIR/$OUTPUT_EXECUTABLE"
