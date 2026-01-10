#include <iostream>
#include <fstream>
#include "filesystem.hpp"
#include <vector>
#include <string>
#include <atomic>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <algorithm>
#include <cmath>
#include <iomanip>

namespace fs = ghc::filesystem;

struct NovelStats {
    std::atomic<size_t> total_nodes{0};
    std::atomic<size_t> dialogue_nodes{0};
    std::atomic<size_t> total_chars{0};
    std::atomic<size_t> total_sentences{0};
    std::atomic<double> total_wait_seconds{0.0};
};

class ThreadPool {
public:
    ThreadPool(size_t threads) : stop(false) {
        for (size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this] {
                for (;;) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });
                        if (this->stop && this->tasks.empty()) return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    task();
                }
            });
        }
    }
    void enqueue(std::function<void()> task) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            tasks.emplace(std::move(task));
        }
        condition.notify_one();
    }
    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread& worker : workers) worker.join();
    }
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};

void process_text_content(const std::string& text, NovelStats& stats) {
    if (text.empty() || text == "[]") return;
    size_t actual_length = 0;
    for (size_t i = 0; i < text.length(); ++i) {
        if (text[i] == '\\' && i + 5 < text.length() && text[i+1] == 'u') {
            actual_length++;
            i += 5;
        } else {
            actual_length++;
        }
    }
    stats.dialogue_nodes++;
    stats.total_chars += actual_length;
    bool in_dot_sequence = false;
    for (char c : text) {
        if (c == '.' || c == '!' || c == '?') {
            if (!in_dot_sequence) {
                stats.total_sentences++;
                in_dot_sequence = true;
            }
        } else {
            in_dot_sequence = false;
        }
    }
}

void process_dialogue_file(const fs::path& path, NovelStats& stats) {
    std::ifstream file(path.string());
    if (!file.is_open()) return;
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("--- !u!114 &") != std::string::npos) {
            stats.total_nodes++;
            continue;
        }
        auto w_pos = line.find("_seconds: ");
        if (w_pos != std::string::npos) {
            try { 
                double val = std::stod(line.substr(w_pos + 10));
                double current = stats.total_wait_seconds.load();
                while(!stats.total_wait_seconds.compare_exchange_weak(current, current + val));
            } catch(...) {}
            continue;
        }
        auto t_pos = line.find("_text: ");
        if (t_pos != std::string::npos) {
            size_t key_indent = line.find_first_not_of(" ");
            std::string content = line.substr(t_pos + 7);
            std::string full_text;
            if (!content.empty() && (content[0] == '"' || content[0] == '\'')) {
                char q = content[0];
                full_text = content.substr(1);
                if (full_text.find(q) == std::string::npos) {
                    std::string next_line;
                    while (std::getline(file, next_line)) {
                        full_text += " " + next_line;
                        if (next_line.find(q) != std::string::npos) break;
                    }
                }
                size_t last_q = full_text.find_last_of(q);
                if (last_q != std::string::npos) full_text = full_text.substr(0, last_q);
            } else {
                full_text = content;
                std::streampos current_pos = file.tellg();
                std::string next_line;
                while (std::getline(file, next_line)) {
                    if (next_line.empty()) {
                        current_pos = file.tellg();
                        continue;
                    }
                    size_t next_indent = next_line.find_first_not_of(" ");
                    if (next_indent != std::string::npos && next_indent <= key_indent) {
                        file.seekg(current_pos);
                        break;
                    }
                    if (next_line.find("---") != std::string::npos) {
                        file.seekg(current_pos);
                        break;
                    }
                    full_text += " " + next_line;
                    current_pos = file.tellg();
                }
            }
            process_text_content(full_text, stats);
        }
    }
}

int main(int argc, char* argv[]) {
    std::string root_path = "";
    std::string json_out = "";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--json" && i + 1 < argc) {
            json_out = argv[++i];
        } else if (arg[0] != '-') {
            root_path = arg;
        }
    }

    if (root_path.empty()) {
        std::cout << "Usage: novel_counter <path> [--json <output.json>]" << std::endl;
        return 1;
    }

    fs::path root = root_path;
    fs::path diag_path, char_path;
    bool d_f = false, c_f = false;

    for (const auto& entry : fs::recursive_directory_iterator(root)) {
        if (entry.path().filename() == "Dialogues") { diag_path = entry.path(); d_f = true; }
        if (entry.path().filename() == "Characters") { char_path = entry.path(); c_f = true; }
        if (d_f && c_f) break;
    }

    if (!d_f) { std::cerr << "Error: No Dialogues folder!" << std::endl; return 1; }

    size_t dialogue_graphs = 0;
    std::vector<fs::path> assets_to_process;
    for (const auto& entry : fs::recursive_directory_iterator(diag_path)) {
        if (entry.path().extension() == ".asset") {
            dialogue_graphs++;
            assets_to_process.push_back(entry.path());
        }
    }

    NovelStats stats;
    unsigned int threads = std::thread::hardware_concurrency();
    {
        ThreadPool pool(threads > 0 ? threads : 4);
        for (const auto& p : assets_to_process) {
            pool.enqueue([&stats, p]() { process_dialogue_file(p, stats); });
        }
    }

    size_t char_assets = 0;
    if (c_f) {
        for (const auto& entry : fs::directory_iterator(char_path)) {
            if (entry.path().extension() == ".asset") char_assets++;
        }
    }

    double playtime_mins = (stats.total_chars / 800.0 * 1.2) + (stats.total_wait_seconds / 60.0);
    size_t avg = (stats.total_sentences ? stats.total_chars / stats.total_sentences : 0);

    std::cout << "\n--- SNEngine Analytics ---" << std::endl;
    std::cout << "Dialogue Graphs: " << dialogue_graphs << std::endl;
    std::cout << "Characters:      " << char_assets << std::endl;
    std::cout << "Total Nodes:     " << stats.total_nodes << std::endl;
    std::cout << "Text Blocks:     " << stats.dialogue_nodes << std::endl;
    std::cout << "Chars (Unicode): " << stats.total_chars << std::endl;
    std::cout << "Playtime:        " << (int)playtime_mins / 60 << "h " << (int)playtime_mins % 60 << "m" << std::endl;

    if (!json_out.empty()) {
        std::ofstream jf(json_out);
        if (jf.is_open()) {
            jf << "{\n"
               << "  \"graphs\": " << dialogue_graphs << ",\n"
               << "  \"characters\": " << char_assets << ",\n"
               << "  \"nodes\": " << stats.total_nodes << ",\n"
               << "  \"dialogues\": " << stats.dialogue_nodes << ",\n"
               << "  \"chars\": " << stats.total_chars << ",\n"
               << "  \"estimated_playtime_minutes\": " << std::fixed << std::setprecision(2) << playtime_mins << "\n"
               << "}";
            std::cout << "Report saved to: " << json_out << std::endl;
        } else {
            std::cerr << "Error: Could not open " << json_out << " for writing!" << std::endl;
        }
    }

    return 0;
}