#pragma once
#include "espresso/espresso_registers.h"
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
   uint32_t cia;              // Current execution address
   uint32_t nia;              // Next execution address

   espresso::gpr_t gpr[32];   // Integer Registers
   espresso::fpr_t fpr[32];   // Floating-point Registers
   espresso::cr_t cr;         // Condition Register
   espresso::xer_t xer;       // XER Carry/Overflow register
   uint32_t lr;               // Link Register
   uint32_t ctr;              // Count Register

   espresso::fpscr_t fpscr;   // Floating-Point Status and Control Register

   espresso::pvr_t pvr;       // Processor Version Register
   espresso::msr_t msr;       // Machine State Register
   uint32_t sr[16];           // Segment Registers

   espresso::gqr_t gqr[8];    // Graphics Quantization Registers
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
