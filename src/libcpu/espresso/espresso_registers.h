#pragma once
#include <atomic>
#include <cstdint>

namespace espresso
{

// General Purpose Integer Registers
using gpr_t = uint32_t;

namespace ConditionRegisterFlag_
{
enum Value : uint32_t
{
   NegativeShift        = 3,
   PositiveShift        = 2,
   ZeroShift            = 1,
   SummaryOverflowShift = 0,

   Negative             = 1 << NegativeShift,
   Positive             = 1 << PositiveShift,
   Zero                 = 1 << ZeroShift,
   SummaryOverflow      = 1 << SummaryOverflowShift,

   LessThan             = Negative,
   GreaterThan          = Positive,
   Equal                = Zero,
   Unordered            = SummaryOverflow,

   FloatingPointException           = Negative,
   FloatingPointExceptionEnabled    = Positive,
   FloatingPointInvalidException    = Zero,
   FloatingPointOverflowException   = SummaryOverflow,
};
}
using ConditionRegisterFlag = ConditionRegisterFlag_::Value;

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

// Floating-Point Registers
union alignas(16) fpr_t
{
   struct
   {
      double value;
   };

   struct
   {
      double paired0;  // Retains precision of a loaded double value.
      double paired1;
   };

   struct
   {
      uint64_t idw;
      uint64_t idw_paired1;
   };

   struct
   {
      uint32_t iw1;
      uint32_t iw0;
   };
};

// Floating-Point Result Flags for fpscr.fprf
namespace FloatingPointResultFlags_
{
enum Value : uint32_t
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
using FloatingPointResultFlags = FloatingPointResultFlags_::Value;

// Rounding Mode for fpscr.rn
namespace FloatingPointRoundMode_
{
enum Value : uint32_t
{
   Nearest  = 0,
   Zero     = 1,
   Positive = 2,
   Negative = 3
};
}
using FloatingPointRoundMode = FloatingPointRoundMode_::Value;

// Bit positions and masks for fpscr
namespace FPSCRRegisterBits_
{
enum Value : uint32_t {
   FXShift     = 31,
   FEXShift    = 30,
   VXShift     = 29,
   OXShift     = 28,
   UXShift     = 27,
   ZXShift     = 26,
   XXShift     = 25,
   VXSNANShift = 24,
   VXISIShift  = 23,
   VXIDIShift  = 22,
   VXZDZShift  = 21,
   VXIMZShift  = 20,
   VXVCShift   = 19,
   VXSOFTShift = 10,
   VXSQRTShift = 9,
   VXCVIShift  = 8,

   FX       = 1u << FXShift,
   FEX      = 1u << FXShift,
   VX       = 1u << FXShift,
   OX       = 1u << OXShift,
   UX       = 1u << UXShift,
   ZX       = 1u << ZXShift,
   XX       = 1u << XXShift,
   VXSNAN   = 1u << VXSNANShift,
   VXISI    = 1u << VXISIShift,
   VXIDI    = 1u << VXIDIShift,
   VXZDZ    = 1u << VXZDZShift,
   VXIMZ    = 1u << VXIMZShift,
   VXVC     = 1u << VXVCShift,
   VXSOFT   = 1u << VXSOFTShift,
   VXSQRT   = 1u << VXSQRTShift,
   VXCVI    = 1u << VXCVIShift,

   AllVX          = VXSNAN | VXISI | VXIDI | VXZDZ | VXIMZ | VXVC | VXSOFT | VXSQRT | VXCVI,
   AllExceptions  = OX | UX | ZX | XX | AllVX,
};
}
using FPSCRRegisterBits = FPSCRRegisterBits_::Value;

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

// gqr.st_type / gqr.ld_type
namespace QuantizedDataType_
{
enum class Value : uint32_t
{
   Floating    = 0,
   Unsigned8   = 4,
   Unsigned16  = 5,
   Signed8     = 6,
   Signed16    = 7
};
}
using QuantizedDataType = QuantizedDataType_::Value;

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

namespace XERegisterBits_
{
enum Value : uint32_t {
   XRShift        = 28,

   XR             = 15u << XRShift,

   CarryShift     = 29,
   OverflowShift  = 30,
   StickyOVShift  = 31,

   Carry          = 1u << CarryShift,
   Overflow       = 1u << OverflowShift,
   StickyOV       = 1u << StickyOVShift,
};
}
using XERegisterBits = XERegisterBits_::Value;

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

} // namespace espresso
