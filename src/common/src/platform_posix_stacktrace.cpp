#include "platform.h"
#include "platform_stacktrace.h"

#ifdef PLATFORM_POSIX
#include <stdexcept>

namespace platform
{

StackTrace * captureStackTrace()
{
   return nullptr;
}

void freeStackTrace(StackTrace *)
{
}

void printStackTrace(StackTrace *)
{
   throw std::logic_error("POSIX support for stack tracing is not implemented");
}

} // namespace platform

#endif
