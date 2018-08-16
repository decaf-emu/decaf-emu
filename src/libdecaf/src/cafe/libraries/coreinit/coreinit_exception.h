#pragma once
#include "coreinit_context.h"
#include "coreinit_enum.h"
#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

using OSExceptionCallbackFn = virt_func_ptr<
   BOOL(virt_ptr<OSContext> context)>;

OSExceptionCallbackFn
OSSetExceptionCallback(OSExceptionType type,
                       OSExceptionCallbackFn callback);

OSExceptionCallbackFn
OSSetExceptionCallbackEx(OSExceptionMode mode,
                         OSExceptionType type,
                         OSExceptionCallbackFn callback);

namespace internal
{

void
initialiseExceptionHandlers();

OSExceptionCallbackFn
getExceptionCallback(OSExceptionMode mode,
                     OSExceptionType type);

void
setExceptionCallback(OSExceptionMode mode,
                     OSExceptionType type,
                     OSExceptionCallbackFn callback);

} // namespace internal

} // namespace cafe::coreinit
