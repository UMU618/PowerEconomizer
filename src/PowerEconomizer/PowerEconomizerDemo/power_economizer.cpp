#include "pch.h"

#include "power_economizer.h"

#include <atlfile.h>

#include <wtsapi32.h>

#include "toml++/toml.h"
#include "umu/apppath_t.h"

#include "utils.h"

namespace {

constexpr std::basic_string_view kUwpFrameHostApp{
    _T("ApplicationFrameHost.exe")};
constexpr UINT_PTR kTimerId = 'MZPE';

}  // namespace

namespace umutech::pweco {

using namespace std::literals::string_view_literals;

PowerEconomizer::~PowerEconomizer() {
  Stop();
}

bool PowerEconomizer::Initialize() noexcept {
  CString myself = utils::GetProcessName(GetCurrentProcess());
  if (!myself.IsEmpty()) {
    umu::console::Print(
        std::format(_T("Add process `{}' to bypass list.\n"), myself));
    bypass_processes_.insert(utils::ToStd(myself.MakeLower()));
  }

  const TCHAR* processes[] = {
    // Edge has energy awareness
    _T("msedge.exe"),
    _T("WebViewHost.exe"),
    _T("msedgewebview2.exe"),
    // UWP Frame has special handling, should not be throttled
    _T("ApplicationFrameHost.exe"),
    // Fire extinguisher should not catch fire
    _T("taskmgr.exe"),
    _T("procexp.exe"),
    _T("procexp64.exe"),
    _T("procmon.exe"),
    _T("procmon64.exe"),
    // Widgets
    _T("Widgets.exe"),
    // System shell
    _T("dwm.exe"),
    _T("explorer.exe"),
    _T("ShellExperienceHost.exe"),
    _T("StartMenuExperienceHost.exe"),
    _T("SearchHost.exe"),
    _T("sihost.exe"),
    _T("fontdrvhost.exe"),
    // IME
    _T("ChsIME.exe"),
    _T("ctfmon.exe"),
    _T("TextInputHost.exe"),
#if _DEBUG
    // Visual Studio
    _T("devenv.exe"),
#endif
    // System Service - they have their awareness
    _T("csrss.exe"),
    _T("smss.exe"),
    _T("svchost.exe"),
    _T("winlogon.exe"),
    // WUDF
    _T("WUDFRd.exe"),
    // Search
    _T("SearchProtocolHost.exe"),
    // Chinese pop apps
    _T("WechatBrowser.exe"),
  };

  for (std::size_t i = 0; i < std::size(processes); ++i) {
    CString process_name{processes[i]};
#if _DEBUG
    umu::console::Print(std::format(
        _T("Add built-in process `{}' to bypass list.\n"), process_name));
#endif
    bypass_processes_.insert(utils::ToStd(process_name.MakeLower()));
  }

  if (LoadConfig()) {
    umu::console::Print(std::format(_T("Add {} processes to bypass list.\n"),
                                    bypass_processes_.size()));
  } else {
    umu::console::Print(
        std::format(_T("Add {} built-in processes to bypass list.\n"),
                    bypass_processes_.size()));
  }

  return true;
}

bool PowerEconomizer::Run() noexcept {
  ThrottleAllUserBackgroundProcesses();
  // HouseKeeper
  SetTimer(nullptr, kTimerId, 5 * 60 * 1000,
           [](HWND, UINT timer_id, UINT_PTR, DWORD) -> VOID {
             if (kTimerId == timer_id) {
               extern PowerEconomizer g_power_economizer;
               g_power_economizer.ThrottleAllUserBackgroundProcesses();
             }
           });
  return true;
}

void PowerEconomizer::Stop() noexcept {
  KillTimer(nullptr, kTimerId);
  for (auto iter = process_priority_class_.begin();
       iter != process_priority_class_.end();
       iter = process_priority_class_.erase(iter)) {
    DWORD pid = iter->first;
    DWORD priority_class = iter->second;
    DWORD access = PROCESS_SET_INFORMATION;
#if _DEBUG
    access |= PROCESS_QUERY_LIMITED_INFORMATION;
#endif
    HANDLE process = OpenProcess(access, FALSE, pid);
    if (nullptr == process) {
      umu::console::Error(
          std::format(_T("!OpenProcess({}), #{}.\n"), pid, GetLastError()));
      continue;
    }
    ON_SCOPE_EXIT([&process] { CloseHandle(process); });
    if (!SetPriorityClass(process, priority_class)) [[unlikely]] {
      continue;
    }
    PROCESS_POWER_THROTTLING_STATE pts{
        .Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION,
        .ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED,
        .StateMask = 0};
    if (!SetProcessInformation(process, ProcessPowerThrottling, &pts,
                               sizeof(pts))) [[unlikely]] {
      continue;
    }
#if _DEBUG
    CString process_name = utils::GetProcessName(process);
    umu::console::Print(
        std::format(_T("Retore process `{}':{} priority class to {}.\n"),
                    process_name, pid, priority_class));
#endif
  }
}

void PowerEconomizer::ThrottleAllUserBackgroundProcesses() noexcept {
  DWORD level = 1;
  WTS_PROCESS_INFO_EX* pi{};
  DWORD count;
  if (!WTSEnumerateProcessesEx(WTS_CURRENT_SERVER_HANDLE, &level,
                               WTS_CURRENT_SESSION,
                               reinterpret_cast<LPWSTR*>(&pi), &count)) {
    umu::console::Error(
        std::format(_T("!WTSEnumerateProcessesEx, #{}.\n"), GetLastError()));
    return;
  }
  // F.53
  ON_SCOPE_EXIT([=] { WTSFreeMemoryEx(WTSTypeProcessInfoLevel1, pi, count); });

  for (DWORD i = 0; i < count; ++i) {
    if (pi[i].ProcessId == pending_pid_) [[unlikely]] {
#if _DEBUG
      umu::console::Print(std::format(_T("Bypass process `{}':{}.\n"),
                                      pi[i].pProcessName, pi[i].ProcessId));
#endif
      continue;
    }

    CString process_name{pi[i].pProcessName};
    process_name.MakeLower();
    if (bypass_processes_.contains(utils::ToStd(process_name))) {
#if _DEBUG
      umu::console::Print(std::format(_T("Bypass process `{}':{}.\n"),
                                      pi[i].pProcessName, pi[i].ProcessId));
#endif
      continue;
    }

    HANDLE process =
        OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_SET_INFORMATION,
                    FALSE, pi[i].ProcessId);
    if (nullptr == process) {
      umu::console::Error(std::format(_T("!OpenProcess(`{}':{}), #{}.\n"),
                                      pi[i].pProcessName, pi[i].ProcessId,
                                      GetLastError()));
      continue;
    }
    ON_SCOPE_EXIT([&process] { CloseHandle(process); });

    if (utils::IsEcoModeEnabled(process)) {
#if _DEBUG
      umu::console::Print(
          std::format(_T("Process `{}':{} has EcoQoS awareness.\n"),
                      pi[i].pProcessName, pi[i].ProcessId));
#endif
      continue;
    }

    if (EnableEcoMode(process, true, pi[i].ProcessId)) [[likely]] {
      umu::console::Print(
          std::format(_T("Enable EcoQoS for process `{}':{}.\n"),
                      pi[i].pProcessName, pi[i].ProcessId));
    }
  }
}

