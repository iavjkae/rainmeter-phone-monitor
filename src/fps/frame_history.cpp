// SPDX-License-Identifier: MIT
#include "fps/frame_history.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <stdexcept>
#include <vector>

namespace phone_monitor::fps {

FrameHistory::FrameHistory(HistoryPolicy policy) : policy_(policy) {
    if (policy_.recent_window_ms == 0 || policy_.history_window_ms < policy_.recent_window_ms ||
        policy_.minimum_samples == 0 || policy_.max_streams == 0 ||
        policy_.max_frames_per_stream < policy_.minimum_samples)
        throw std::invalid_argument("Invalid FPS history policy");
}

void FrameHistory::add(const PresentEvent &event, std::uint64_t now_ms) {
    if (!std::isfinite(event.frame_time_ms) || event.frame_time_ms <= 0)
        return;
    const Key key{event.process_id, event.swap_chain};
    if (streams_.count(key) == 0 && streams_.size() >= policy_.max_streams) {
        const auto oldest =
            std::min_element(streams_.begin(), streams_.end(), [](const auto &a, const auto &b) {
                return a.second.frames.back().timestamp_ms < b.second.frames.back().timestamp_ms;
            });
        streams_.erase(oldest);
    }
    auto &stream = streams_[key];
    if (!stream.frames.empty() && now_ms < stream.frames.back().timestamp_ms)
        return;
    stream.name = event.process_name;
    stream.frames.push_back({now_ms, event.frame_time_ms});
    while (stream.frames.size() > policy_.max_frames_per_stream ||
           now_ms - stream.frames.front().timestamp_ms > policy_.history_window_ms)
        stream.frames.pop_front();
}

FpsSample FrameHistory::sample(std::uint64_t now_ms, std::uint32_t foreground_pid,
                               bool foreground_only) {
    Stream *chosen = nullptr;
    std::size_t best_count = 0;
    bool chosen_is_foreground = false;
    for (auto it = streams_.begin(); it != streams_.end();) {
        auto &frames = it->second.frames;
        while (!frames.empty() && now_ms >= frames.front().timestamp_ms &&
               now_ms - frames.front().timestamp_ms > policy_.history_window_ms)
            frames.pop_front();
        if (frames.empty()) {
            it = streams_.erase(it);
            continue;
        }
        const auto &key = it->first;
        auto &stream = it++->second;
        // The reader can append after the caller captures now_ms but before it takes the lock.
        // Use only the prefix visible at the snapshot time, including for the stale check.
        const auto end = std::upper_bound(
            frames.begin(), frames.end(), now_ms,
            [](auto timestamp, const auto &frame) { return timestamp < frame.timestamp_ms; });
        if (end == frames.begin() ||
            now_ms - std::prev(end)->timestamp_ms > policy_.stream_stale_ms)
            continue;
        const bool foreground = key.first == foreground_pid;
        if ((foreground_only && !foreground) || (chosen_is_foreground && !foreground))
            continue;
        const auto count =
            static_cast<std::size_t>(std::count_if(frames.begin(), end, [&](const auto &frame) {
                return now_ms - frame.timestamp_ms < policy_.recent_window_ms;
            }));
        if (count < policy_.minimum_samples ||
            (foreground == chosen_is_foreground && count <= best_count))
            continue;
        chosen = &stream;
        best_count = count;
        chosen_is_foreground = foreground;
    }
    FpsSample result;
    result.collector_active = true;
    if (!chosen)
        return result;
    double recent_sum = 0;
    std::vector<double> durations;
    for (const auto &frame : chosen->frames) {
        if (frame.timestamp_ms > now_ms)
            break;
        durations.push_back(frame.duration_ms);
        if (now_ms - frame.timestamp_ms < policy_.recent_window_ms)
            recent_sum += frame.duration_ms;
    }
    // FPS is the inverse mean of recent frame times. 1% Low uses the mean of the
    // slowest ceil(1%) frames in the longer history, rather than a percentile of FPS.
    std::sort(durations.rbegin(), durations.rend());
    const auto slow_count =
        std::max<std::size_t>(1, static_cast<std::size_t>(std::ceil(durations.size() * .01)));
    double slow_sum = 0;
    for (std::size_t i = 0; i < slow_count; ++i)
        slow_sum += durations[i];
    result.frames_per_second = best_count * 1000.0 / recent_sum;
    result.frame_time_ms = recent_sum / best_count;
    result.one_percent_low = slow_count * 1000.0 / slow_sum;
    result.process_name = chosen->name;
    return result;
}

} // namespace phone_monitor::fps
