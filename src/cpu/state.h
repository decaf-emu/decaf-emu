#pragma once
#include <atomic>
#include "cpu_registers.h"

namespace cpu
{

struct CoreState
{
   std::atomic_bool interrupt { false };
};

}

// Thread registers
// TODO: Some system registers may not be thread-specific!
struct ThreadState
{
   cpu::CoreState *core;
   struct Tracer *tracer;

   uint32_t cia;     // Current execution address
   uint32_t nia;     // Next execution address

   gpr_t gpr[32];    // Integer Registers
   fpr_t fpr[32];    // Floating-point Registers
   cr_t cr;          // Condition Register
   xer_t xer;        // XER Carry/Overflow register
   uint32_t lr;      // Link Register
   uint32_t ctr;     // Count Register

   fpscr_t fpscr;    // Floating-Point Status and Control Register

   pvr_t pvr;        // Processor Version Register
   msr_t msr;        // Machine State Register
   uint32_t sr[16];  // Segment Registers

   uint32_t tbu;     // Time Base Upper
   uint32_t tbl;     // Time Base Lower

   gqr_t gqr[8];     // Graphics Quantization Registers

   // Reserve data for lwarx / stwcx.
   bool reserve;
   uint32_t reserveAddress;
   uint32_t reserveData;
};
