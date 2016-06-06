#pragma once
#include "platform/platform.h"
#include "log.h"

#ifdef PLATFORM_WINDOWS
#include <Windows.h>
#endif

inline void
debugPrint(std::string out)
{
   gLog->debug(out);

#ifdef PLATFORM_WINDOWS
   out.push_back('\n');
   OutputDebugStringA(out.c_str());
#endif
}
