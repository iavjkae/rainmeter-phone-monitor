// SPDX-License-Identifier: MIT
#pragma once

#include "fps/state_protocol.h"

#include <filesystem>

namespace phone_monitor::windows {

// Invalidates old readings before capture starts and on every scope exit, including exceptions.
// One collector owns this destination; atomic replacement hides partially written records.
class StatePublisher {
  public:
    explicit StatePublisher(std::filesystem::path destination);
    ~StatePublisher() noexcept;
    StatePublisher(const StatePublisher &) = delete;
    StatePublisher &operator=(const StatePublisher &) = delete;
    void publish(const fps::StateRecord &record) const;

  private:
    std::filesystem::path destination_;
};

} // namespace phone_monitor::windows
