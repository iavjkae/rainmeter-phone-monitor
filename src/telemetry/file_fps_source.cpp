// SPDX-License-Identifier: MIT
#include "fps/state_protocol.h"
#include "telemetry/providers.h"

#include <fstream>
#include <utility>

namespace phone_monitor {
namespace {

class FileFpsSource final : public FpsSource {
  public:
    FileFpsSource(std::filesystem::path path, std::uint64_t freshness_ms)
        : path_(std::move(path)), freshness_ms_(freshness_ms) {}

    FpsSample sample(std::uint64_t now_ms) override {
        std::ifstream input(path_);
        std::string line;
        if (!std::getline(input, line))
            return {};
        const auto record = fps::parse_state(line);
        if (!record || !fps::is_fresh(record->timestamp_ms, now_ms, freshness_ms_))
            return {};
        return record->sample;
    }

  private:
    std::filesystem::path path_;
    std::uint64_t freshness_ms_;
};

} // namespace

std::unique_ptr<FpsSource> make_file_fps_source(std::filesystem::path path,
                                                std::uint64_t freshness_ms) {
    return std::make_unique<FileFpsSource>(std::move(path), freshness_ms);
}

} // namespace phone_monitor
