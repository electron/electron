#include "lib/browser/api/electron_api_power_monitor_win.h"

#include "base/logging.h"
#include "base/win/message_window.h"
#include "lib/browser/api/electron_api_power_monitor.h"

namespace electron {

namespace api {

namespace {

bool resume_event_emitted = false;

}  // namespace

PowerMonitorWin::PowerMonitorWin() {
  message_window_ = std::make_unique<base::win::MessageWindow>();
  message_window_->Create(base::BindRepeating(&PowerMonitorWin::WndProc,
                                              base::Unretained(this)));
}

PowerMonitorWin::~PowerMonitorWin() = default;

bool PowerMonitorWin::WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
  switch (message) {
    case WM_POWERBROADCAST:
      if (wparam == PBT_APMRESUMEAUTOMATIC) {
        if (!resume_event_emitted) {
          resume_event_emitted = true;
          Emit("resume");
        }
      } else if (wparam == PBT_APMSUSPEND) {
        resume_event_emitted = false;
        Emit("suspend");
      }
      break;
  }
  return false;
}

}  // namespace api

}  // namespace electron
