// SPDX-License-Identifier: MIT
#include "fps/config.h"

#include <algorithm>
#include <charconv>
#include <fstream>
#include <map>
#include <stdexcept>

namespace phone_monitor::fps {
namespace {

using Settings = std::map<std::string, std::string>;

std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    return first == std::string::npos
               ? ""
               : value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
}

void read_settings(const std::filesystem::path &file, bool required, Settings &settings) {
    if (!std::filesystem::exists(file) && !required)
        return;
    std::ifstream input(file);
    if (!input)
        throw std::runtime_error("FPS configuration cannot be opened");
    std::string section, line;
    while (std::getline(input, line)) {
        if (line.substr(0, 3) == "\xef\xbb\xbf")
            line.erase(0, 3);
        line = trim(line);
        if (line.empty() || line.front() == '#' || line.front() == ';')
            continue;
        if (line.front() == '[' && line.back() == ']') {
            section = line.substr(1, line.size() - 2);
            continue;
        }
        const auto separator = line.find('=');
        if (separator == std::string::npos || section.empty())
            throw std::runtime_error("Invalid FPS configuration syntax");
        settings[section + '.' + trim(line.substr(0, separator))] =
            trim(line.substr(separator + 1));
    }
    if (!input.eof())
        throw std::runtime_error("FPS configuration read failed");
}

std::uint64_t number(Settings &settings, const std::string &key, std::uint64_t fallback,
                     std::uint64_t minimum, std::uint64_t maximum) {
    const auto entry = settings.find(key);
    if (entry == settings.end())
        return fallback;
    const auto text = entry->second;
    settings.erase(entry);
    std::uint64_t result = 0;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), result);
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size() || result < minimum ||
        result > maximum)
        throw std::runtime_error("Invalid FPS numeric setting");
    return result;
}

std::string text(Settings &settings, const std::string &key, std::string fallback) {
    const auto entry = settings.find(key);
    if (entry == settings.end())
        return fallback;
    auto result = entry->second;
    settings.erase(entry);
    return result;
}

std::vector<std::string> patterns(Settings &settings, const std::string &key) {
    std::vector<std::string> result;
    auto value = text(settings, key, "");
    std::size_t start = 0;
    while (start <= value.size()) {
        const auto end = value.find(';', start);
        auto pattern = trim(value.substr(start, end == std::string::npos ? end : end - start));
        if (!pattern.empty())
            result.push_back(std::move(pattern));
        if (end == std::string::npos)
            break;
        start = end + 1;
    }
    return result;
}

std::filesystem::path relative_path(const std::filesystem::path &directory,
                                    const std::string &value) {
    auto path = std::filesystem::u8path(value);
    if (path.empty())
        throw std::runtime_error("Empty FPS file path");
    return path.is_absolute() ? path : directory / path;
}

} // namespace

BridgeConfig load_config(const std::filesystem::path &base_file,
                         const std::filesystem::path &local_file) {
    Settings settings;
    // Local values overwrite matching base keys, including an explicitly empty filter.
    read_settings(base_file, true, settings);
    read_settings(local_file, false, settings);
    BridgeConfig result;
    result.presentmon = relative_path(base_file.parent_path(),
                                      text(settings, "Paths.PresentMon", "PresentMon.exe"));
    result.state_file =
        relative_path(base_file.parent_path(), text(settings, "Paths.StateFile", "fps-state.txt"));
    const auto session = text(settings, "Capture.SessionName", "RainmeterPhoneFPS");
    if (session.empty() || session.size() > 128 ||
        session.find_first_not_of(
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-") !=
            std::string::npos)
        throw std::runtime_error("Invalid FPS session name");
    result.session_name.assign(session.begin(), session.end());
    result.selection.include = patterns(settings, "Selection.Include");
    result.selection.exclude = patterns(settings, "Selection.Exclude");
    result.selection.foreground_only = number(settings, "Selection.ForegroundOnly", 1, 0, 1) != 0;
    result.update_interval_ms = number(settings, "Capture.UpdateIntervalMs", 1000, 100, 60000);
    result.unload_grace_ms = number(settings, "Capture.UnloadGraceMs", 30000, 1000, 600000);
    auto &history = result.history;
    history.recent_window_ms = number(settings, "History.RecentWindowMs", 1500, 100, 60000);
    history.history_window_ms =
        number(settings, "History.HistoryWindowMs", 30000, history.recent_window_ms, 600000);
    history.stream_stale_ms = number(settings, "History.StreamStaleMs", 2500, 100, 60000);
    history.minimum_samples =
        static_cast<std::size_t>(number(settings, "History.MinimumSamples", 3, 1, 100000));
    history.max_streams =
        static_cast<std::size_t>(number(settings, "History.MaxStreams", 64, 1, 1024));
    history.max_frames_per_stream = static_cast<std::size_t>(
        number(settings, "History.MaxFramesPerStream", 120000, history.minimum_samples, 1000000));
    // Each accessor consumes its key, making misspellings an error instead of silent defaults.
    if (!settings.empty())
        throw std::runtime_error("Unknown FPS configuration key");
    return result;
}

} // namespace phone_monitor::fps
