#include <fstream>
#include <vector>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

void clear_directory(const fs::path& p) {
    if (fs::exists(p) && fs::is_directory(p)) {
        for (const auto& entry : fs::directory_iterator(p)) {
            try {
                fs::remove_all(entry.path());
            } catch (...) {}
        }
    }
}

int main(int argc, char* argv[]) {
    // Если Unity передает путь аргументом — берем его, иначе работаем в текущей папке
    fs::path projectRoot = (argc > 1) ? argv[1] : fs::current_path();
    
    // Путь к конфигу рядом с самим exe
    fs::path exeDir = fs::path(argv[0]).parent_path();
    fs::path configPath = exeDir / "cleanup_list.txt";

    if (!fs::exists(configPath)) return 1;

    std::ifstream configFile(configPath.string());
    std::string line;

    while (std::getline(configFile, line)) {
        if (line.empty() || line[0] == '#') continue;

        size_t sep = line.find(':');
        if (sep == std::string::npos) continue;

        std::string action = line.substr(0, sep);
        std::string relPath = line.substr(sep + 1);

        // Убираем лишние пробелы и символы переноса
        if (!relPath.empty() && (relPath.back() == '\r' || relPath.back() == ' ')) relPath.pop_back();

        fs::path fullPath = projectRoot / relPath;
        if (!fs::exists(fullPath)) continue;

        try {
            if (action == "DELETE") {
                fs::remove_all(fullPath);
            } 
            else if (action == "CLEAR") {
                clear_directory(fullPath);
            }
        } catch (...) {}
    }

    // Жестко прописанная логика для WebGLTemplates
    fs::path webglRoot = projectRoot / "Assets/WebGLTemplates";
    if (fs::exists(webglRoot)) {
        for (const auto& entry : fs::directory_iterator(webglRoot)) {
            if (entry.path().filename() != "SNEngine") {
                try { fs::remove_all(entry.path()); } catch (...) {}
            }
        }
    }

    return 0;
}