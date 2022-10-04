#pragma once

#include <cassert>
#include <format>
#include <string>
#include <string_view>

#include <atlbase.h>
#include <atlstr.h>

#include "umu/console.h"
#include "umu/scope_exit.hpp"

constexpr WORD kColorError = FOREGROUND_RED | FOREGROUND_INTENSITY;
constexpr WORD kColorInfo = FOREGROUND_GREEN;
