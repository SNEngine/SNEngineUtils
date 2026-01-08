@echo off
echo Building SNEngineUtils for Windows...

REM Check for CMake
cmake --version >nul 2>&1
if %errorlevel% neq 0 (
    echo CMake not found. Please install CMake and add to PATH.
    goto error_exit
)

REM Check for compiler
g++ --version >nul 2>&1
if %errorlevel% neq 0 (
    echo g++ not found. Please ensure MinGW-w64 is installed and added to PATH.
    goto error_exit
)

REM Create build directory
if not exist build mkdir build
cd build

REM Configure project with CMake (specify MinGW generator)
cmake .. -G "MinGW Makefiles"

REM Compile project
if %errorlevel% equ 0 (
    mingw32-make
) else (
    echo CMake configuration error
    cd ..
    goto error_exit
)

REM Create Windows directory and copy results
cd ..
if not exist Windows mkdir Windows
copy build\SNEngine_Cleaner.exe Windows\ >nul
copy build\SNEngine_Symbols.exe Windows\ >nul
copy cleanup_list.txt Windows\ >nul

REM Clean up build directory, keeping only executables and txt file
cd build
copy ..\cleanup_list.txt . >nul
del /q /f CMakeCache.txt cmake_install.cmake Makefile CMakeConfigureLog.yaml cmake.check_cache
del /q /f Makefile2 Makefile.cmake CMakeDirectoryInformation.cmake InstallScripts.json TargetDirectories.txt progress.marks
rd /s /q CMakeFiles
rd /s /q pkgRedirects
del /q /f libtinyfiledialogs.a
cd ..

echo Build completed!
echo Created files in Windows directory:
echo - SNEngine_Cleaner.exe
echo - SNEngine_Symbols.exe
echo - cleanup_list.txt
goto end

:error_exit
echo An error occurred!
pause
exit /b 1

:end
REM Just wait for a keypress without displaying Russian message
ping -n 1 127.0.0.1 > nul