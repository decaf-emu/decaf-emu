#include "coreinit.h"
#include "coreinit_exception.h"

ExceptionCallback
gExceptionCallbacks[static_cast<size_t>(ExceptionType::Max)];

ExceptionCallback
OSSetExceptionCallback(ExceptionType exceptionType, ExceptionCallback callback)
{
   return OSSetExceptionCallbackEx(1, exceptionType, callback);
}

ExceptionCallback
OSSetExceptionCallbackEx(uint32_t unk1, ExceptionType exceptionType, ExceptionCallback callback)
{
   auto index = static_cast<size_t>(exceptionType);
   auto previous = gExceptionCallbacks[index];
   gExceptionCallbacks[index] = callback;
   return previous;
}

void
CoreInit::registerExceptionFunctions()
{
   RegisterSystemFunction(OSSetExceptionCallback);
   RegisterSystemFunction(OSSetExceptionCallbackEx);
}
