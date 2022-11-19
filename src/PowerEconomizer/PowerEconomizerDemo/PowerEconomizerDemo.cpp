#include "pch.h"

#include <signal.h>

#include "foreground_window_processor.h"
#include "power_economizer.h"
#include "utils.h"

constexpr WORD kColorNo =
    FOREGROUND_RED | FOREGROUND_INTENSITY | COMMON_LVB_UNDERSCORE;
constexpr WORD kColorYes = FOREGROUND_GREEN | COMMON_LVB_UNDERSCORE;

DWORD g_main_thread_id = 0;
umutech::pweco::PowerEconomizer g_power_economizer;

void SignalHandler(int sig) {
  if (0 != g_main_thread_id) {
    PostThreadMessage(g_main_thread_id, WM_QUIT, 0, 0);
    umu::console::Print(
        std::format(_T("Received signal {} from thread {}...\n"), sig,
                    GetCurrentThreadId()));
  }
}

int main() {
  umu::ScopeGuard auto_pause{[] {
    if (umu::console::IsRunAlone()) {
      umu::console::Pause();
    }
  }};

  DWORD major_version;
  DWORD minor_version;
  DWORD build_number;
  if (!umutech::pweco::utils::GetNtVersion(&major_version, &minor_version,
                                           &build_number)) [[unlikely]] {
    umu::console::ColorError(kColorError, _T("Get NT version failed!\n"));
    return EXIT_FAILURE;
  }

  build_number &= 0xffff;
  bool supported = false;

  if ((10 == major_version && 22000 <= build_number) || 10 < major_version) {
    supported = true;
  }

  umu::console::Print(std::format(_T("OS version: {}.{}.{}, "), major_version,
                                  minor_version, build_number));
  if (!supported) {
    umu::console::ColorPrint(kColorNo, _T("unsupported"));
    return EXIT_FAILURE;
  }
  umu::console::ColorPrint(kColorYes, _T("supported"));

  umu::console::Print(_T("\nIs run as admin: "));
  if (umutech::pweco::utils::IsUserAdmin()) {
    umu::console::ColorPrint(kColorYes, _T("Yes"));
  } else {
    umu::console::ColorPrint(kColorNo, _T("No"));
  }

  g_main_thread_id = GetCurrentThreadId();
  umu::console::Print(
      std::format(_T("\nMain thread ID: {}\n"), g_main_thread_id));

  if (!g_power_economizer.Initialize()) [[unlikely]] {
    return EXIT_FAILURE;
  }
  if (!g_power_economizer.Run()) [[unlikely]] {
    return EXIT_FAILURE;
  }

  umutech::pweco::ForegroundWindowProcessor fwp;
  if (!fwp.Run()) [[unlikely]] {
    return EXIT_FAILURE;
  }

  signal(SIGINT, SignalHandler);
  signal(SIGBREAK, SignalHandler);
  auto_pause.Dismiss();
  umu::console::ColorPrint(kColorInfo, _T("Press CTRL+C to quit...\n"));

  MSG msg{};
  while (GetMessage(&msg, nullptr, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  return static_cast<int>(msg.wParam);
}
