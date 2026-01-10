# SNEngineUtils

A collection of lightweight C++ utilities designed to automate Unity project configuration and cleanup for SNEngine. These tools replace heavier Python scripts with standalone, fast executables.

## Tools

### 1. SNEngine Symbols Setup
Updates `ProjectSettings.asset` to include required Scripting Define Symbols across all target platforms.
- **Symbols added:** `UNITASK_DOTWEEN_SUPPORT`, `SNENGINE_SUPPORT`, `DOTWEEN`.
- **Interface:** Native folder selection dialog (Windows/Linux) or command-line execution.

### 2. SNEngine Project Cleaner
A background utility for post-build or pre-setup cleanup.
- **Functionality:** Deletes redundant assets, clears specific resource folders, and manages WebGL templates.
- **Configurable:** Uses `cleanup_list.txt` to define paths for deletion or clearing.
- **Silent Mode:** Runs without a console window (ideal for Unity background processes).

### 3. SNEngine Code Counter
A code line counter utility that counts lines in .cs files within a directory.
- **Functionality:** Recursively scans directories for .cs files and counts lines
- **Output:** Shows total files, lines, average lines per file, and total size
- **Optional:** Generate JSON report with `--report` flag

### 4. SNEngine Novel Counter
A novel project analyzer that analyzes SNEngine dialogue and character files.
- **Functionality:** Analyzes .asset files in Dialogues and Characters folders
- **Output:** Shows character count, nodes, dialogues, sentences, character count, wait times, and estimated playtime
- **Optional:** Generate JSON report with `--json` flag

## Performance & Portability
- **Standalone:** No Python or external runtimes required.
- **Compact:** Optimized for size (< 1MB).
- **Cross-platform:** Built with C++17 and native system calls.

## How to Build
Use CMake to build all utilities at once:

```bash
mkdir build
cd build
cmake ..
make
```

On Windows with MinGW:

```cmd
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
mingw32-make
```

Or use the provided build scripts:

**Windows:**
```cmd
build.bat
```

**Linux:**
```bash
./build.sh
```

The build scripts will create a directory structure where each utility has its own folder with subdirectories for each platform:
- SNEngine_Cleaner/Windows/SNEngine_Cleaner.exe (with cleanup_list.txt)
- SNEngine_Symbols/Windows/SNEngine_Symbols.exe
- SNEngine_Code_Counter/Windows/SNEngine_Code_Counter.exe
- SNEngine_Novel_Counter/Windows/SNEngine_Novel_Counter.exe

And for Linux:
- SNEngine_Cleaner/Linux/SNEngine_Cleaner (with cleanup_list.txt)
- SNEngine_Symbols/Linux/SNEngine_Symbols
- SNEngine_Code_Counter/Linux/SNEngine_Code_Counter
- SNEngine_Novel_Counter/Linux/SNEngine_Novel_Counter

Use `g++` (MinGW) for Windows (legacy method):

```cmd
# Build Symbols Setup (with UI)
g++ -Os -s -static tinyfiledialogs.c presetup.cpp -o SNEngine_Symbols.exe -lcomdlg32 -lole32

# Build Cleaner (Silent Mode)
g++ -Os -s -static cleaner.cpp -o SNEngine_Cleaner.exe -mwindows
```

## Usage

### SNEngine Code Counter
```bash
./SNEngine_Code_Counter <directory_path> [--report]
```

The `--report` flag generates a JSON report in `report.json`.

### SNEngine Novel Counter
```bash
./SNEngine_Novel_Counter <directory_path> [--json <output.json>]
```

The `--json` flag generates a JSON report to the specified file.