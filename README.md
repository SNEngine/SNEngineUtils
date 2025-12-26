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

## Performance & Portability
- **Standalone:** No Python or external runtimes required.
- **Compact:** Optimized for size (< 1MB).
- **Cross-platform:** Built with C++17 and native system calls.

## How to Build
Use `g++` (MinGW) for Windows:

```cmd
# Build Symbols Setup (with UI)
g++ -Os -s -static tinyfiledialogs.c presetup.cpp -o SNEngine_Symbols.exe -lcomdlg32 -lole32

# Build Cleaner (Silent Mode)
g++ -Os -s -static cleaner.cpp -o SNEngine_Cleaner.exe -mwindows