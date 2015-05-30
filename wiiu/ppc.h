#pragma once
#include <cstdint>

// General Purpose Integer Registers
using gpr_t = uint32_t;

// Floating-Point Registers
union fpr_t
{
   double value;
   uint64_t idw;

   struct
   {
      float paired0;
      float paired1;
   };

   struct
   {
      uint32_t iw0;
      uint32_t iw1;
   };
};

namespace ConditionRegisterFlag
{
enum ConditionRegisterFlag : uint32_t
{
   Negative          = 1 << 3,
   Positive          = 1 << 2,
   Zero              = 1 << 1,
   SummaryOverflow   = 1 << 0,

   LessThan          = Negative,
   GreaterThan       = Positive,
   Equal             = Zero,
   Unordered         = SummaryOverflow,

   FloatingPointException           = Negative,
   FloatingPointExceptionEnabled    = Positive,
   FloatingPointInvalidException    = Zero,
   FloatingPointOverflowException   = SummaryOverflow,
};
}

// Condition Register
union cr_t
{
   uint32_t value;

   struct
   {
      uint32_t cr7 : 4;
      uint32_t cr6 : 4;
      uint32_t cr5 : 4;
      uint32_t cr4 : 4;
      uint32_t cr3 : 4;
      uint32_t cr2 : 4;
      uint32_t cr1 : 4;
      uint32_t cr0 : 4;
   };
};

// XER Register
union xer_t
{
   uint32_t value;

   struct
   {
      uint32_t byteCount : 7; // For lmwx, stmwx
      uint32_t : 22;
      uint32_t ca : 1;        // Carry
      uint32_t ov : 1;        // Overflow
      uint32_t so : 1;        // Sticky OV
   };

   struct
   {
      uint32_t : 28;
      uint32_t crxr; // [0-3] condition stuff in xer
   };
};

// Machine State Register
union msr_t
{
   uint32_t value;

   struct
   {
      uint32_t le : 1;  // Little-endian mode enabled
      uint32_t ri : 1;  // Exception is recoverable
      uint32_t : 2;
      uint32_t dr : 1;  // Data address translation enabled
      uint32_t ir : 1;  // Instruction address translation enabled
      uint32_t ip : 1;  // Exception prefix
      uint32_t : 1;
      uint32_t fe1 : 1; // Floating-point exception mode 1
      uint32_t be : 1;  // Branch trace enabled
      uint32_t se : 1;  // Single-step trace enabled
      uint32_t fe0 : 1; // Floating-point exception mode 0
      uint32_t me : 1;  // Machine check enabled
      uint32_t fp : 1;  // Floating-point available
      uint32_t pr : 1;  // Privelege level (0 = supervisor, 1 = user)
      uint32_t ee : 1;  // External interrupt enabled
      uint32_t ile : 1; // Exception little-endian mode
      uint32_t : 1;
      uint32_t pow : 1; // Power management enabled
      uint32_t : 13;
   };
};

namespace FloatingPointResultFlags
{
enum FloatingPointResultFlags : uint32_t
{
   ClassDescriptor   = 1 << 4,
   Negative          = 1 << 3,
   Positive          = 1 << 2,
   Zero              = 1 << 1,
   NaN               = 1 << 0,

   LessThan          = Negative,
   GreaterThan       = Positive,
   Equal             = Zero,
   Unordered         = NaN,
};
}

namespace FloatingPointRoundMode
{
enum FloatingPointRoundMode : uint32_t
{
   Nearest  = 0,
   Positive = 1,
   Zero     = 2,
   Negative = 3
};
}

// Floating-Point Status and Control Register
union fpscr_t
{
   uint32_t value;

   struct
   {
      uint32_t : 28;
      uint32_t cr1 : 4;
   };

