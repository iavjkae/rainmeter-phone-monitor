// SPDX-License-Identifier: MIT
#pragma once

#include "fps/config.h"
#include "fps/csv_reader.h"
#include "telemetry/model.h"

#include <deque>
#include <map>
#include <utility>

namespace phone_monitor::fps {

// Not thread-safe: the pipe reader and sampler must hold the same external mutex.
// All timestamps use monotonic system uptime in milliseconds, never wall-clock time.
class FrameHistory {
  public:
    explicit FrameHistory(HistoryPolicy policy);
    void add(const PresentEvent &event, std::uint64_t now_ms);
    FpsSample sample(std::uint64_t now_ms, std::uint32_t foreground_pid, bool foreground_only);

  private:
    struct Frame {
        std::uint64_t timestamp_ms;
        double duration_ms;
    };
    struct Stream {
        std::string name;
        std::deque<Frame> frames;
    };
    using Key = std::pair<std::uint32_t, std::string>;
    HistoryPolicy policy_;
    std::map<Key, Stream> streams_;
};

} // namespace phone_monitor::fps
