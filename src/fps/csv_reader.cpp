// SPDX-License-Identifier: MIT
#include "fps/csv_reader.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <locale>
#include <sstream>
#include <vector>

namespace phone_monitor::fps {
namespace {

std::optional<std::vector<std::string>> parse_csv(std::string_view line) {
    if (line.substr(0, 3) == "\xef\xbb\xbf")
        line.remove_prefix(3);
    if (!line.empty() && line.back() == '\r')
        line.remove_suffix(1);
    std::vector<std::string> fields(1);
    bool quoted = false;
    for (std::size_t i = 0; i < line.size(); ++i) {
        const char ch = line[i];
        if (ch == '"') {
            if (quoted && i + 1 < line.size() && line[i + 1] == '"') {
                fields.back() += '"';
                ++i;
            } else {
                quoted = !quoted;
            }
        } else if (ch == ',' && !quoted) {
            fields.emplace_back();
        } else {
            fields.back() += ch;
        }
    }
    if (quoted)
        return std::nullopt;
    return fields;
}

} // namespace

std::optional<PresentEvent> CsvReader::consume(std::string_view line) {
    const auto fields = parse_csv(line);
    if (!fields || fields->empty())
        return std::nullopt;
    if (std::find(fields->begin(), fields->end(), "Application") != fields->end()) {
        constexpr std::array<const char *, 4> names{"Application", "ProcessID", "SwapChainAddress",
                                                    "MsBetweenPresents"};
        std::array<std::size_t, 4> columns{};
        columns_.reset();
        for (std::size_t i = 0; i < names.size(); ++i) {
            const auto found = std::find(fields->begin(), fields->end(), names[i]);
            if (found == fields->end())
                return std::nullopt;
            columns[i] = static_cast<std::size_t>(found - fields->begin());
        }
        columns_ = columns;
        return std::nullopt;
    }
    if (!columns_ || *std::max_element(columns_->begin(), columns_->end()) >= fields->size())
        return std::nullopt;
    const auto &columns = *columns_;
    PresentEvent event;
    event.process_name = (*fields)[columns[0]];
    event.swap_chain = (*fields)[columns[2]];
    const auto &pid = (*fields)[columns[1]];
    const auto parsed = std::from_chars(pid.data(), pid.data() + pid.size(), event.process_id);
    if (parsed.ec != std::errc{} || parsed.ptr != pid.data() + pid.size() || event.process_id == 0)
        return std::nullopt;
    std::istringstream duration((*fields)[columns[3]]);
    duration.imbue(std::locale::classic());
    if (!(duration >> event.frame_time_ms) || !(duration >> std::ws).eof() ||
        !std::isfinite(event.frame_time_ms) || event.frame_time_ms <= 0 ||
        event.process_name.empty() || event.swap_chain.empty())
        return std::nullopt;
    return event;
}

} // namespace phone_monitor::fps
