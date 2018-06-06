#pragma once
#include "espresso/espresso_registerformats.h"
#include "mem.h"

#include <atomic>
#include <chrono>
#include <thread>

struct Tracer;

namespace cpu
{

static const uint32_t coreClockSpeed = 1243125000;
static const uint32_t busClockSpeed = 248625000;
static const uint32_t timerClockSpeed = busClockSpeed / 4;

using TimerDuration = std::chrono::duration<uint64_t, std::ratio<1, timerClockSpeed>>;

struct CoreRegs
{
   //! Current execution address
   uint32_t cia;

   //! Next execution address
   uint32_t nia;

   //! Integer Registers
   espresso::Register gpr[32];

   //! Floating-point Registers
   espresso::FloatingPointRegister fpr[32];

   //! Condition Register
   espresso::ConditionRegister cr;

   //! XER Carry/Overflow register
   espresso::FixedPointExceptionRegister xer;

   //! Link Register
   uint32_t lr;

   //! Count Register
   uint32_t ctr;

   //! Floating-Point Status and Control Register
   espresso::FloatingPointStatusAndControlRegister fpscr;

   //! Processor Version Register
   espresso::ProcessorVersionRegister pvr;

   //! Machine State Register
   espresso::MachineStateRegister msr;

   //! Segment Registers
   uint32_t sr[16];

   //! Graphics Quantization Registers
   espresso::GraphicsQuantisationRegister gqr[8];
};

struct Core : CoreRegs
{
   // State data used by the CPU executors
   uint32_t id;
   std::thread thread;
   uint32_t interrupt_mask { 0xFFFFFFFF };
   std::atomic<uint32_t> interrupt { 0 };
   bool reserveFlag { false };
   uint32_t reserveData;
   std::chrono::steady_clock::time_point next_alarm;

   // Tracer used to record executed instructions
   Tracer *tracer;

   // Get current core time
   uint64_t tb();
};

} // namespace cpu
