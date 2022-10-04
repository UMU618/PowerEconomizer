#pragma once

#include <string>
#include <tuple>

namespace umutech::pweco::utils {

// narrow_cast(): a searchable way to do narrowing casts of values
template <class T, class U>
constexpr T NarrowCast(U&& u) noexcept {
  return static_cast<T>(std::forward<U>(u));
}

bool EnableEcoMode(HANDLE process, bool enable) noexcept;
bool GetNtVersion(DWORD* major, DWORD* minor, DWORD* build_number) noexcept;
CString GetProcessName(HANDLE process) noexcept;
bool IsUserAdmin() noexcept;
std::tuple<DWORD, DWORD, HANDLE> OpenWindowProcess(HWND window) noexcept;

inline bool IsEcoModeEnabled(HANDLE process) noexcept {
  PROCESS_POWER_THROTTLING_STATE pts{
      .Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION};
  if (!GetProcessInformation(process, ProcessPowerThrottling, &pts,
                             sizeof(pts))) {
    return false;
  }
  return PROCESS_POWER_THROTTLING_EXECUTION_SPEED & pts.StateMask;
}

inline std::basic_string<TCHAR> ToStd(const CString& cs) noexcept {
  return std::basic_string<TCHAR>{cs.GetString(),
                                  static_cast<std::size_t>(cs.GetLength())};
}

}  // namespace umutech::pweco::utils
