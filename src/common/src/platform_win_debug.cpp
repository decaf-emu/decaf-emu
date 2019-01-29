#include "platform.h"

#ifdef PLATFORM_WINDOWS
#include "platform_debug.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace platform
{

void
debugBreak()
{
   DebugBreak();
}

void
debugLog(const std::string& message)
{
   OutputDebugStringA(message.c_str());
}

} // namespace platform

#endif
