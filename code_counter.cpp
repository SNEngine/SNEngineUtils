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

struct FileInfo {
    std::string name;
    size_t lines;
};

std::mutex result_mutex;
std::vector<FileInfo> all_results;

void process_file(const fs::path& file_path, std::atomic<size_t>& total_lines, std::atomic<uintmax_t>& total_bytes, bool collect_data) {
    std::ifstream file(file_path.string());
    if (!file.is_open()) return;

    total_bytes += fs::file_size(file_path);

    size_t lines = 0;
    std::string line;
    while (std::getline(file, line)) lines++;

    total_lines += lines;

    if (collect_data) {
        std::lock_guard<std::mutex> lock(result_mutex);
        all_results.push_back({file_path.filename().string(), lines});
    }
}

std::string format_size(uintmax_t bytes) {
    double size = static_cast<double>(bytes);
    const char* units[] = {"B", "KB", "MB", "GB"};
    int i = 0;
    while (size >= 1024 && i < 3) {
        size /= 1024;
        i++;
    }
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << size << " " << units[i];
    return ss.str();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: counter <directory_path> [--report]" << std::endl;
        return 1;
    }

    std::string target_path_str = argv[1];
    bool create_report = false;

    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--report") {
            create_report = true;
        }
    }

    fs::path target_path = target_path_str;
    if (!fs::exists(target_path)) {
        std::cerr << "Path not found: " << target_path_str << std::endl;
        return 1;
    }

    std::atomic<size_t> total_lines{0};
    std::atomic<size_t> file_count{0};
    std::atomic<uintmax_t> total_bytes{0};
    unsigned int num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) num_threads = 4;

    {
        ThreadPool pool(num_threads);
        for (const auto& entry : fs::recursive_directory_iterator(target_path)) {
            try {
                if (fs::is_regular_file(entry) && entry.path().extension() == ".cs") {
                    file_count++;
                    pool.enqueue([&total_lines, &total_bytes, path = entry.path(), create_report]() {
                        process_file(path, total_lines, total_bytes, create_report);
                    });
                }
            } catch (...) { continue; }
        }
    }

    size_t average = 0;
    if (file_count > 0) {
        average = static_cast<size_t>(std::round(static_cast<double>(total_lines) / file_count));
    }

    std::string total_size_str = format_size(total_bytes);

    if (create_report) {
        std::ofstream json_file("report.json");
        json_file << "{\n";
        json_file << "  \"summary\": {\n";
        json_file << "    \"total_files\": " << file_count << ",\n";
        json_file << "    \"total_lines\": " << total_lines << ",\n";
        json_file << "    \"average_lines\": " << average << ",\n";
        json_file << "    \"total_size_bytes\": " << total_bytes << ",\n";
        json_file << "    \"total_size_human\": \"" << total_size_str << "\"\n";
        json_file << "  },\n";
        json_file << "  \"details\": [\n";
        for (size_t i = 0; i < all_results.size(); ++i) {
            json_file << "    {\n";
            json_file << "      \"file\": \"" << all_results[i].name << "\",\n";
            json_file << "      \"lines\": " << all_results[i].lines << "\n";
            json_file << "    }" << (i == all_results.size() - 1 ? "" : ",") << "\n";
        }
        json_file << "  ]\n";
        json_file << "}";
        json_file.close();
    }

    std::cout << "Directory: " << target_path.string() << "\n";
    std::cout << "Files:     " << file_count << "\n";
    std::cout << "Lines:     " << total_lines << "\n";
    std::cout << "Average:   " << average << " lines per script\n";
    std::cout << "Size:      " << total_size_str << "\n";
    if (create_report) std::cout << "Report:    Generated (report.json)\n";

    return 0;
}