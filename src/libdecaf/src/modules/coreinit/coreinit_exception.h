#pragma once
#include "coreinit_enum.h"
#include "coreinit_thread.h"
#include "ppcutils/wfunc_ptr.h"

#include <common/cbool.h>

namespace coreinit
{

/**
 * \defgroup coreinit_exception Exception Handling
 * \ingroup coreinit
 * @{
 */

using OSExceptionCallback = wfunc_ptr<BOOL, OSContext*>;

OSExceptionCallback
OSSetExceptionCallback(OSExceptionType exceptionType,
                       OSExceptionCallback callback);

OSExceptionCallback
OSSetExceptionCallbackEx(uint32_t unk1,
                         OSExceptionType exceptionType,
                         OSExceptionCallback callback);

/** @} */

} // namespace coreinit