   struct
   {
      uint32_t rn : 2;     // FP Rounding Control
      uint32_t ni : 1;     // FP non-IEEE mode
      uint32_t xe : 1;     // FP Inexact Exception Enable
      uint32_t ze : 1;     // IEEE FP Zero Divide Exception Enable
      uint32_t ue : 1;     // IEEE FP Underflow Exception Enable
      uint32_t oe : 1;     // IEEE FP Overflow Exception Enable
      uint32_t ve : 1;     // FP Invalid Operation Exception Enable
      uint32_t vxcvi : 1;  // FP Invalid Operation Exception for Invalid Integer Convert
      uint32_t vxsqrt : 1; // FP Invalid Operation Exception for Invalid Square Root
      uint32_t vxsoft : 1; // FP Invalid Operation Exception for Software Request
      uint32_t : 1;
      uint32_t fprf : 5;   // FP Result Flags
      uint32_t fi : 1;     // FP Fraction Inexact
      uint32_t fr : 1;     // FP Fraction Rounded
      uint32_t vxvc : 1;   // FP Invalid Operation Exception for Invalid Compare
      uint32_t vximz : 1;  // FP Invalid Operation Exception for Inf*0
      uint32_t vxzdz : 1;  // FP Invalid Operation Exception for 0/0
      uint32_t vxidi : 1;  // FP Invalid Operation Exception for Inf/Inf
      uint32_t vxisi : 1;  // FP Invalid Operation Exception for Inf-Inf
      uint32_t vxsnan : 1; // FP Invalid Operation Exception for SNaN
      uint32_t xx : 1;     // FP Inexact Exception
      uint32_t zx : 1;     // FP Zero Divide Exception
      uint32_t ux : 1;     // FP Underflow Exception
      uint32_t ox : 1;     // FP Overflow Exception
      uint32_t vx : 1;     // FP Invalid Operation Exception Summary
      uint32_t fex : 1;    // FP Enabled Exception Summary
      uint32_t fx : 1;     // FP Exception Summary
   };
};

// Processor Version Register
union pvr_t
{
   uint32_t value;

   struct
   {
      uint32_t revision : 16;
      uint32_t version : 16;
   };
};

enum class SprEncoding
{
   CTR = 9,
   DABR = 1013,
   DAR = 19,
   DBAT0L = 537,
   DBAT0U = 536,
   DBAT1L = 539,
   DBAT1U = 538,
   DBAT2L = 541,
   DBAT2U = 540,
   DBAT3L = 543,
   DBAT3U = 542,
   DBAT4L = 569,
   DBAT4U = 568,
   DBAT5L = 571,
   DBAT5U = 570,
   DBAT6L = 573,
   DBAT6U = 572,
   DBAT7L = 575,
   DBAT7U = 574,
   DEC = 22,
   DSISR = 18,
   DMAL = 923,
   DMAU = 922,
   EAR = 282,
   GQR0 = 912,
   GQR1 = 913,
   GQR2 = 914,
   GQR3 = 915,
   GQR4 = 916,
   GQR5 = 917,
   GQR6 = 918,
   GQR7 = 919,
   HID0 = 1008,
   HID1 = 1009,
   HID2 = 920,
   HID4 = 1011,
   IABR = 1010,
   IBAT0L = 529,
   IBAT0U = 528,
   IBAT1L = 531,
   IBAT1U = 530,
   IBAT2L = 533,
   IBAT2U = 532,
   IBAT3L = 535,
   IBAT3U = 534,
   IBAT4L = 561,
   IBAT4U = 560,
   IBAT5L = 563,
   IBAT5U = 562,
   IBAT6L = 565,
   IBAT6U = 564,
   IBAT7L = 567,
   IBAT7U = 566,
   ICTC = 1019,
   L2CR = 1017,
   LR = 8,
   MMCR0 = 952,
   MMCR1 = 956,
   PMC1 = 953,
   PMC2 = 954,
   PMC3 = 957,
   PMC4 = 958,
   PVR = 287,
   SDR1 = 25,
   SIA = 955,
   SPRG0 = 272,
   SPRG1 = 273,
   SPRG2 = 274,
   SPRG3 = 275,
   SRR0 = 26,
   SRR1 = 27,
   TBL = 268,
   TBLW = 284,
   TBU = 269,
   TBUW = 285,
   TDCL = 1012,
   TDCH = 1018,
   THRM1 = 1020,
   THRM2 = 1021,
   THRM3 = 1022,
   UMMCR0 = 936,
   UMMCR1 = 940,
   UPMC1 = 937,
   UPMC2 = 938,
   UPMC3 = 941,
   UPMC4 = 942,
   USIA = 939,
   WPAR = 921,
   XER = 1
};

// Thread registers
// TODO: Some system registers may not be thread-specific!
struct ThreadState
{
   struct Module *module;

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

   // Reserve for lwarx / stwcx.
   bool reserve;
   uint32_t reserveAddress;
};
