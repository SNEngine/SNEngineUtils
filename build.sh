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

    # Создание директории структуры для каждой утилиты с подкаталогами для платформ
    # Создание структуры для SNEngine_Cleaner
    mkdir -p ../SNEngine_Cleaner/Linux
    cp SNEngine_Cleaner ../SNEngine_Cleaner/Linux/
    cp ../cleanup_list.txt ../SNEngine_Cleaner/Linux/
    chmod +x ../SNEngine_Cleaner/Linux/SNEngine_Cleaner

    # Создание структуры для SNEngine_Symbols
    mkdir -p ../SNEngine_Symbols/Linux
    cp SNEngine_Symbols ../SNEngine_Symbols/Linux/
    chmod +x ../SNEngine_Symbols/Linux/SNEngine_Symbols

    # Создание структуры для SNEngine_Code_Counter
    mkdir -p ../SNEngine_Code_Counter/Linux
    cp SNEngine_Code_Counter ../SNEngine_Code_Counter/Linux/
    chmod +x ../SNEngine_Code_Counter/Linux/SNEngine_Code_Counter

    # Создание структуры для SNEngine_Novel_Counter
    mkdir -p ../SNEngine_Novel_Counter/Linux
    cp SNEngine_Novel_Counter ../SNEngine_Novel_Counter/Linux/
    chmod +x ../SNEngine_Novel_Counter/Linux/SNEngine_Novel_Counter

    # Очистка промежуточных файлов, оставляя только исполняемые и txt файл
    find . -maxdepth 1 -type f ! -name 'SNEngine_Cleaner' ! -name 'SNEngine_Symbols' ! -name 'SNEngine_Code_Counter' ! -name 'SNEngine_Novel_Counter' ! -name 'cleanup_list.txt' -delete
    rm -rf CMakeFiles/

    echo "Сборка завершена успешно!"
    echo "Создана следующая структура каталогов:"
    echo "- SNEngine_Cleaner/Linux/SNEngine_Cleaner (с cleanup_list.txt)"
    echo "- SNEngine_Symbols/Linux/SNEngine_Symbols"
    echo "- SNEngine_Code_Counter/Linux/SNEngine_Code_Counter"
    echo "- SNEngine_Novel_Counter/Linux/SNEngine_Novel_Counter"
else
    echo "Ошибка при сборке"
    exit 1
fi