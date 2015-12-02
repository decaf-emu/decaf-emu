#include "coreinit.h"
#include "coreinit_exception.h"

OSExceptionCallback
gExceptionCallbacks[OSExceptionType::Max];

OSExceptionCallback
OSSetExceptionCallback(OSExceptionType exceptionType, OSExceptionCallback callback)
{
   return OSSetExceptionCallbackEx(1, exceptionType, callback);
}

OSExceptionCallback
OSSetExceptionCallbackEx(uint32_t unk1, OSExceptionType exceptionType, OSExceptionCallback callback)
{
   auto index = static_cast<size_t>(exceptionType);
   auto previous = gExceptionCallbacks[index];
   gExceptionCallbacks[index] = callback;
   return previous;
}

void
CoreInit::registerExceptionFunctions()
{
   RegisterKernelFunction(OSSetExceptionCallback);
   RegisterKernelFunction(OSSetExceptionCallbackEx);
}
