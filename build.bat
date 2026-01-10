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

REM Return to parent directory after build
cd ..

REM Create utility directories with platform subdirectories and copy results

REM Create directory structure and copy SNEngine_Cleaner
if not exist SNEngine_Cleaner mkdir SNEngine_Cleaner
if not exist SNEngine_Cleaner\Windows mkdir SNEngine_Cleaner\Windows
copy build\SNEngine_Cleaner.exe SNEngine_Cleaner\Windows\ >nul
copy cleanup_list.txt SNEngine_Cleaner\Windows\ >nul

REM Create directory structure and copy SNEngine_Symbols
if not exist SNEngine_Symbols mkdir SNEngine_Symbols
if not exist SNEngine_Symbols\Windows mkdir SNEngine_Symbols\Windows
copy build\SNEngine_Symbols.exe SNEngine_Symbols\Windows\ >nul

REM Create directory structure and copy SNEngine_Code_Counter
if not exist SNEngine_Code_Counter mkdir SNEngine_Code_Counter
if not exist SNEngine_Code_Counter\Windows mkdir SNEngine_Code_Counter\Windows
copy build\SNEngine_Code_Counter.exe SNEngine_Code_Counter\Windows\ >nul

REM Create directory structure and copy SNEngine_Novel_Counter
if not exist SNEngine_Novel_Counter mkdir SNEngine_Novel_Counter
if not exist SNEngine_Novel_Counter\Windows mkdir SNEngine_Novel_Counter\Windows
copy build\SNEngine_Novel_Counter.exe SNEngine_Novel_Counter\Windows\ >nul

REM Clean up build directory, keeping only executables and txt file
cd build
del /q /f CMakeCache.txt cmake_install.cmake Makefile CMakeConfigureLog.yaml cmake.check_cache
del /q /f Makefile2 Makefile.cmake CMakeDirectoryInformation.cmake InstallScripts.json TargetDirectories.txt progress.marks
rd /s /q CMakeFiles
rd /s /q pkgRedirects
del /q /f libtinyfiledialogs.a
del /q /f *.obj *.cpp.obj *.c.obj
cd ..

echo Build completed!
echo Created directory structure:
echo - SNEngine_Cleaner\Windows\SNEngine_Cleaner.exe (with cleanup_list.txt)
echo - SNEngine_Symbols\Windows\SNEngine_Symbols.exe
echo - SNEngine_Code_Counter\Windows\SNEngine_Code_Counter.exe
echo - SNEngine_Novel_Counter\Windows\SNEngine_Novel_Counter.exe
goto end

:error_exit
echo An error occurred!
pause
exit /b 1

:end
REM Just wait for a keypress without displaying Russian message
ping -n 1 127.0.0.1 > nul