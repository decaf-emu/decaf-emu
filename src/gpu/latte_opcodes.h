#pragma once
#include <cstdint>
#include <vector>
#include <map>

#pragma pack(push, 1)

namespace latte
{

static const uint32_t NumGPR = 128;
static const uint32_t NumTempRegisters = 16; // TODO: Find correct value!

static const uint32_t WordsPerSlot = 2; // 1 slot is 2x 32bit words

static const uint32_t SlotsPerCF = 1;
static const uint32_t SlotsPerALU = 1;
static const uint32_t SlotsPerTEX = 2; // Only uses 98 bits of data though
static const uint32_t SlotsPerMEM = 2; // TODO: Verify
static const uint32_t SlotsPerVTX = 2; // TODO: Verify

static const uint32_t WordsPerCF = SlotsPerCF * WordsPerSlot;
static const uint32_t WordsPerALU = SlotsPerALU * WordsPerSlot;
static const uint32_t WordsPerTEX = SlotsPerTEX * WordsPerSlot;
static const uint32_t WordsPerMEM = SlotsPerMEM * WordsPerSlot;
static const uint32_t WordsPerVTX = SlotsPerVTX * WordsPerSlot;

namespace cf
{

namespace Type
{
enum Type : uint32_t
{
   Normal         = 0,
   Export         = 1,
   Alu            = 2,
   AluExtended    = 3
};
}

namespace KcacheMode
{
enum KcacheMode : uint32_t
{
   NOP            = 0,
   Lock1          = 1,
   Lock2          = 2,
   LockLoopindex  = 3
};
}

namespace Cond
{
enum Cond : uint32_t
{
   Active         = 0,
   False          = 1,
   Bool           = 2,
   NotBool        = 3
};
}

struct Word0
{
   uint32_t addr;
};

struct Word1
{
   uint32_t popCount : 3;
   uint32_t cfConst : 5;
   uint32_t cond : 2;
   uint32_t count : 3;
   uint32_t callCount : 6;
   uint32_t : 2;
   uint32_t endOfProgram : 1;
   uint32_t validPixelMode : 1;
   uint32_t inst : 7;
   uint32_t wholeQuadMode : 1;
   uint32_t barrier : 1;
};

struct AluWord0
{
   uint32_t addr : 16;
   uint32_t kcacheBank0 : 4;
   uint32_t kcacheBank1 : 2;
   uint32_t kcacheMode0 : 4;
};

struct AluWord1
{
   uint32_t kcacheMode1 : 2;
   uint32_t kcacheAddr0 : 8;
   uint32_t kcacheAddr1 : 8;
   uint32_t count : 7;
   uint32_t altConst : 1;
   uint32_t inst : 4;
   uint32_t wholeQuadMode : 1;
   uint32_t barrier : 1;
};

struct ExpWord0
{
   uint32_t dstReg : 13;
   uint32_t type : 2;
   uint32_t srcReg : 7;
   uint32_t srcRel : 1;
   uint32_t indexGpr : 7;
   uint32_t elemSize : 2;
};

struct ExpWord1
{
   uint32_t srcSelX : 3;
   uint32_t srcSelY : 3;
   uint32_t srcSelZ : 3;
   uint32_t srcSelW : 3;
   uint32_t : 9;
   uint32_t endOfProgram : 1;
   uint32_t validPixelMode : 1;
   uint32_t inst : 7;
   uint32_t wholeQuadMode : 1;
   uint32_t barrier : 1;
};

union Instruction
{
   struct
   {
      uint32_t : 32;
      uint32_t : 28;
      uint32_t type : 2;
      uint32_t : 2;
   };

   struct
   {
      Word0 word0;
      Word1 word1;
   };

   struct
   {
      AluWord0 aluWord0;
      AluWord1 aluWord1;
   };

