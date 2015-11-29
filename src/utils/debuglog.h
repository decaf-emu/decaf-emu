#pragma once
#include <spdlog/spdlog.h>
#include "platform/platform.h"

template<typename... Args>
static void
debugPrint(fmt::CStringRef msg, Args... args)
{
   auto out = fmt::format(msg, args...);
   out += "\n";

#ifdef PLATFORM_WINDOWS
   OutputDebugStringA(out.c_str());
#else
   printf("%s", out.c_str());
#endif
}
