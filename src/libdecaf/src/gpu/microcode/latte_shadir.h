#pragma once
#include <array>
#include <memory>
#include <vector>
#include <unordered_set>
#include "common/types.h"
#include "latte_instructions.h"

namespace latte
{

namespace shadir
{

struct Instruction
{
   enum Type
   {
      Invalid,
      CF,
      CF_ALU,
      ALU,
      ALU_REDUCTION,
      TEX,
      VTX,
      EXP,
   };

   Instruction(Type type) :
      type(type)
   {
   }

   virtual ~Instruction() = default;

   Type type = Invalid;
   const char *name = nullptr;
   uint32_t cfPC = -1;
   uint32_t groupPC = -1;
};

struct CfInstruction : Instruction
{
   CfInstruction() :
      Instruction(Instruction::CF)
   {
   }

   virtual ~CfInstruction() override = default;

   SQ_CF_INST id;
   SQ_CF_COND cond = SQ_CF_COND_ACTIVE;

   uint32_t addr = 0;
   uint32_t popCount = 0;
   uint32_t callCount = 0;
   uint32_t constant = 0;

   bool validPixelMode = false;
   bool wholeQuadMode = false;
   bool barrier = false;

   std::vector<std::unique_ptr<Instruction>> clause;
};

enum class ValueType
{
   Float,
   Uint,
   Int,
};

struct AluSource
{
   SQ_ALU_SRC sel;
   SQ_REL rel;
   SQ_CHAN chan;
   ValueType type;
   bool negate;
   bool absolute;

   union
   {
      uint32_t literalUint;
      int32_t literalInt;
      float literalFloat;
   };
};

struct AluDest
{
   uint32_t sel;
   SQ_REL rel;
   SQ_CHAN chan;
   bool clamp;
   ValueType type;
};

struct AluInstruction : Instruction
{
   AluInstruction() :
      Instruction(Instruction::ALU)
   {
   }

   virtual ~AluInstruction() override = default;

   struct CfAluInstruction *parent = nullptr;

   union
   {
      SQ_OP2_INST op2;
      SQ_OP3_INST op3;
   };

   SQ_ALU_ENCODING encoding;
   SQ_ALU_FLAGS flags = SQ_ALU_FLAG_NONE;
   SQ_CHAN unit;
   SQ_ALU_VEC_BANK_SWIZZLE bankSwizzle;
   SQ_PRED_SEL predSel;
   SQ_INDEX_MODE indexMode;

   bool updateExecuteMask = false;
   bool updatePredicate = false;
   bool writeMask = true;
   bool isReduction = false;

   uint32_t srcCount;
   std::array<AluSource, 3> src;
   AluDest dst;

   union
   {
      SQ_ALU_OMOD outputModifier = SQ_ALU_OMOD_OFF;
      SQ_ALU_EXECUTE_MASK_OP executeMaskOP;
   };
};

struct AluReductionInstruction : Instruction
{
   AluReductionInstruction() :
      Instruction(Instruction::ALU_REDUCTION)
   {
   }

   virtual ~AluReductionInstruction() override = default;

   std::array<AluInstruction *, 4> units;
   SQ_OP2_INST op2;
};

struct TextureFetchRegister
{
   uint32_t id;
   SQ_REL rel;
   std::array<SQ_SEL, 4> sel;
};

struct TextureFetchInstruction : Instruction
{
   TextureFetchInstruction() :
      Instruction(Instruction::TEX)
   {
   }

   virtual ~TextureFetchInstruction() override = default;

   SQ_TEX_INST id;

   uint32_t resourceID;
   uint32_t samplerID;

   TextureFetchRegister src;
   TextureFetchRegister dst;

   int32_t lodBias;
   std::array<int32_t, 3> offset;
   std::array<bool, 4> coordNormalise;

   bool bcFracMode;
   bool fetchWholeQuad;
   bool altConst;
};

struct VertexFetchSrc
{
   uint32_t id;
   SQ_REL rel;
   SQ_SEL sel;
};

struct VertexFetchDst
{
   uint32_t id;
   SQ_REL rel;
   std::array<SQ_SEL, 4> sel;
};

struct VertexFetchInstruction : Instruction
{
   VertexFetchInstruction() :
      Instruction(Instruction::VTX)
   {
   }

