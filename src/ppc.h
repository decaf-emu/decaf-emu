#pragma once
#include <cstdint>

// General Purpose Integer Registers
using gpr_t = uint32_t;

// Floating-Point Registers
struct fpr_t
{
   union {
      struct {
         double paired0;
         double paired1;
      };
      struct {
         uint64_t idw;
         uint64_t idw1;
      };
      struct
      {
         uint32_t iw0;
         uint32_t iw1;
         uint32_t iw2;
         uint32_t iw3;
      };
      struct {
         uint64_t value0;
         uint64_t value1;
      };
      struct {
         uint64_t data[2];
      } value;
   };
};

namespace ConditionRegisterFlag
{
enum ConditionRegisterFlag : uint32_t
{
   NegativeShift = 3,
   PositiveShift = 2,
   ZeroShift = 1,
   SummaryOverflowShift = 0,

   Negative          = 1 << NegativeShift,
   Positive          = 1 << PositiveShift,
   Zero              = 1 << ZeroShift,
   SummaryOverflow   = 1 << SummaryOverflowShift,

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

namespace XERegisterBits {
enum XERegisterBits : uint32_t {
   XRShift = 28,

   XR = 0xFu << XRShift,

   CarryShift = 29,
   OverflowShift = 30,
   StickyOVShift = 31,

   Carry = 1u << CarryShift,
   Overflow = 1u << OverflowShift,
   StickyOV = 1u << StickyOVShift,
};
}

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

// gqr.st_type / gqr.ld_type
enum class QuantizedDataType : uint32_t
{
   Floating    = 0,
   Unsigned8   = 4,
   Unsigned16  = 5,
   Signed8     = 6,
   Signed16    = 7
};

// Graphics Quantization Registers
union gqr_t
{
   uint32_t value;

   struct
   {
      uint32_t st_type : 3;
      uint32_t : 5;
      uint32_t st_scale : 6;
      uint32_t : 2;
      uint32_t ld_type : 3;
      uint32_t : 5;
      uint32_t ld_scale : 6;
      uint32_t : 2;
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
   GQR0 = 0x380, // guessed!
   GQR1 = 0x381, // OSSaveContext
   GQR2 = 0x382, // OSSaveContext
   GQR3 = 0x383, // OSSaveContext
   GQR4 = 0x384, // OSSaveContext
   GQR5 = 0x385, // OSSaveContext
   GQR6 = 0x386, // OSSaveContext
   GQR7 = 0x387, // OSSaveContext
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
   MMCR0 = 0x3a8, // OSSaveContext
   MMCR1 = 0x3ac, // OSSaveContext
   PMC1 = 0x3a9, // OSSaveContext
   PMC2 = 0x3aa, // OSSaveContext
   PMC3 = 0x3ad, // OSSaveContext
   PMC4 = 0x3ae, // OSSaveContext
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

   // Reserve for lwarx / stwcx.
   bool reserve;
   uint32_t reserveAddress;
   uint32_t reserveData;
};

uint32_t
getCRF(ThreadState *state, uint32_t field);

void
setCRF(ThreadState *state, uint32_t field, uint32_t value);

uint32_t
getCRB(ThreadState *state, uint32_t bit);

void
setCRB(ThreadState *state, uint32_t bit, uint32_t value);
