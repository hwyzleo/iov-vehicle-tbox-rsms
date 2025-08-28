//
// Created by hwyz_leo on 2025/8/6.
//
#include "spdlog/spdlog.h"
#include "utils.h"

#include "rsms_signal_cache.h"

RsmsSignalCache &RsmsSignalCache::get_instance() {
    static RsmsSignalCache instance;
    return instance;
}

bool RsmsSignalCache::start() {
    spdlog::info("启动国标信号缓存实例");
    if (!init()) {
        return false;
    }
    is_running_ = true;
    write_thread_ = std::thread(&RsmsSignalCache::write_file_thread, this);
    return false;
}

void RsmsSignalCache::stop() {
    spdlog::info("停止国标信号缓存实例");
    is_running_ = false;
    if (write_thread_.joinable()) {
        write_thread_.join();
    }
    write_file();
}

bool RsmsSignalCache::set_byte(const int &key, const uint8_t &value) {
    signal_map_[key] = std::to_string(value);
    return true;
}

bool RsmsSignalCache::get_byte(const int &key, uint8_t &out_value) {
    auto it = signal_map_.find(key);
    if (it != signal_map_.end()) {
        out_value = static_cast<uint8_t>(std::stoi(it->second));
        return true;
    }
    return false;
}

bool RsmsSignalCache::set_word(const int &key, const uint16_t &value) {
    signal_map_[key] = std::to_string(value);
    return true;
}

bool RsmsSignalCache::get_word(const int &key, uint16_t &out_value) {
    auto it = signal_map_.find(key);
    if (it != signal_map_.end()) {
        out_value = static_cast<uint16_t>(std::stoi(it->second));
        return true;
    }
    return false;
}

bool RsmsSignalCache::set_dword(const int &key, const uint32_t &value) {
    signal_map_[key] = std::to_string(value);
    return true;
}

bool RsmsSignalCache::get_dword(const int &key, uint32_t &out_value) {
    auto it = signal_map_.find(key);
    if (it != signal_map_.end()) {
        out_value = static_cast<uint32_t>(std::stoi(it->second));
        return true;
    }
    return false;
}

bool RsmsSignalCache::set_string(const int &key, const std::string &value) {
    signal_map_[key] = value;
    return true;
}

bool RsmsSignalCache::get_string(const int &key, std::string &out_value) {
    auto it = signal_map_.find(key);
    if (it != signal_map_.end()) {
        out_value = it->second;
        return true;
    }
    return false;
}

bool RsmsSignalCache::set_boolean(const int &key, const bool &value) {
    signal_map_[key] = value ? "1" : "0";
    return true;
}

bool RsmsSignalCache::get_boolean(const int &key, bool &out_value) {
    auto it = signal_map_.find(key);
    if (it != signal_map_.end()) {
        out_value = it->second == "1";
        return true;
    }
    return false;
}

bool RsmsSignalCache::init() {
    load_file_data();
    return true;
}

void RsmsSignalCache::load_file_data() {
    std::ifstream file(cache_file_path_);
    if (!file.is_open()) {
        // 文件不存在或无法打开，这是正常的初始状态
        spdlog::info("缓存文件[{}]不存在或无法打开，将创建新的缓存", cache_file_path_);
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        // 解析每行数据，格式为 key=value
        size_t delimiter_pos = line.find('=');
        if (delimiter_pos != std::string::npos) {
            try {
                // 提取键和值
                int key = std::stoi(line.substr(0, delimiter_pos));
                std::string value = line.substr(delimiter_pos + 1);

                // 存入信号映射表
                signal_map_[key] = value;
            } catch (const std::exception &e) {
                spdlog::warn("解析缓存文件行失败: {}", line);
            }
        }
    }

    spdlog::info("从缓存文件加载了 {} 个信号", signal_map_.size());
    file.close();
}

void RsmsSignalCache::write_file_thread() {
    while (is_running_) {
        std::this_thread::sleep_for(std::chrono::seconds(write_interval_seconds_));
        if (is_running_) {
            write_file();
        }
    }
}

void RsmsSignalCache::write_file() {
    std::string temp_path = cache_file_path_ + ".tmp";
    std::ofstream file(temp_path);
    if (!file.is_open()) {
        spdlog::error("创建临时文件[{}]失败", temp_path);
        return;
    }
    for (const auto &pair: signal_map_) {
        file << pair.first << "=" << pair.second << std::endl;
    }
    file.close();
    hwyz::Utils::rename_file(temp_path, cache_file_path_);
}
