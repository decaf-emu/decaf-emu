#pragma once
#include <atomic>
#include <cstdint>
#include <common/enum_start.h>

namespace espresso
{

// General Purpose Integer Registers
using Register = uint32_t;


/**
 * Condition Register
 */
union ConditionRegister
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

/**
 * Condition Register Flags
 *
 * Value of each cr field cr0...cr7
 */
FLAGS_BEG(ConditionRegisterFlag, uint32_t)
   FLAGS_VALUE(SummaryOverflow,                    1 << 0)
   FLAGS_VALUE(Zero,                               1 << 1)
   FLAGS_VALUE(Positive,                           1 << 2)
   FLAGS_VALUE(Negative,                           1 << 3)

   FLAGS_VALUE(Unordered,                          SummaryOverflow)
   FLAGS_VALUE(Equal,                              Zero)
   FLAGS_VALUE(GreaterThan,                        Positive)
   FLAGS_VALUE(LessThan,                           Negative)

   FLAGS_VALUE(FloatingPointOverflowException,     SummaryOverflow)
   FLAGS_VALUE(FloatingPointInvalidException,      Zero)
   FLAGS_VALUE(FloatingPointExceptionEnabled,      Positive)
   FLAGS_VALUE(FloatingPointException,             Negative)
FLAGS_END(ConditionRegisterFlag)


/**
 * Floating-Point Registers
 */
union alignas(16) FloatingPointRegister
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


/**
 * Floating-Point Status and Control Register
 */
union FloatingPointStatusAndControlRegister
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

/**
 * Floating-Point Status and Control Register Flags
 */
FLAGS_BEG(FpscrFlags, uint32_t)
   FLAGS_VALUE(VXCVI,                              1u << 8)
   FLAGS_VALUE(VXSQRT,                             1u << 9)
   FLAGS_VALUE(VXSOFT,                             1u << 10)

   FLAGS_VALUE(VXVC,                               1u << 19)
   FLAGS_VALUE(VXIMZ,                              1u << 20)
   FLAGS_VALUE(VXZDZ,                              1u << 21)
   FLAGS_VALUE(VXIDI,                              1u << 22)
   FLAGS_VALUE(VXISI,                              1u << 23)
   FLAGS_VALUE(VXSNAN,                             1u << 24)
   FLAGS_VALUE(XX,                                 1u << 25)
   FLAGS_VALUE(ZX,                                 1u << 26)
   FLAGS_VALUE(UX,                                 1u << 27)
   FLAGS_VALUE(OX,                                 1u << 28)
   FLAGS_VALUE(VX,                                 1u << 29)
   FLAGS_VALUE(FEX,                                1u << 30)
   FLAGS_VALUE(FX,                                 1u << 31)

   FLAGS_VALUE(AllVX,            VXSNAN | VXISI | VXIDI | VXZDZ | VXIMZ |
                                 VXVC | VXSOFT | VXSQRT | VXCVI)
   FLAGS_VALUE(AllExceptions,    AllVX | OX | UX | ZX | XX)
ENUM_END(FpscrFlags)

/**
 * Floating-Point Result Flags
 *
 * Value of fpscr.fprf
 */
FLAGS_BEG(FloatingPointResultFlags, uint32_t)
   FLAGS_VALUE(NaN,                                1 << 0)
   FLAGS_VALUE(Zero,                               1 << 1)
   FLAGS_VALUE(Positive,                           1 << 2)
   FLAGS_VALUE(Negative,                           1 << 3)
   FLAGS_VALUE(ClassDescriptor,                    1 << 4)

   FLAGS_VALUE(Unordered,                          NaN)
   FLAGS_VALUE(Equal,                              Zero)
   FLAGS_VALUE(GreaterThan,                        Positive)
   FLAGS_VALUE(LessThan,                           Negative)
FLAGS_END(FloatingPointResultFlags)

/**
 * Floating-Point Rounding Mode
 *
 * Value of fpscr.rn
 */
ENUM_BEG(FloatingPointRoundMode, uint32_t)
   ENUM_VALUE(Nearest,                             0)
   ENUM_VALUE(Zero,                                1)
   ENUM_VALUE(Positive,                            2)
   ENUM_VALUE(Negative,                            3)
ENUM_END(FloatingPointRoundMode)


/**
 * Graphics Quantization Registers
 */
union GraphicsQuantisationRegister
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


/**
 * Graphics Quantization Registers Quantized Data Type
 *
 * Value of gqr.st_type / gqr.ld_type
 */
ENUM_BEG(QuantizedDataType, uint32_t)
   ENUM_VALUE(Floating,                            0)
   ENUM_VALUE(Unsigned8,                           4)
   ENUM_VALUE(Unsigned16,                          5)
   ENUM_VALUE(Signed8,                             6)
   ENUM_VALUE(Signed16,                            7)
ENUM_END(QuantizedDataType)


/**
 * Machine State Register
 */
union MachineStateRegister
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


/**
 * Processor Version Register
 */
union ProcessorVersionRegister
{
   uint32_t value;

   struct
   {
      uint32_t revision : 16;
      uint32_t version : 16;
   };
};


/**
 * Fixed Point Exception Register
 */
union FixedPointExceptionRegister
{
   uint32_t value;

   struct
   {
      // Byte count for lmwx, stmwx
      uint32_t byteCount : 7;

      uint32_t : 22;

      //! Carry
      uint32_t ca : 1;

      //! Overflow
      uint32_t ov : 1;

      //! Sticky overflow
      uint32_t so : 1;
   };

   struct
   {
      uint32_t : 28;
      uint32_t crxr : 4;
   };
};

} // namespace espresso

#include <common/enum_end.h>
