#pragma once

namespace umutech::pweco {

class ForegroundWindowProcessor {
 public:
  ForegroundWindowProcessor() = default;
  ~ForegroundWindowProcessor();

  bool Run() noexcept;
  void Stop() noexcept;

 private:
  static VOID CALLBACK ForegroundEventProc(HWINEVENTHOOK win_event_hook,
                                           DWORD event,
                                           HWND window,
                                           LONG object_id,
                                           LONG child_id,
                                           DWORD event_thread_id,
                                           DWORD event_time_ms) noexcept;

 private:
  HWINEVENTHOOK win_event_hook_{nullptr};
};

}  // namespace umutech::pweco
