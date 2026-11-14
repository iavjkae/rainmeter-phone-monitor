// SPDX-License-Identifier: MIT
#include "platform/windows/state_publisher.h"

#include <windows.h>

#include <fstream>
#include <stdexcept>
#include <utility>

namespace phone_monitor::windows {
namespace {

void write_state(const std::filesystem::path &destination, const fps::StateRecord &record) {
    auto temporary = destination;
    temporary += L".tmp";
    const auto text = fps::serialize_state(record);
    bool opened = false;
    try {
        // Close the writer before rename or cleanup, including when writing fails.
        {
            std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
            if (!output)
                throw std::runtime_error("Cannot create FPS state");
            opened = true;
            if (!output.write(text.data(), static_cast<std::streamsize>(text.size())))
                throw std::runtime_error("Cannot write FPS state");
            output.close();
            if (!output)
                throw std::runtime_error("Cannot close FPS state");
        }
        // A reader lacking delete-sharing can briefly block replacement of the old file.
        // Retry for at most 200 ms, never truncate the currently published record.
        for (unsigned attempt = 0;; ++attempt) {
            if (MoveFileExW(temporary.c_str(), destination.c_str(), MOVEFILE_REPLACE_EXISTING))
                return;
            const auto error = GetLastError();
            // Windows may report an open destination as ACCESS_DENIED. A true permission
            // failure also reaches this bounded timeout and propagates to the caller.
            if (attempt == 20 || (error != ERROR_SHARING_VIOLATION &&
                                  error != ERROR_LOCK_VIOLATION && error != ERROR_ACCESS_DENIED))
                throw std::runtime_error("Cannot replace FPS state");
            Sleep(10);
        }
    } catch (...) {
        // Delete only a temporary file we successfully opened, preserving the original error.
        if (opened) {
            std::error_code ignored;
            std::filesystem::remove(temporary, ignored);
        }
        throw;
    }
}

} // namespace

StatePublisher::StatePublisher(std::filesystem::path destination)
    : destination_(std::move(destination)) {
    publish({});
}

StatePublisher::~StatePublisher() noexcept {
    try {
        publish({});
    } catch (...) {
        // Cleanup must preserve an in-flight capture error. If replacement is impossible,
        // remove the old record where permitted; readers also enforce a freshness timeout.
        std::error_code ignored;
        std::filesystem::remove(destination_, ignored);
    }
}

void StatePublisher::publish(const fps::StateRecord &record) const {
    write_state(destination_, record);
}

} // namespace phone_monitor::windows