   struct
   {
      ExpWord0 expWord0;
      ExpWord1 expWord1;
   };
};

enum inst : uint32_t
{
#define CF_INST(name, value) name = value,
#include "latte_opcodes_def.inl"
#undef CF_INST
};

extern std::map<uint32_t, const char *> name;

} // namespace cf

namespace exp
{

namespace Type
{
enum Type
{
   Pixel          = 0,
   Position       = 1,
   Parameter      = 2,
   Write          = 0,
   WriteInd       = 1,
   WriteAck       = 2,
   WriteIndAck    = 3,
};
}

enum inst : uint32_t
{
#define EXP_INST(name, value) name = value,
#include "latte_opcodes_def.inl"
#undef EXP_INST
};

extern std::map<uint32_t, const char *> name;

} // namespace exp

namespace alu
{

namespace Encoding
{
enum Encoding
{
   OP2                  = 0,
   OP3                  = 1
};
}

namespace Source
{
enum Source : uint32_t
{
   RegisterFirst        = 0,
   RegisterLast         = 127,

   KcacheBank0First     = 128,
   KcacheBank0Last      = 159,

   KcacheBank1First     = 160,
   KcacheBank1Last      = 191,

   Src1DoubleLSW        = 244, // Constant double 1.0, float LSW
   Src1DoubleMSW        = 245, // Constant double 1.0, float LSW
   Src05DoubleLSW       = 246, // Constant double 0.5, float LSW
   Src05DoubleMSW       = 247, // Constant double 0.5, float MSW

   Src0Float            = 248, // Constant 0.0 float
   Src1Float            = 249, // Constant 1.0 float
   Src1Integer          = 250, // Constant 1 integer
   SrcMinus1Integer     = 251, // Constant -1 integer

   Src05Float           = 252, // Constant 0.5 float
   SrcLiteral           = 253, // Literal constant
   SrcPreviousVector    = 254, // Previous vector result
   SrcPreviousScalar    = 255, // Previous scalar result

   CfileConstantsFirst  = 256,
   CfileConstantsLast   = 511,
};
}

namespace Channel
{
enum Channel : uint32_t
{
   X        = 0,
   Y        = 1,
   Z        = 2,
   W        = 3,
   Unknown
};
}

namespace Select
{
enum Select : uint32_t
{
   X        = 0,
   Y        = 1,
   Z        = 2,
   W        = 3,
   Zero     = 4,
   One      = 5,
   Mask     = 7,
   Unknown
};
}

namespace BankSwizzle
{
enum BankSwizzle : uint32_t
{
   Vec012   = 0,
   Vec021   = 1,
   Vec120   = 2,
   Vec102   = 3,
   Vec201   = 4,
   Vec210   = 5
};
}

namespace IndexMode
{
enum IndexMode : uint32_t
{
   ArX         = 0,  // Add AR.X
   ArY         = 1,  // Add AR.Y
   ArZ         = 2,  // Add AR.Z
   ArW         = 3,  // Add AR.W
   Loop        = 4,  // Add loop index (aL)
   Global      = 5,  // Treat GPR as absolute, not thread-relative
   GlobalArX   = 6   // Treat GPR as absolute and add AR.x
};
}

namespace PredicateSelect
{
enum PredicateSelect : uint32_t
{
   Off         = 0,  // Execute all pixels
   Zero        = 2,  // Execute if predicate = 0
   One         = 3   // Execute if predicate = 1
};
}

namespace OutputModifier
{
enum OutputModifier : uint32_t
{
   Off         = 0,
   Multiply2   = 1,
   Multiply4   = 2,
   Divide2     = 3
};
}

struct Word0
{
   uint32_t src0Sel : 9;
   uint32_t src0Rel : 1;
   uint32_t src0Chan : 2;
   uint32_t src0Neg : 1;
   uint32_t src1Sel : 9;
   uint32_t src1Rel : 1;
   uint32_t src1Chan : 2;
   uint32_t src1Neg : 1;
   uint32_t indexMode : 3;
   uint32_t predSel : 2;
   uint32_t last : 1;
};

struct Word1
{
   uint32_t : 15;
   uint32_t encoding : 3;
   uint32_t bankSwizzle : 3;
   uint32_t dstGpr : 7;
   uint32_t dstRel : 1;
   uint32_t dstChan : 2;
   uint32_t clamp : 1;
};

struct Word1Op2
{
   uint32_t src0Abs : 1;
   uint32_t src1Abs : 1;
   uint32_t updateExecuteMask : 1;
   uint32_t updatePred : 1;
   uint32_t writeMask : 1;
   uint32_t omod : 2;
   uint32_t inst : 11;
   uint32_t : 14;
};

struct Word1Op3
{
   uint32_t src2Sel : 9;
   uint32_t src2Rel : 1;
   uint32_t src2Chan : 2;
   uint32_t src2Neg : 1;
   uint32_t inst : 5;
   uint32_t : 14;
};

struct Instruction
{
   Word0 word0;

