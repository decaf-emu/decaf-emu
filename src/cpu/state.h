#pragma once
#include <cstdint>
#include <atomic>

// General Purpose Integer Registers
using gpr_t = uint32_t;

// Floating-Point Registers
struct fpr_t
{
   union {
      struct {
         double value;
      };
      struct {
         float paired1;
         float paired0;
      };
      struct {
         uint64_t idw;
      };
      struct
      {
         uint32_t iw1;
         uint32_t iw0;
      };
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

   Negative = 1 << NegativeShift,
   Positive = 1 << PositiveShift,
   Zero = 1 << ZeroShift,
   SummaryOverflow = 1 << SummaryOverflowShift,

   LessThan = Negative,
   GreaterThan = Positive,
   Equal = Zero,
   Unordered = SummaryOverflow,

   FloatingPointException = Negative,
   FloatingPointExceptionEnabled = Positive,
   FloatingPointInvalidException = Zero,
   FloatingPointOverflowException = SummaryOverflow,
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

namespace XERegisterBits
{
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
      uint32_t crxr : 4; // [0-3] condition stuff in xer
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
   ClassDescriptor = 1 << 4,
   Negative = 1 << 3,
   Positive = 1 << 2,
   Zero = 1 << 1,
   NaN = 1 << 0,

   LessThan = Negative,
   GreaterThan = Positive,
   Equal = Zero,
   Unordered = NaN,
};
}

namespace FloatingPointRoundMode
{
enum FloatingPointRoundMode : uint32_t
{
   Nearest = 0,
   Zero = 1,
   Positive = 2,
   Negative = 3
};
}

namespace FPSCRRegisterBits
{
enum FPSCRRegisterBits : uint32_t {
   FXShift = 31,
   FEXShift = 30,
   VXShift = 29,
   OXShift = 28,
   UXShift = 27,
   ZXShift = 26,
   XXShift = 25,
   VXSNANShift = 24,
   VXISIShift = 23,
   VXIDIShift = 22,
   VXZDZShift = 21,
   VXIMZShift = 20,
   VXVCShift = 19,
   VXSOFTShift = 10,
   VXSQRTShift = 9,
   VXCVIShift = 8,

   FX = 1u << FXShift,
   FEX = 1u << FXShift,
   VX = 1u << FXShift,
   OX = 1u << OXShift,
   UX = 1u << UXShift,
   ZX = 1u << ZXShift,
   XX = 1u << XXShift,
   VXSNAN = 1u << VXSNANShift,
   VXISI = 1u << VXISIShift,
   VXIDI = 1u << VXIDIShift,
   VXZDZ = 1u << VXZDZShift,
   VXIMZ = 1u << VXIMZShift,
   VXVC = 1u << VXVCShift,
   VXSOFT = 1u << VXSOFTShift,
   VXSQRT = 1u << VXSQRTShift,
   VXCVI = 1u << VXCVIShift,

   AllVX = VXSNAN | VXISI | VXIDI | VXZDZ | VXIMZ | VXVC | VXSOFT | VXSQRT | VXCVI,
   AllExceptions = OX | UX | ZX | XX | AllVX,
};
}

// Floating-Point Status and Control Register
union fpscr_t
{
   uint32_t value;

   struct
   {
      uint32_t : 12;
      uint32_t fpcc : 4;
      uint32_t : 12;
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
   Floating = 0,
   Unsigned8 = 4,
   Unsigned16 = 5,
   Signed8 = 6,
   Signed16 = 7
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

// http://wiiubrew.org/wiki/SPRs
enum class SprEncoding
{
   XER = 0x1,
   LR = 0x8,
   CTR = 0x9,
   DSISR = 0x12,
   DAR = 0x13,
   DEC = 0x16,
   SDR1 = 0x19,
   SRR0 = 0x1A,
   SRR1 = 0x1B,
   UTBL = 0x10C,
   UTBU = 0x10D,
   SPRG0 = 0x110,
   SPRG1 = 0x111,
   SPRG2 = 0x112,
   SPRG3 = 0x113,
   EAR = 0x11A,
   TBL = 0x11C,
   TBU = 0x11D,
   PVR = 0x11F,
   IBAT0U = 0x210,
   IBAT0L = 0x211,
   IBAT1U = 0x212,
   IBAT1L = 0x213,
   IBAT2U = 0x214,
   IBAT2L = 0x215,
   IBAT3U = 0x216,
   IBAT3L = 0x217,
   DBAT0U = 0x218,
   DBAT0L = 0x219,
   DBAT1U = 0x21A,
   DBAT1L = 0x21B,
   DBAT2U = 0x21C,
   DBAT2L = 0x21D,
   DBAT3U = 0x21E,
   DBAT3L = 0x21F,
   IBAT4U = 0x230,
   IBAT4L = 0x231,
   IBAT5U = 0x232,
   IBAT5L = 0x233,
   IBAT6U = 0x234,
   IBAT6L = 0x235,
   IBAT7U = 0x236,
   IBAT7L = 0x237,
   DBAT4U = 0x238,
   DBAT4L = 0x239,
   DBAT5U = 0x23A,
   DBAT5L = 0x23B,
   DBAT6U = 0x23C,
   DBAT6L = 0x23D,
   DBAT7U = 0x23E,
   DBAT7L = 0x23F,
   UGQR0 = 0x380,
   UGQR1 = 0x381,
   UGQR2 = 0x382,
   UGQR3 = 0x383,
   UGQR4 = 0x384,
   UGQR5 = 0x385,
   UGQR6 = 0x386,
   UGQR7 = 0x387,
   UHID2 = 0x388,
   UWPAR = 0x389,
   UDMAU = 0x38A,
   UDMAL = 0x38B,
   GQR0 = 0x390,
   GQR1 = 0x391,
   GQR2 = 0x392,
   GQR3 = 0x393,
   GQR4 = 0x394,
   GQR5 = 0x395,
   GQR6 = 0x396,
   GQR7 = 0x397,
   HID2 = 0x398,
   WPAR = 0x399,
   DMA_U = 0x39A,
   DMA_L = 0x39B,
   UMMCR0 = 0x3A8,
   UPMC1 = 0x3A9,
   UPMC2 = 0x3AA,
   USIA = 0x3AB,
   UMMCR1 = 0x3AC,
   UPMC3 = 0x3AD,
   UPMC4 = 0x3AE,
   HID5 = 0x3B0,
   PCSR = 0x3B2,
   SCR = 0x3B3,
   CAR = 0x3B4,
   BCR = 0x3B5,
   WPSAR = 0x3B6,
   MMCR0 = 0x3B8,
   PMC1 = 0x3B9,
   PMC2 = 0x3BA,
   SIA = 0x3BB,
   MMCR1 = 0x3BC,
   PMC3 = 0x3BD,
   PMC4 = 0x3BE,
   DCATE = 0x3D0,
   DCATR = 0x3D1,
   DMATL0 = 0x3D8,
   DMATU0 = 0x3D9,
   DMATR0 = 0x3DA,
   DMATL1 = 0x3DB,
   DMATU1 = 0x3DC,
   DMATR1 = 0x3DD,
   UPIR = 0x3EF,
   HID0 = 0x3F0,
   HID1 = 0x3F1,
   IABR = 0x3F2,
   HID4 = 0x3F3,
   TDCL = 0x3F4,
   DABR = 0x3F5,
   L2CR = 0x3F9,
   TDCH = 0x3FA,
   ICTC = 0x3FB,
   THRM1 = 0x3FC,
   THRM2 = 0x3FD,
   THRM3 = 0x3FE,
   PIR = 0x3FF,
};

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
