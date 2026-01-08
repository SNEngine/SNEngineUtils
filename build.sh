#!/bin/bash
# Скрипт для сборки проекта SNEngineUtils

echo "Сборка SNEngineUtils для Linux..."

# Проверка наличия необходимых инструментов
if ! command -v cmake &> /dev/null; then
    echo "CMake не найден. Установите CMake."
    exit 1
fi

if ! command -v g++ &> /dev/null; then
    echo "g++ не найден. Установите g++."
    exit 1
fi

# Создание директории для сборки
mkdir -p build
cd build

# Конфигурация проекта с помощью CMake
cmake ..

# Компиляция проекта
if make; then
    echo "Сборка завершена успешно!"

    # Создание директории Linux и копирование результатов
    mkdir -p ../Linux
    cp SNEngine_Cleaner ../Linux/
    cp SNEngine_Symbols ../Linux/
    cp ../cleanup_list.txt ../Linux/

    # Установка прав доступа на исполняемые файлы
    chmod +x ../Linux/SNEngine_Cleaner ../Linux/SNEngine_Symbols

    # Очистка промежуточных файлов, оставляя только исполняемые и txt файл
    find . -maxdepth 1 -type f ! -name 'SNEngine_Cleaner' ! -name 'SNEngine_Symbols' ! -name 'cleanup_list.txt' -delete
    rm -rf CMakeFiles/

    echo "Файлы скопированы в директорию Linux"
    echo "Созданные файлы в Linux директории:"
    echo "- SNEngine_Cleaner"
    echo "- SNEngine_Symbols"
    echo "- cleanup_list.txt"
else
    echo "Ошибка при сборке"
    exit 1
fi