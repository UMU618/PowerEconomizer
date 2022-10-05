#include "pch.h"

#include "utils.h"

#include <atlpath.h>

#include "umu/module.hpp"

namespace umutech::pweco::utils {

bool EnableEcoMode(HANDLE process, bool enable) noexcept {
  PROCESS_POWER_THROTTLING_STATE pts{
      .Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION,
      .ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED,
      .StateMask = (enable ? PROCESS_POWER_THROTTLING_EXECUTION_SPEED : 0UL)};
  if (!SetProcessInformation(process, ProcessPowerThrottling, &pts,
                             sizeof(pts))) {
    return false;
  }
  if (!SetPriorityClass(process, enable ? IDLE_PRIORITY_CLASS
                                        : NORMAL_PRIORITY_CLASS)) [[unlikely]] {
    return false;
  }
  return true;
}

bool GetNtVersion(DWORD* major, DWORD* minor, DWORD* build_number) noexcept {
  umu::Module ntdll;
  DWORD ec = ntdll.GetOrLoad(L"ntdll.dll");
  if (NO_ERROR != ec) [[unlikely]] {
    umu::console::Error(std::format(_T("Load ntdll.dll failed with {}\n"), ec));
    return false;
  }
  auto proc = reinterpret_cast<void(WINAPI*)(DWORD*, DWORD*, DWORD*)>(
      GetProcAddress(ntdll, "RtlGetNtVersionNumbers"));
  if (!proc) [[unlikely]] {
    umu::console::Error(_T("No RtlGetNtVersionNumbers\n"));
    return false;
  }

  proc(major, minor, build_number);
  return true;
}

CString GetProcessName(HANDLE process) noexcept {
  DWORD length = 1024;
  CString name;

  if (QueryFullProcessImageName(process, 0, name.GetBuffer(length), &length))
      [[likely]] {
    name.ReleaseBuffer(length);
    CPath path(name);
    return name.Mid(path.FindFileName());
  }

  return _T("");
}

bool IsUserAdmin() noexcept {
  bool result = false;
  SID_IDENTIFIER_AUTHORITY nt_authority = SECURITY_NT_AUTHORITY;
  PSID administrators_group;
  if (AllocateAndInitializeSid(&nt_authority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                               DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                               &administrators_group)) {
    BOOL is_member;
    if (CheckTokenMembership(NULL, administrators_group, &is_member) &&
        is_member) {
      result = true;
    }
    FreeSid(administrators_group);
  }

  return result;
}

std::tuple<DWORD, DWORD, HANDLE> OpenWindowProcess(HWND window) noexcept {
  DWORD pid = 0;
  DWORD tid = GetWindowThreadProcessId(window, &pid);
  if (0 == tid) [[unlikely]] {
    return {tid, pid, nullptr};
  }
  HANDLE process = OpenProcess(
      PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_SET_INFORMATION, FALSE, pid);
  return {tid, pid, process};
}

}  // namespace umutech::pweco::utils