void PowerEconomizer::HandleForegroundWindow(HWND window) noexcept {
  auto [tid, pid, process] = utils::OpenWindowProcess(window);
  if (0 == tid) [[unlikely]] {
    umu::console::Error(std::format(_T("!GetWindowThreadProcessId({}), #{}.\n"),
                                    static_cast<void*>(window),
                                    GetLastError()));
    return;
  }
  if (nullptr == process) {
    umu::console::Error(
        std::format(_T("!OpenProcess({}), #{}.\n"), pid, GetLastError()));
    return;
  }
  ON_SCOPE_EXIT([&process] { CloseHandle(process); });

  CString process_name = utils::GetProcessName(process);
  if (process_name.IsEmpty()) [[unlikely]] {
    umu::console::Error(
        std::format(_T("!GetProcessName({}), #{}.\n"), pid, GetLastError()));
    return;
  }

  if (process_name.CompareNoCase(kUwpFrameHostApp.data()) == 0) {
    struct Captured {
      bool found;
      DWORD pid;
      HANDLE process;
    };

    Captured captured{.found = false, .pid = pid};

    EnumChildWindows(
        window,
        [](HWND inner_window, LPARAM lparam) -> BOOL {
          auto [inner_tid, inner_pid, inner_process] =
              utils::OpenWindowProcess(inner_window);
          assert(0 != lparam);
          if (0 == inner_tid || nullptr == inner_process) {
            return TRUE;  // continue EnumChildWindows
          }
          auto captured = reinterpret_cast<Captured*>(lparam);
          if (captured->pid != inner_pid) {
            captured->found = true;
            captured->pid = inner_pid;
            captured->process = inner_process;
            return FALSE;  // break EnumChildWindows
          }
          return TRUE;  // continue EnumChildWindows
        },
        reinterpret_cast<LPARAM>(&captured));

    if (captured.found) {
      CloseHandle(process);
      process = captured.process;
      process_name = utils::GetProcessName(process);
      if (process_name.IsEmpty()) {
        umu::console::Error(std::format(_T("UWP!GetProcessName({}), #{}.\n"),
                                        captured.pid, GetLastError()));
        return;
      }
    }
  }
#if _DEBUG
  umu::console::Print(
      std::format(_T("  Process `{}':{}\n"), process_name, pid));
#endif

  // Boost the current foreground app
  auto process_lower_name = process_name;
  bool bypass =
      bypass_processes_.contains(utils::ToStd(process_lower_name.MakeLower()));
  if (!bypass) {
    if (EnableEcoMode(process, false, pid)) {
      umu::console::Print(
          std::format(_T("Boost `{}':{}\n"), process_name, pid));
    }
  }

  // Impose EcoQoS for previous foreground app
  if (0 != pending_pid_ && pid != pending_pid_) {
    HANDLE previous_process =
        OpenProcess(PROCESS_SET_INFORMATION, FALSE, pending_pid_);
    if (nullptr != previous_process) {
      if (EnableEcoMode(previous_process, false, pending_pid_)) [[likely]] {
        umu::console::Print(std::format(_T("Throttle `{}':{}\n"),
                                        pending_process_name_, pending_pid_));
      }
      CloseHandle(previous_process);
      pending_pid_ = 0;
    } else {
      int ec = GetLastError();
      umu::console::Error(std::format(_T("!OpenProcess(`{}':{}), #{}.\n"),
                                      pending_process_name_, pending_pid_,
                                      GetLastError()));
    }
  }

  if (!bypass) {
    pending_pid_ = pid;
    pending_process_name_ = process_name;
  }
}

