// SPDX-License-Identifier: MIT
#pragma once

#include "fps/config.h"
#include "platform/windows/handle.h"

#include <atomic>
#include <functional>
#include <string_view>
#include <thread>

namespace phone_monitor::windows {

class PresentMonProcess {
  public:
    // Invoked on the reader thread. The view is valid only for the duration of the call.
    using LineHandler = std::function<void(std::string_view)>;
    PresentMonProcess(const fps::BridgeConfig &config, LineHandler handler);
    ~PresentMonProcess();
    PresentMonProcess(const PresentMonProcess &) = delete;
    PresentMonProcess &operator=(const PresentMonProcess &) = delete;
    // Returns false only after a successful exit and a drained pipe; failures throw.
    bool running(std::uint64_t wait_ms);

  private:
    void read_output(const LineHandler &handler) noexcept;
    Handle job_;
    Handle pipe_;
    Handle process_;
    std::atomic<bool> reader_failed_{false};
    std::thread reader_;
};

} // namespace phone_monitor::windows