   virtual ~VertexFetchInstruction() override = default;

   SQ_VTX_INST id;
   SQ_VTX_FETCH_TYPE fetchType;

   SQ_DATA_FORMAT dataFormat;
   SQ_NUM_FORMAT numFormat;
   SQ_FORMAT_COMP formatComp;
   SQ_SRF_MODE srfMode;
   SQ_ENDIAN endian;

   uint32_t bufferID;
   VertexFetchSrc src;
   VertexFetchDst dst;

   uint32_t offset;
   uint32_t megaFetchCount;

   bool fetchWholeQuad;
   bool useConstFields;
   bool constBufNoStride;
   bool altConst;
   bool megaFetch;
};

struct ExportRW
{
   uint32_t id = 0;
   SQ_REL rel = SQ_ABSOLUTE;
};

struct ExportInstruction : Instruction
{
   ExportInstruction() :
      Instruction(Instruction::EXP)
   {
   }

   virtual ~ExportInstruction() override = default;

   SQ_CF_EXP_INST id;
   SQ_EXPORT_TYPE exportType;

   ExportRW rw;

   uint32_t arrayBase;
   uint32_t index;
   uint32_t elemSize;
   uint32_t burstCount;

   bool validPixelMode;
   bool wholeQuadMode;
   bool barrier;
   bool isSemantic = true;

   // EXP_*
   std::array<SQ_SEL, 4> srcSel;

   // MEM_*
   uint32_t arraySize = 0;
   uint32_t compMask = 0;
};

struct KCache
{
   uint32_t bank;
   SQ_CF_KCACHE_MODE mode;
   uint32_t addr;
};

struct CfAluInstruction : Instruction
{
   CfAluInstruction() :
      Instruction(Instruction::CF_ALU)
   {
   }

   virtual ~CfAluInstruction() override = default;

   SQ_CF_ALU_INST id;

   uint32_t addr;
   std::array<KCache, 2> kcache;

   bool altConst;
   bool wholeQuadMode;
   bool barrier;

   std::vector<std::unique_ptr<Instruction>> clause;
};

struct Block
{
   enum Type
   {
      CodeBlock,
      Loop,
      Conditional
   };

   Block(Type type) :
      type(type)
   {
   }

   virtual ~Block() = default;

   Type type;
};

using BlockList = std::vector<std::unique_ptr<Block>>;

struct CodeBlock : Block
{
   CodeBlock() :
      Block(Block::CodeBlock)
   {
   }

   virtual ~CodeBlock() override = default;

   std::vector<Instruction *> code; // Non-owning
};

struct LoopBlock : Block
{
   LoopBlock() :
      Block(Block::Loop)
   {
   }

   virtual ~LoopBlock() override = default;

   BlockList inner;
};

struct ConditionalBlock : Block
{
   ConditionalBlock(Instruction *condition) :
      Block(Block::Conditional),
      condition(condition)
   {
   }

   virtual ~ConditionalBlock() override = default;

   Instruction *condition; // Non-owning
   BlockList inner;
   BlockList innerElse;
};

} // namespace shadir2

struct Shader
{
   enum Type
   {
      Unknown,
      Pixel,
      Vertex,
      Fetch,
      Geometry,
   };

   Type type = Shader::Unknown;
   std::vector<std::unique_ptr<shadir::Instruction>> code;
   std::vector<shadir::Instruction *> linear;
   std::vector<shadir::ExportInstruction *> exports;
   std::vector<std::unique_ptr<shadir::Instruction>> custom;
   std::vector<std::unique_ptr<shadir::Block>> blocks;

   // Store any error/warning messages from decoding
   std::vector<std::string> decodeMessages;

   // Information about registers used
   std::unordered_set<uint32_t> pvUsed;         // Set of Previous Vectors used
   std::unordered_set<uint32_t> psUsed;         // Set of Previous Scalars used
   std::unordered_set<uint32_t> gprsUsed;       // Set of General Puprose Registers used
   std::unordered_set<uint32_t> uniformsUsed;   // Set of Uniforms used
   std::unordered_set<uint32_t> samplersUsed;   // Set of Sampler IDs used
   std::unordered_set<uint32_t> resourcesUsed;  // Set of Resource IDs used
};

} // namespace latte