bool PowerEconomizer::EnableEcoMode(HANDLE process,
                                    bool enable,
                                    DWORD pid) noexcept {
  PROCESS_POWER_THROTTLING_STATE pts{
      .Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION,
      .ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED,
  };

  if (enable) {
    pts.StateMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;
    if (!SetProcessInformation(process, ProcessPowerThrottling, &pts,
                               sizeof(pts))) [[unlikely]] {
      return false;
    }

    DWORD priority_class = GetPriorityClass(process);
    if (!SetPriorityClass(process, IDLE_PRIORITY_CLASS)) [[unlikely]] {
      return false;
    }
    process_priority_class_.insert({pid, priority_class});
  } else {
    DWORD priority_class = NORMAL_PRIORITY_CLASS;
    if (auto iter = process_priority_class_.find(pid);
        iter != process_priority_class_.end()) {
      priority_class = iter->second;
      process_priority_class_.erase(iter);
#if _DEBUG
      umu::console::Print(std::format(
          _T("  Restore process {} priority class {}\n"), pid, priority_class));
#endif
    }
    if (!SetPriorityClass(process, priority_class)) [[unlikely]] {
      return false;
    }

    pts.StateMask = 0;
    if (!SetProcessInformation(process, ProcessPowerThrottling, &pts,
                               sizeof(pts))) [[unlikely]] {
      return false;
    }
  }
  return true;
}

bool PowerEconomizer::LoadConfig() noexcept {
  CString config_path =
      umu::apppath_t::GetProgramDirectory() + _T("PowerEconomizer.toml");
  CAtlFile file;
  HRESULT hr =
      file.Create(config_path, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING);
  if (FAILED(hr)) {
    return false;
  }
  ULONGLONG file_size = 0;
  hr = file.GetSize(file_size);
  if (FAILED(hr)) [[unlikely]] {
    return false;
  }
  if (16 * 1024 * 1024 < file_size) {
    umu::console::ColorError(
        kColorError,
        std::format(_T("Config file size {} > 16MiB!\n"), file_size));
    return false;
  }
  std::string buffer;
  buffer.resize(file_size);
  DWORD read_size = 0;
  hr = file.Read(buffer.data(), utils::NarrowCast<DWORD>(file_size), read_size);
  if (FAILED(hr)) [[unlikely]] {
    return false;
  }
  buffer.resize(read_size);

  toml::table t;
  try {
    t = toml::parse(buffer);
  } catch (const toml::parse_error& err) {
    umu::console::ColorError(
        kColorError, CA2T(std::format("Parse config file error: {}, {}\n",
                                      err.what(), err.description())
                              .data())
                         .m_psz);
    return false;
  }

  toml::table::const_iterator iter = t.find("config");
  if (iter == t.cend() || !iter->second.is_table()) {
    umu::console::ColorError(
        kColorError, std::format(_T("No [config] in `{}'\n"), config_path));
    return false;
  }
  auto c = iter->second.as_table();
  auto bypass = (*c)["bypass_process_list"];
  if (!bypass.is_array()) {
    umu::console::ColorError(
        kColorError,
        std::format(_T("Invalid bypass_process_list in `{}'\n"), config_path));
    return false;
  }

  for (auto&& elem : *bypass.as_array()) {
    if (elem.is_string()) {
      auto s = elem.as_string()->value_or(""sv);
      if (!s.empty()) {
        CString process_name{CA2T(s.data(), CP_UTF8).m_psz};
#if _DEBUG
        umu::console::Print(std::format(
            _T("Add process `{}' to bypass list.\n"), process_name));
#endif
        bypass_processes_.insert(utils::ToStd(process_name.MakeLower()));
      }
    }
  }

  return true;
}

}  // namespace umutech::pweco
