#include <fstream>
#include <vector>
#include <string>
#include <set>
#include <sstream>
#include <iostream>

// Cross-platform filesystem implementation
#ifdef _WIN32
    #include <windows.h>
    #include <shlwapi.h>
    #include <direct.h>
    #pragma comment(lib, "shlwapi.lib")
    #define GetCurrentDir _getcwd
#else
    #include <sys/stat.h>
    #include <dirent.h>
    #include <unistd.h>
    #include <cstring>  // Added for strcmp function
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
        path parent_path() const {
            size_t pos = p.find_last_of("/\\");
            if (pos == std::string::npos) return path("");
            return path(p.substr(0, pos));
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

    inline bool is_directory(const path& p) {
#ifdef _WIN32
        DWORD attributes = GetFileAttributesA(p.string().c_str());
        return (attributes != INVALID_FILE_ATTRIBUTES) && 
               (attributes & FILE_ATTRIBUTE_DIRECTORY);
#else
        struct stat info;
        if (stat(p.string().c_str(), &info) != 0) return false;
        return (info.st_mode & S_IFDIR) != 0;
#endif
    }

    inline bool is_empty(const path& p) {
#ifdef _WIN32
        WIN32_FIND_DATAA findData;
        HANDLE hFind = FindFirstFileA((p.string() + "\\*").c_str(), &findData);
        if (hFind == INVALID_HANDLE_VALUE) return false;
        
        bool isEmpty = true;
        do {
            if (strcmp(findData.cFileName, ".") != 0 && 
                strcmp(findData.cFileName, "..") != 0) {
                isEmpty = false;
                break;
            }
        } while (FindNextFileA(hFind, &findData));
        
        FindClose(hFind);
        return isEmpty;
#else
        DIR* dir = opendir(p.string().c_str());
        if (dir == nullptr) return false;
        
        struct dirent* entry;
        int count = 0;
        while ((entry = readdir(dir)) != nullptr) {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                count++;
                break;
            }
        }
        closedir(dir);
        return count == 0;
#endif
    }

    inline bool remove(const path& p) {
#ifdef _WIN32
        if (is_directory(p)) {
            return RemoveDirectoryA(p.string().c_str()) != 0;
        } else {
            return DeleteFileA(p.string().c_str()) != 0;
        }
#else
        return remove(p.string().c_str()) == 0;
#endif
    }

    inline bool remove_all(const path& p) {
#ifdef _WIN32
        if (is_directory(p)) {
            // For directories on Windows, use robocopy method to handle locked files
            std::string tempDir = p.string() + "_temp_delete";
            std::string cmd = "mkdir \"" + tempDir + "\" >nul 2>&1 && robocopy \"" + tempDir + "\" \"" + p.string() + "\" /MIR >nul 2>&1 && rmdir \"" + tempDir + "\" >nul 2>&1";
            int result = system(cmd.c_str());
            // If robocopy fails, fallback to rd command
            if (result != 0) {
                cmd = "rd /s /q \"" + p.string() + "\" 2>nul";
                result = system(cmd.c_str());
            }
            return result == 0;
        } else {
            // For individual files on Windows, try multiple methods
            // First try standard DeleteFile
            if (DeleteFileA(p.string().c_str()) != 0) {
                return true;
            }

            // If that fails, try moving to temp location and deleting from there
            char tempPath[MAX_PATH];
            GetTempPathA(MAX_PATH, tempPath);
            std::string tempFile = std::string(tempPath) + p.filename();

            if (MoveFileA(p.string().c_str(), tempFile.c_str()) != 0) {
                return DeleteFileA(tempFile.c_str()) != 0;
            }

            // Last resort: try system del command
            std::string cmd = "del /f /q \"" + p.string() + "\" 2>nul";
            int result = system(cmd.c_str());
            return result == 0;
        }
#else
        std::string cmd = "rm -rf \"" + p.string() + "\"";
        int result = system(cmd.c_str());
        return result == 0;
#endif
    }

    inline path current_path() {
        char buffer[4096];
        if (GetCurrentDir(buffer, sizeof(buffer)) != nullptr) {
            return path(buffer);
        }
        return path("");
    }

    // Directory iterator - simplified implementation
    class directory_iterator {
    public:
        struct directory_entry {
            path p;
            directory_entry(const path& path) : p(path) {}
            const path& get_path() const { return p; }  // Changed name to avoid conflict
        };
        
    private:
        std::vector<directory_entry> entries;
        size_t index;
        bool valid;
        
    public:
        directory_iterator(const path& p) : index(0), valid(true) {
#ifdef _WIN32
            WIN32_FIND_DATAA findData;
            HANDLE hFind = FindFirstFileA((p.string() + "\\*").c_str(), &findData);
            if (hFind != INVALID_HANDLE_VALUE) {
                do {
                    if (strcmp(findData.cFileName, ".") != 0 && 
                        strcmp(findData.cFileName, "..") != 0) {
                        entries.push_back(directory_entry(p / findData.cFileName));
                    }
                } while (FindNextFileA(hFind, &findData));
                
                FindClose(hFind);
            }
#else
            DIR* dir = opendir(p.string().c_str());
            if (dir != nullptr) {
                struct dirent* entry;
                while ((entry = readdir(dir)) != nullptr) {
                    if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                        entries.push_back(directory_entry(p / entry->d_name));
                    }
                }
                closedir(dir);
            }
#endif
        }
        
        directory_iterator() : index(0), valid(false) {}
        
        directory_entry& operator*() {
            return entries[index];
        }
        
        directory_iterator& operator++() {
            ++index;
            if (index >= entries.size()) {
                valid = false;
            }
            return *this;
        }
        
        bool operator!=(const directory_iterator& other) const {
            return valid != other.valid;
        }
    };
}

