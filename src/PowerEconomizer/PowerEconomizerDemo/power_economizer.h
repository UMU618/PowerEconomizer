#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>

namespace umutech::pweco {

class PowerEconomizer {
 public:
  PowerEconomizer() = default;
  ~PowerEconomizer();

  bool Initialize() noexcept;
  bool Run() noexcept;
  void Stop() noexcept;

  void HandleForegroundWindow(HWND window) noexcept;
  void ThrottleAllUserBackgroundProcesses() noexcept;

 private:
  // supersede utils::EnableEcoMode
  bool EnableEcoMode(HANDLE process, bool enable, DWORD pid) noexcept;
  bool LoadConfig() noexcept;

 private:
  DWORD pending_pid_;
  CString pending_process_name_;

  std::unordered_set<std::basic_string<TCHAR>> bypass_processes_;
  std::unordered_map<DWORD, DWORD> process_priority_class_;
};

}  // namespace umutech::pweco
