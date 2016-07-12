#pragma once
#include <atomic>
#include <chrono>
#include <thread>
#include "espresso/espresso_registers.h"

struct Tracer;

namespace cpu
{

static const uint32_t coreClockSpeed = 1243125000;
static const uint32_t busClockSpeed = 800000000;

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

                              // Reserve data for lwarx / stwcx.
   bool reserve;
   uint32_t reserveAddress;
   uint32_t reserveData;
};

struct Core : CoreRegs
{
   // Tracer used to record executed instructions
   Tracer *tracer;

   // State data used by the CPU executors
   uint32_t id;
   std::thread thread;
   uint32_t interrupt_mask { 0xFFFFFFFF };
   std::atomic<uint32_t> interrupt { 0 };
   std::chrono::system_clock::time_point next_alarm;

   inline uint64_t tb() {
      using ppc_timer_duration = std::chrono::duration < uint64_t, std::ratio<1, busClockSpeed * 4>>;
      auto now = std::chrono::high_resolution_clock::now();
      auto ticks = std::chrono::duration_cast<ppc_timer_duration>(now.time_since_epoch());
      return ticks.count();
   }
};

} // namespace cpu
