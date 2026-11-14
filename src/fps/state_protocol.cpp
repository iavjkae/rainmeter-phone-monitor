// SPDX-License-Identifier: MIT
#include "fps/state_protocol.h"

#include <algorithm>
#include <cmath>
#include <locale>
#include <sstream>

namespace phone_monitor::fps {

std::optional<StateRecord> parse_state(std::string_view line) {
    const auto delimiter = line.find('|');
    if (delimiter == std::string_view::npos || delimiter == 0 || line.front() == '-')
        return std::nullopt;
    std::istringstream input{std::string(line)};
    input.imbue(std::locale::classic());
    StateRecord record;
    char separator = 0;
    auto read_number = [&](auto &number) {
        return static_cast<bool>(input >> number >> separator) && separator == '|';
    };
    auto &sample = record.sample;
    if (!read_number(record.timestamp_ms) || !read_number(sample.frames_per_second) ||
        !read_number(sample.one_percent_low) || !read_number(sample.frame_time_ms))
        return std::nullopt;
    for (double value : {sample.frames_per_second, sample.one_percent_low, sample.frame_time_ms})
        if (!std::isfinite(value) || value < 0)
            return std::nullopt;
    std::getline(input, sample.process_name);
    if (sample.process_name.find_first_of("|\r\n") != std::string::npos)
        return std::nullopt;
    sample.collector_active = true;
    return record;
}

std::string serialize_state(const StateRecord &record) {
    auto name = record.sample.process_name;
    for (char &ch : name)
        if (ch == '|' || ch == '\r' || ch == '\n')
            ch = '_';
    std::ostringstream output;
    output.imbue(std::locale::classic());
    output << record.timestamp_ms << '|' << record.sample.frames_per_second << '|'
           << record.sample.one_percent_low << '|' << record.sample.frame_time_ms << '|' << name;
    return output.str();
}

bool is_fresh(std::uint64_t timestamp_ms, std::uint64_t now_ms, std::uint64_t freshness_ms) {
    return timestamp_ms != 0 && now_ms >= timestamp_ms && now_ms - timestamp_ms < freshness_ms;
}

} // namespace phone_monitor::fps
