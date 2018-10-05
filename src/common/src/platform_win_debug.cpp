#include "platform.h"
#include "platform_debug.h"

#ifdef PLATFORM_WINDOWS
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
