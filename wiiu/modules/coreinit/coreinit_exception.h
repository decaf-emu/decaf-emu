#pragma once
#include "systemtypes.h"
#include "coreinit_thread.h"

// From WUP-P-ARKE
enum class ExceptionType
{
   SystemReset = 0,
   MachineCheck = 1,
   DSI = 2,
   ISI = 3,
   ExternalInterrupt = 4,
   Alignment = 5,
   Program = 6,
   FloatingPoint = 7,
   Decrementer = 8,
   SystemCall = 9,
   Trace = 10,
   PerformanceMonitor = 11,
   Breakpoint = 12,
   SystemInterrupt = 13,
   ICI = 14,
   Max = 15
};

using ExceptionCallback = wfunc_ptr<BOOL, OSContext*>;

ExceptionCallback
OSSetExceptionCallback(ExceptionType exceptionType, ExceptionCallback callback);

ExceptionCallback
OSSetExceptionCallbackEx(uint32_t unk1, ExceptionType exceptionType, ExceptionCallback callback);