   union
   {
      Word1 word1;
      Word1Op2 op2;
      Word1Op3 op3;
   };
};

enum inst : uint32_t
{
#define ALU_INST(name, value) name = value,
#include "latte_opcodes_def.inl"
#undef ALU_INST
};

enum op2 : uint32_t
{
#define ALU_OP2(name, value, srcs, flags) name = value,
#include "latte_opcodes_def.inl"
#undef ALU_OP2
};

enum op3 : uint32_t
{
#define ALU_OP3(name, value, srcs, flags) name = value,
#include "latte_opcodes_def.inl"
#undef ALU_OP3
};

struct Opcode
{
   enum Flags
   {
      Vector         = (1 << 0),
      Transcendental = (1 << 1),
      Reduction      = (1 << 2),
      PredSet        = (1 << 3),
      IntIn          = (1 << 4),
      IntOut         = (1 << 5),
      UintIn         = (1 << 6),
      UintOut        = (1 << 7),
   };

   uint32_t id;
   uint32_t srcs;
   Flags flags;
   const char *name;
};

extern std::map<uint32_t, const char *> name;
extern std::map<uint32_t, Opcode> op2info;
extern std::map<uint32_t, Opcode> op3info;

} // namespace alu

namespace tex
{

struct Word0
{
   uint32_t inst : 5;
   uint32_t bcFracMode : 1;
   uint32_t : 1;
   uint32_t fetchWholeQuad : 1;
   uint32_t resourceID : 8;
   uint32_t srcReg : 7;
   uint32_t srcRel : 1;
   uint32_t altConst : 1;
   uint32_t : 7;
};

struct Word1
{
   uint32_t dstReg : 7;
   uint32_t dstRel : 1;
   uint32_t : 1;
   uint32_t dstSelX : 3;
   uint32_t dstSelY : 3;
   uint32_t dstSelZ : 3;
   uint32_t dstSelW : 3;
   uint32_t lodBias : 7;
   uint32_t coordTypeX : 1;
   uint32_t coordTypeY : 1;
   uint32_t coordTypeZ : 1;
   uint32_t coordTypeW : 1;
};

struct Word2
{
   uint32_t offsetX : 5;
   uint32_t offsetY : 5;
   uint32_t offsetZ : 5;
   uint32_t samplerID : 5;
   uint32_t srcSelX : 3;
   uint32_t srcSelY : 3;
   uint32_t srcSelZ : 3;
   uint32_t srcSelW : 3;
};

struct Instruction
{
   Word0 word0;
   Word1 word1;
   Word2 word2;
};

enum inst : uint32_t
{
#define TEX_INST(name, value) name = value,
#include "latte_opcodes_def.inl"
#undef TEX_INST
};

extern std::map<uint32_t, const char *> name;

} // namespace tex

namespace vtx
{

namespace FetchType
{
enum FetchType
{
   VertexData     = 0,
   InstanceData   = 1,
   NoIndexOffset  = 2,
};
}

enum DataFormat
{
   FMT_INVALID                = 0,
   FMT_8                      = 1,
   FMT_4_4                    = 2,
   FMT_3_3_2                  = 3,
   FMT_RESERVED_4             = 4,
   FMT_16                     = 5,
   FMT_16_FLOAT               = 6,
   FMT_8_8                    = 7,
   FMT_5_6_5                  = 8,
   FMT_6_5_5                  = 9,
   FMT_1_5_5_5                = 10,
   FMT_4_4_4_4                = 11,
   FMT_5_5_5_1                = 12,
   FMT_32                     = 13,
   FMT_32_FLOAT               = 14,
   FMT_16_16                  = 15,
   FMT_16_16_FLOAT            = 16,
   FMT_8_24                   = 17,
   FMT_8_24_FLOAT             = 18,
   FMT_24_8                   = 19,
   FMT_24_8_FLOAT             = 20,
   FMT_10_11_11               = 21,
   FMT_10_11_11_FLOAT         = 22,
   FMT_11_11_10               = 23,
   FMT_11_11_10_FLOAT         = 24,
   FMT_2_10_10_10             = 25,
   FMT_8_8_8_8                = 26,
   FMT_10_10_10_2             = 27,
   FMT_X24_8_32_FLOAT         = 28,
   FMT_32_32                  = 29,
   FMT_32_32_FLOAT            = 30,
   FMT_16_16_16_16            = 31,
   FMT_16_16_16_16_FLOAT      = 32,
   FMT_RESERVED_33            = 33,
   FMT_32_32_32_32            = 34,
   FMT_32_32_32_32_FLOAT      = 35,
   FMT_RESERVED_36            = 36,
   FMT_1                      = 37,
   FMT_1_REVERSED             = 38,
   FMT_GB_GR                  = 39,
   FMT_BG_RG                  = 40,
   FMT_32_AS_8                = 41,
   FMT_32_AS_8_8              = 42,
   FMT_5_9_9_9_SHAREDEXP      = 43,
   FMT_8_8_8                  = 44,
   FMT_16_16_16               = 45,
   FMT_16_16_16_FLOAT         = 46,
   FMT_32_32_32               = 47,
   FMT_32_32_32_FLOAT         = 48,
   FMT_BC1                    = 49,
   FMT_BC2                    = 50,
   FMT_BC3                    = 51,
   FMT_BC4                    = 52,
   FMT_BC5                    = 53,
   FMT_APC0                   = 54,
   FMT_APC1                   = 55,
   FMT_APC2                   = 56,
   FMT_APC3                   = 57,
   FMT_APC4                   = 58,
   FMT_APC5                   = 59,
   FMT_APC6                   = 60,
   FMT_APC7                   = 61,
   FMT_CTX1                   = 62,
   FMT_RESERVED_63            = 63
};

struct Word0
{
   uint32_t inst : 5;
   uint32_t fetchType : 2;
   uint32_t fetchWholeQuad : 1;
   uint32_t bufferID : 8;
   uint32_t srcGpr : 7;
   uint32_t srcRel : 1;
   uint32_t srcSelX : 2;
   uint32_t megaFetchCount : 6;
};

struct Word1
{
   uint32_t : 9;
   uint32_t dstSelX : 3;
   uint32_t dstSelY : 3;
   uint32_t dstSelZ : 3;
   uint32_t dstSelW : 3;
   uint32_t useConstFields : 1;
   uint32_t dataFormat : 6;
   uint32_t numFormatAll : 2;
   uint32_t formatCompAll : 1;
   uint32_t srfModeAll : 1;
};

struct Word1Gpr
{
   uint32_t dstGpr : 7;
   uint32_t dstRel : 1;
   uint32_t : 24;
};

struct Word1Sem
{
   uint32_t semanticID : 8;
   uint32_t : 24;
};

struct Word2
{
   uint32_t offset : 16;
   uint32_t endianSwap : 2;
   uint32_t constBufNoStride : 1;
   uint32_t megaFetch : 1;
   uint32_t altConst : 1;
   uint32_t : 11;
};

struct Instruction
{
   Word0 word0;

   union
   {
      Word1 word1;
      Word1Gpr word1gpr;
      Word1Sem word1sem;
   };

   Word2 word2;
};

extern std::map<uint32_t, const char *> name;

} // namespace vtx

namespace mem
{

extern std::map<uint32_t, const char *> name;

} // namespace mem

} // namespace latte

#pragma pack(pop)
