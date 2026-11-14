// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace phone_monitor::fps {

struct SelectionPolicy {
    std::vector<std::string> include;
    std::vector<std::string> exclude;
    bool foreground_only = true;
};

struct HistoryPolicy {
    std::uint64_t recent_window_ms = 1500;
    std::uint64_t history_window_ms = 30000;
    std::uint64_t stream_stale_ms = 2500;
    std::size_t minimum_samples = 3;
    std::size_t max_streams = 64;
    std::size_t max_frames_per_stream = 120000;
};

struct BridgeConfig {
    std::filesystem::path presentmon;
    std::filesystem::path state_file;
    std::wstring session_name;
    std::uint64_t update_interval_ms = 1000;
    std::uint64_t unload_grace_ms = 30000;
    SelectionPolicy selection;
    HistoryPolicy history;
};

// Base settings are required. A local override can change individual keys without editing defaults.
BridgeConfig load_config(const std::filesystem::path &base_file,
                         const std::filesystem::path &local_file);

} // namespace phone_monitor::fps
