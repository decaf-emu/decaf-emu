#include "platform.h"
#include "platform_debug.h"

#ifdef PLATFORM_POSIX

namespace platform
{

void
debugBreak()
{
   // TODO: Implement debug breaks for POSIX
}

void
debugLog(const std::string& message)
{
   // TODO: Implement IDE debug logging for POSIX
}

} // namespace platform

#endif
