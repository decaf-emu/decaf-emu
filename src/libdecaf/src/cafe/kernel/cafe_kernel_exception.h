#pragma once
#include "cafe_kernel_context.h"
#include <libcpu/be2_struct.h>
#include <libcpu/cpu.h>

namespace cafe::kernel
{

enum class ExceptionType
{
   SystemReset          = 0,
   MachineCheck         = 1,
   DSI                  = 2,
   ISI                  = 3,
   ExternalInterrupt    = 4,
   Alignment            = 5,
   Program              = 6,
   FloatingPoint        = 7,
   Decrementer          = 8,
   SystemCall           = 9,
   Trace                = 10,
   PerformanceMonitor   = 11,
   Breakpoint           = 12,
   SystemInterrupt      = 13,
   ICI                  = 14,
   Max,
};

namespace internal
{

void
initialiseExceptionContext(cpu::Core *core);

void
initialiseExceptionHandlers();

void
initialiseStaticExceptionData();

} // namespace internal

} // namespace cafe::kernel
