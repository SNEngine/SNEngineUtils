#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <sstream>

extern "C" {
    #include "tinyfiledialogs.h"
}

namespace fs = std::filesystem;

std::string trim(const std::string& s) {
    size_t first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, (last - first + 1));
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "        SNEngine Symbols Setup         " << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "[*] Opening folder selection dialog..." << std::endl;

    // Вызов диалогового окна выбора папки
    char const * pickedDir = tinyfd_selectFolderDialog("Select Unity Project Folder", NULL);
    
    if (!pickedDir) {
        std::cout << "[!] Selection cancelled." << std::endl;
        return 0;
    }

    fs::path projectRoot = pickedDir;
    fs::path targetFile = projectRoot / "ProjectSettings" / "ProjectSettings.asset";

    std::cout << "[*] Checking: " << targetFile.string() << std::endl;

    if (!fs::exists(targetFile)) {
        std::cerr << "[-] Error: Not a Unity project folder!" << std::endl;
        system("pause");
        return 1;
    }

    // Список символов для добавления
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
        // Ищем строку Standalone в блоке scriptingDefineSymbols
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

    std::cout << "\nPress any key to exit...";
    system("pause > nul");
    return 0;
}