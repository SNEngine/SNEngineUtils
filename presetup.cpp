#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>

#ifdef _WIN32
    #include <windows.h>
    #include <shlwapi.h>
    #include <direct.h>
    #pragma comment(lib, "shlwapi.lib")
    #define GetCurrentDir _getcwd
#else
    #include <sys/stat.h>
    #include <unistd.h>
    #define GetCurrentDir getcwd
#endif

namespace fs {
    class path {
    private:
        std::string p;
    public:
        path(const std::string& s) : p(s) {}
        path(const char* s) : p(s ? s : "") {}
        path operator/(const std::string& other) const {
            if (p.empty()) return path(other);
            if (other.empty()) return path(p);
#ifdef _WIN32
            char sep = '\\';
#else
            char sep = '/';
#endif
            return path(p + sep + other);
        }
        const std::string& string() const { return p; }
        std::string filename() const {
            size_t pos = p.find_last_of("/\\");
            if (pos == std::string::npos) return p;
            return p.substr(pos + 1);
        }
    };

    inline bool exists(const path& p) {
#ifdef _WIN32
        return PathFileExistsA(p.string().c_str()) != FALSE;
#else
        struct stat info;
        return stat(p.string().c_str(), &info) == 0;
#endif
    }
}

extern "C" {
    #include "tinyfiledialogs.h"
}

#ifndef _WIN32
bool is_package_installed(const std::string& cmd) {
    std::string check_cmd = "command -v " + cmd + " > /dev/null 2>&1";
    return system(check_cmd.c_str()) == 0;
}
#endif

std::string trim(const std::string& s) {
    size_t first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, (last - first + 1));
}

void pause_program() {
#ifdef _WIN32
    system("pause > nul");
#else
    std::cout << "\nPress ENTER to continue...";
    std::cin.ignore();
#endif
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "        SNEngine Symbols Setup         " << std::endl;
    std::cout << "========================================" << std::endl;

#ifndef _WIN32
    if (!is_package_installed("zenity") && !is_package_installed("kdialog")) {
        std::cout << "[!] No graphical dialog backend found (zenity or kdialog)." << std::endl;
        std::cout << "[*] To fix this, run: sudo apt install zenity" << std::endl;
        std::cout << "[*] Continuing in console mode...\n" << std::endl;
    }
#endif

    std::cout << "[*] Opening folder selection dialog..." << std::endl;

    char const * pickedDir = tinyfd_selectFolderDialog("Select Unity Project Folder", NULL);

    if (!pickedDir) {
        std::cout << "[!] Selection cancelled." << std::endl;
        return 0;
    }

    fs::path projectRoot = pickedDir;
    fs::path targetFile = projectRoot / "ProjectSettings" / "ProjectSettings.asset";

    if (!fs::exists(targetFile)) {
        std::cerr << "[-] Error: Not a Unity project folder!" << std::endl;
        pause_program();
        return 1;
    }

    // Список символов 
    std::vector<std::string> symbols = {"UNITASK_DOTWEEN_SUPPORT", "SNENGINE_SUPPORT", "DOTWEEN"};

    std::ifstream inFile(targetFile.string());
    if (!inFile.is_open()) {
        std::cerr << "[-] Error: Could not open ProjectSettings.asset" << std::endl;
        return 1;
    }

    std::stringstream buffer;
    std::string line;
    bool modified = false;

    while (std::getline(inFile, line)) {
        if (line.find("Standalone:") != std::string::npos && line.find("scriptingDefineSymbols:") == std::string::npos) {
            size_t pos = line.find("Standalone:");
            std::string prefix = line.substr(0, pos + 11);
            std::string currentVal = trim(line.substr(pos + 11));

            for (const auto& sym : symbols) {
                if (currentVal.find(sym) == std::string::npos) {
                    if (!currentVal.empty() && currentVal.back() != ';') currentVal += ";";
                    currentVal += sym;
                    modified = true;
                }
            }
            buffer << prefix << " " << currentVal << "\n";
        } else {
            buffer << line << "\n";
        }
    }
    inFile.close();

    if (modified) {
        std::ofstream outFile(targetFile.string());
        outFile << buffer.str();
        std::cout << "[+] Success: Symbols updated." << std::endl;
    } else {
        std::cout << "[~] No changes: Symbols already present." << std::endl;
    }

    pause_program();
    return 0;
}