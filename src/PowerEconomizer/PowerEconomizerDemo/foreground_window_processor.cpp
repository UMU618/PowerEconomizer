#include "pch.h"

#include "foreground_window_processor.h"

#include "power_economizer.h"

extern umutech::pweco::PowerEconomizer g_power_economizer;

namespace umutech::pweco {

ForegroundWindowProcessor::~ForegroundWindowProcessor() {
  Stop();
}

bool ForegroundWindowProcessor::Run() noexcept {
  if (nullptr != win_event_hook_) [[unlikely]] {
    return true;
  }
  win_event_hook_ =
      SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, nullptr,
                      ForegroundEventProc, 0, 0,
                      WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
  if (nullptr == win_event_hook_) [[unlikely]] {
    umu::console::Error(
        std::format(_T("!SetWinEventHook(), #{}.\n"), GetLastError()));
    return false;
  }
  return true;
}

void ForegroundWindowProcessor::Stop() noexcept {
  if (nullptr != win_event_hook_) {
    UnhookWinEvent(win_event_hook_);
    win_event_hook_ = nullptr;
  }
}

VOID ForegroundWindowProcessor::ForegroundEventProc(
    HWINEVENTHOOK /*win_event_hook*/,
    DWORD event,
    HWND window,
    LONG /*object_id*/,
    LONG /*child_id*/,
    DWORD event_thread_id,
    DWORD event_time_ms) noexcept {
  assert(EVENT_SYSTEM_FOREGROUND == event);
#if _DEBUG
  umu::console::Print(std::format(_T("{:08X}: window {}, thread {}\n"),
                                  event_time_ms, static_cast<void*>(window),
                                  event_thread_id));
#endif
  g_power_economizer.HandleForegroundWindow(window);
}

}  // namespace umutech::pweco