void delete_with_exceptions(const fs::path& p, const std::set<std::string>& exceptions) {
    if (!fs::exists(p)) return;

    if (fs::is_directory(p)) {
        fs::directory_iterator iter(p);
        while (iter != fs::directory_iterator()) {
            auto& entry = *iter;
            if (exceptions.count(entry.get_path().filename())) {
                ++iter;
                continue;
            }
            try {
                fs::remove_all(entry.get_path());
            } catch (...) {}
            ++iter;
        }

        try {
            if (fs::is_empty(p)) {
                fs::remove(p);
            }
        } catch (...) {}
    } else {
        try {
            fs::remove(p);
        } catch (...) {}
    }
}

void clear_directory(const fs::path& p) {
    if (fs::exists(p) && fs::is_directory(p)) {
        fs::directory_iterator iter(p);
        while (iter != fs::directory_iterator()) {
            auto& entry = *iter;
            try {
                fs::remove_all(entry.get_path());
            } catch (...) {}
            ++iter;
        }
    }
}

int main(int argc, char* argv[]) {
    fs::path projectRoot = (argc > 1) ? argv[1] : fs::current_path();
    fs::path exeDir = fs::path(argv[0]).parent_path();
    fs::path configPath = exeDir / "cleanup_list.txt";

    if (!fs::exists(configPath)) return 1;

    std::ifstream configFile(configPath.string());
    std::string line;

    while (std::getline(configFile, line)) {
        if (line.empty() || line[0] == '#') continue;

        size_t actionSep = line.find(':');
        if (actionSep == std::string::npos) continue;

        std::string action = line.substr(0, actionSep);
        std::string pathData = line.substr(actionSep + 1);

        size_t exSep = pathData.find('|');
        std::string relPath = pathData.substr(0, exSep);
        std::set<std::string> exceptions;

        if (exSep != std::string::npos) {
            std::string exString = pathData.substr(exSep + 1);
            std::stringstream ss(exString);
            std::string segment;
            while (std::getline(ss, segment, ',')) {
                while (!segment.empty() && (segment.back() == '\r' || segment.back() == '\n' || segment.back() == ' ')) {
                    segment.pop_back();
                }
                if (!segment.empty()) exceptions.insert(segment);
            }
        }

        while (!relPath.empty() && (relPath.back() == '\r' || relPath.back() == '\n' || relPath.back() == ' ')) {
            relPath.pop_back();
        }

        fs::path fullPath = projectRoot / relPath;
        if (!fs::exists(fullPath)) continue;

        try {
            if (action == "DELETE") {
                if (!exceptions.empty()) {
                    delete_with_exceptions(fullPath, exceptions);
                } else {
                    fs::remove_all(fullPath);
                }
            }
            else if (action == "CLEAR") {
                clear_directory(fullPath);
            }
        } catch (...) {}
    }

    fs::path webglRoot = projectRoot / "Assets/WebGLTemplates";
    if (fs::exists(webglRoot)) {
        fs::directory_iterator iter(webglRoot);
        while (iter != fs::directory_iterator()) {
            auto& entry = *iter;
            if (entry.get_path().filename() != "SNEngine") {
                try {
                    fs::remove_all(entry.get_path());
                } catch (...) {}
            }
            ++iter;
        }
    }

    return 0;
}