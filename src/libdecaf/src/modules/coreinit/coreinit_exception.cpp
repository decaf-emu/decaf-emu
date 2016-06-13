#include <array>
#include "coreinit.h"
#include "coreinit_exception.h"

namespace coreinit
{

static std::array<OSExceptionCallback, OSExceptionType::Max>
sExceptionCallbacks;


/**
 * Set an exception handler.
 */
OSExceptionCallback
OSSetExceptionCallback(OSExceptionType exceptionType, OSExceptionCallback callback)
{
   return OSSetExceptionCallbackEx(1, exceptionType, callback);
}


/**
 * Set an exception handler.
 */
OSExceptionCallback
OSSetExceptionCallbackEx(uint32_t unk1, OSExceptionType exceptionType, OSExceptionCallback callback)
{
   auto index = static_cast<size_t>(exceptionType);
   auto previous = sExceptionCallbacks[index];
   sExceptionCallbacks[index] = callback;
   return previous;
}

void
Module::initialiseExceptions()
{
   sExceptionCallbacks.fill(nullptr);
}

void
Module::registerExceptionFunctions()
{
   RegisterKernelFunction(OSSetExceptionCallback);
   RegisterKernelFunction(OSSetExceptionCallbackEx);
}

} // namespace coreinit
