// SPDX-License-Identifier: MIT
#include "fps/application.h"
#include "fps/config.h"
#include "fps/csv_reader.h"
#include "fps/frame_history.h"
#include "fps/process_filter.h"
#include "platform/windows/bridge_environment.h"
#include "platform/windows/presentmon_process.h"
#include "platform/windows/state_publisher.h"

#include <mutex>

namespace phone_monitor::fps {

int run_application() {
    windows::SingleInstance instance;
    if (!instance.acquired())
        return 0;
    const auto directory = windows::executable_directory();
    const auto config = load_config(directory / "FPSBridge.ini", directory / "FPSBridge.local.ini");
    ProcessFilter filter(config.selection);
    FrameHistory history(config.history);
    CsvReader parser;
    std::mutex mutex;
    windows::StatePublisher state(config.state_file);
    {
        // The capture must be destroyed before the callback's history, parser and mutex.
        windows::PresentMonProcess capture(config, [&](std::string_view line) {
            if (const auto event = parser.consume(line);
                event && filter.allows(event->process_name)) {
                std::lock_guard<std::mutex> guard(mutex);
                history.add(*event, GetTickCount64());
            }
        });
        auto last_panel = GetTickCount64();
        while (capture.running(config.update_interval_ms)) {
            const auto now = GetTickCount64();
            if (windows::panel_is_loaded(directory.parent_path()))
                last_panel = now;
            else if (now - last_panel > config.unload_grace_ms)
                break;
            FpsSample sample;
            {
                std::lock_guard<std::mutex> guard(mutex);
                sample = history.sample(now, windows::foreground_process_id(),
                                        config.selection.foreground_only);
            }
            state.publish({now, std::move(sample)});
        }
    }
    return 0;
}

} // namespace phone_monitor::fps
