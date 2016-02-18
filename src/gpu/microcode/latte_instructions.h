#pragma once
#include "types.h"
#include "gpu/latte_enum_sq.h"

namespace latte
{

enum SQ_CF_INST : uint32_t
{
#define CF_INST(name, value) SQ_CF_INST_##name = value,
#include "latte_instructions_def.inl"
#undef CF_INST
};

enum SQ_CF_EXP_INST : uint32_t
{
#define EXP_INST(name, value) SQ_CF_INST_##name = value,
#include "latte_instructions_def.inl"
#undef EXP_INST
};

enum SQ_CF_ALU_INST : uint32_t
{
#define ALU_INST(name, value) SQ_CF_INST_##name = value,
#include "latte_instructions_def.inl"
#undef ALU_INST
};

enum SQ_OP2_INST : uint32_t
{
#define ALU_OP2(name, value, srcs, flags) SQ_OP2_INST_##name = value,
#include "latte_instructions_def.inl"
#undef ALU_OP2
};

enum SQ_OP3_INST : uint32_t
{
#define ALU_OP3(name, value, srcs, flags) SQ_OP3_INST_##name = value,
#include "latte_instructions_def.inl"
#undef ALU_OP3
};

enum SQ_TEX_INST : uint32_t
{
#define TEX_INST(name, value) SQ_TEX_INST_##name = value,
#include "latte_instructions_def.inl"
#undef TEX_INST
};

enum SQ_VTX_INST : uint32_t
{
#define VTX_INST(name, value) SQ_VTX_INST_##name = value,
#include "latte_instructions_def.inl"
#undef VTX_INST
};

enum SQ_CF_INST_TYPE : uint32_t
{
   SQ_CF_INST_TYPE_NORMAL        = 0,
   SQ_CF_INST_TYPE_EXPORT        = 1,
   SQ_CF_INST_TYPE_ALU           = 2,
   SQ_CF_INST_TYPE_ALU_EXTENDED  = 3,
};

enum SQ_ALU_FLAGS : uint32_t
{
   SQ_ALU_FLAG_NONE              = 0,
   SQ_ALU_FLAG_VECTOR            = (1 << 0),
   SQ_ALU_FLAG_TRANSCENDENTAL    = (1 << 1),
   SQ_ALU_FLAG_REDUCTION         = (1 << 2),
   SQ_ALU_FLAG_PRED_SET          = (1 << 3),
   SQ_ALU_FLAG_INT_IN            = (1 << 4),
   SQ_ALU_FLAG_INT_OUT           = (1 << 5),
   SQ_ALU_FLAG_UINT_IN           = (1 << 6),
   SQ_ALU_FLAG_UINT_OUT          = (1 << 7),
};

// Control flow instruction word 0
struct SQ_CF_WORD0
{
   uint32_t ADDR;
};

// Control flow instruction word 1
struct SQ_CF_WORD1
{
   uint32_t POP_COUNT : 3;
   uint32_t CF_CONST : 5;
   SQ_CF_COND COND : 2;
   uint32_t COUNT : 3;
   uint32_t CALL_COUNT : 6;
   uint32_t COUNT_3 : 1;
   uint32_t : 1;
   uint32_t END_OF_PROGRAM : 1;
   uint32_t VALID_PIXEL_MODE : 1;
   SQ_CF_INST CF_INST : 7;
   uint32_t WHOLE_QUAD_MODE : 1;
   uint32_t BARRIER : 1;
};

// Control flow ALU clause instruction word 0
struct SQ_CF_ALU_WORD0
{
   uint32_t ADDR : 22;
   uint32_t KCACHE_BANK0 : 4;
   uint32_t KCACHE_BANK1 : 4;
   SQ_CF_KCACHE_MODE KCACHE_MODE0 : 2;
};

// Control flow ALU clause instruction word 1
struct SQ_CF_ALU_WORD1
{
   SQ_CF_KCACHE_MODE KCACHE_MODE1 : 2;
   uint32_t KCACHE_ADDR0 : 8;
   uint32_t KCACHE_ADDR1 : 8;
   uint32_t COUNT : 7;
   uint32_t ALT_CONST : 1;
   SQ_CF_ALU_INST CF_INST : 4;
   uint32_t WHOLE_QUAD_MODE : 1;
   uint32_t BARRIER : 1;
};

// ALU instruction word 0
struct SQ_ALU_WORD0
{
   SQ_ALU_SRC SRC0_SEL : 9;
   SQ_REL SRC0_REL : 1;
   SQ_CHAN SRC0_CHAN : 2;
   uint32_t SRC0_NEG : 1;
   SQ_ALU_SRC SRC1_SEL : 9;
   SQ_REL SRC1_REL : 1;
   SQ_CHAN SRC1_CHAN : 2;
   uint32_t SRC1_NEG : 1;
   SQ_INDEX_MODE INDEX_MODE : 3;
   SQ_PRED_SEL PRED_SEL : 2;
   uint32_t LAST : 1;
};

// ALU instruction word 1
struct SQ_ALU_WORD1
{
   uint32_t : 15;
   SQ_ALU_ENCODING ENCODING : 3;
   SQ_ALU_VEC_BANK_SWIZZLE BANK_SWIZZLE : 3;
   uint32_t DST_GPR : 7;
   SQ_REL DST_REL : 1;
   SQ_CHAN DST_CHAN : 2;
   uint32_t CLAMP : 1;
};

// ALU instruction word 1. This subencoding is used for instructions taking 0-2 operands.
struct SQ_ALU_WORD1_OP2
{
   uint32_t SRC0_ABS : 1;
   uint32_t SRC1_ABS : 1;
   uint32_t UPDATE_EXECUTE_MASK : 1;
   uint32_t UPDATE_PRED : 1;
   uint32_t WRITE_MASK : 1;
   SQ_ALU_OMOD OMOD : 2;
   SQ_OP2_INST ALU_INST : 11;
   uint32_t : 14;
};

// ALU instruction word 1. This subencoding is used for instructions taking 3 operands.
struct SQ_ALU_WORD1_OP3
{
   SQ_ALU_SRC SRC2_SEL : 9;
   SQ_REL SRC2_REL : 1;
   SQ_CHAN SRC2_CHAN : 2;
   uint32_t SRC2_NEG : 1;
   SQ_OP3_INST ALU_INST : 5;
   uint32_t : 14;
};

// Word 0 of the control flow instruction for alloc/export.
struct SQ_CF_ALLOC_EXPORT_WORD0
{
   uint32_t ARRAY_BASE : 13;
   SQ_EXPORT_TYPE TYPE : 2;
   uint32_t RW_GPR : 7;
   SQ_REL RW_REL : 1;
   uint32_t INDEX_GPR : 7;
   uint32_t ELEM_SIZE : 2;
};

// Word 1 of the control flow instruction
struct SQ_CF_ALLOC_EXPORT_WORD1
{
   uint32_t : 17;
   uint32_t BURST_COUNT : 4;
   uint32_t END_OF_PROGRAM : 1;
   uint32_t VALID_PIXEL_MODE : 1;
   SQ_CF_EXP_INST CF_INST : 7;
   uint32_t WHOLE_QUAD_MODE : 1;
   uint32_t BARRIER : 1;
};

// Word 1 of the control flow instruction. This subencoding is used by alloc/exports
// for all input / outputs to scratch / ring / stream / reduction buffers.
struct SQ_CF_ALLOC_EXPORT_WORD1_BUF
{
   uint32_t ARRAY_SIZE : 12;
   uint32_t COMP_MASK : 4;
};

// Word 1 of the control flow instruction. This subencoding is used by
// alloc/exports for PIXEL, POS, and PARAM.
struct SQ_CF_ALLOC_EXPORT_WORD1_SWIZ
{
   SQ_SEL SRC_SEL_X : 3;
   SQ_SEL SRC_SEL_Y : 3;
   SQ_SEL SRC_SEL_Z : 3;
   SQ_SEL SRC_SEL_W : 3;
};

// Texture fetch clause instruction word 0
struct SQ_TEX_WORD0
{
   SQ_TEX_INST TEX_INST : 5;
   uint32_t BC_FRAC_MODE : 1;
   uint32_t : 1;
   uint32_t FETCH_WHOLE_QUAD : 1;
   uint32_t RESOURCE_ID : 8;
   uint32_t SRC_GPR : 7;
   SQ_REL SRC_REL : 1;
   uint32_t ALT_CONST : 1;
   uint32_t : 7;
};

// Texture fetch clause instruction word 1
struct SQ_TEX_WORD1
{
   uint32_t DST_GPR : 7;
   SQ_REL DST_REL : 1;
   uint32_t : 1;
   SQ_SEL DST_SEL_X : 3;
   SQ_SEL DST_SEL_Y : 3;
   SQ_SEL DST_SEL_Z : 3;
   SQ_SEL DST_SEL_W : 3;
   uint32_t LOD_BIAS : 7;
   SQ_TEX_COORD_TYPE COORD_TYPE_X : 1;
   SQ_TEX_COORD_TYPE COORD_TYPE_Y : 1;
   SQ_TEX_COORD_TYPE COORD_TYPE_Z : 1;
   SQ_TEX_COORD_TYPE COORD_TYPE_W : 1;
};

// Texture fetch clause instruction word 2
struct SQ_TEX_WORD2
{
   uint32_t OFFSET_X : 5;
   uint32_t OFFSET_Y : 5;
   uint32_t OFFSET_Z : 5;
   uint32_t SAMPLER_ID : 5;
   SQ_SEL SRC_SEL_X : 3;
   SQ_SEL SRC_SEL_Y : 3;
   SQ_SEL SRC_SEL_Z : 3;
   SQ_SEL SRC_SEL_W : 3;
};

// Vertex fetch clause instruction word 0.
union SQ_VTX_WORD0
{
   uint32_t value;

   struct
   {
      SQ_VTX_INST VTX_INST : 5;
      SQ_VTX_FETCH_TYPE FETCH_TYPE : 2;
      uint32_t FETCH_WHOLE_QUAD : 1;
      uint32_t BUFFER_ID : 8;
      uint32_t SRC_GPR : 7;
      SQ_REL SRC_REL : 1;
      SQ_SEL SRC_SEL_X : 2;
      uint32_t MEGA_FETCH_COUNT : 6;
   };
};

// Vertex fetch clause instruction word 1
struct SQ_VTX_WORD1_SEM
{
   uint32_t SEMANTIC_ID : 8;
   uint32_t : 24;
};

struct SQ_VTX_WORD1_GPR
{
   uint32_t DST_GPR : 7;
   SQ_REL DST_REL : 1;
   uint32_t : 24;
};

union SQ_VTX_WORD1
{
   uint32_t value;

   struct
   {
      uint32_t : 9;
      SQ_SEL DST_SEL_X : 3;
      SQ_SEL DST_SEL_Y : 3;
      SQ_SEL DST_SEL_Z : 3;
      SQ_SEL DST_SEL_W : 3;
      uint32_t USE_CONST_FIELDS : 1;
      SQ_DATA_FORMAT DATA_FORMAT : 6;
      SQ_NUM_FORMAT NUM_FORMAT_ALL : 2;
      SQ_FORMAT_COMP FORMAT_COMP_ALL : 1;
      SQ_SRF_MODE SRF_MODE_ALL : 1;
   };

   SQ_VTX_WORD1_SEM SEM;
   SQ_VTX_WORD1_GPR GPR;
};

// Vertex fetch clause instruction word 2
union SQ_VTX_WORD2
{
   uint32_t value;

   struct
   {
      uint32_t OFFSET : 16;
      SQ_ENDIAN ENDIAN_SWAP : 2;
      uint32_t CONST_BUF_NO_STRIDE : 1;
      uint32_t MEGA_FETCH : 1;
      uint32_t ALT_CONST : 1;
      uint32_t : 11;
   };
};

union ControlFlowInst
{
   struct
   {
      SQ_CF_WORD0 word0;
      SQ_CF_WORD1 word1;
   };

   struct
   {
      SQ_CF_ALU_WORD0 word0;
      SQ_CF_ALU_WORD1 word1;
   } alu;

   struct
   {
      SQ_CF_ALLOC_EXPORT_WORD0 word0;

      union
      {
         SQ_CF_ALLOC_EXPORT_WORD1 word1;
         SQ_CF_ALLOC_EXPORT_WORD1_BUF buf;
         SQ_CF_ALLOC_EXPORT_WORD1_SWIZ swiz;
      };
   } exp;

   struct
   {
      uint32_t : 32;
      uint32_t : 28;
      SQ_CF_INST_TYPE CF_INST_TYPE : 2;
      uint32_t : 2;
   };
};

struct AluInst
{
   SQ_ALU_WORD0 word0;

   union
   {
      SQ_ALU_WORD1 word1;
      SQ_ALU_WORD1_OP2 op2;
      SQ_ALU_WORD1_OP3 op3;
   };
};

struct VertexFetchInst
{
   SQ_VTX_WORD0 word0;

   union
   {
      SQ_VTX_WORD1 word1;
      SQ_VTX_WORD1_GPR gpr;
      SQ_VTX_WORD1_SEM sem;
   };

   SQ_VTX_WORD2 word2;
   uint32_t padding;
};

struct TextureFetchInst
{
   SQ_TEX_WORD0 word0;
   SQ_TEX_WORD1 word1;
   SQ_TEX_WORD2 word2;
   uint32_t padding;
};

const char *getInstructionName(SQ_CF_INST id);
const char *getInstructionName(SQ_CF_EXP_INST id);
const char *getInstructionName(SQ_CF_ALU_INST id);
const char *getInstructionName(SQ_OP2_INST id);
const char *getInstructionName(SQ_OP3_INST id);
const char *getInstructionName(SQ_TEX_INST id);
const char *getInstructionName(SQ_VTX_INST id);

uint32_t getInstructionNumSrcs(SQ_OP2_INST id);
uint32_t getInstructionNumSrcs(SQ_OP3_INST id);

SQ_ALU_FLAGS getInstructionFlags(SQ_OP2_INST id);
SQ_ALU_FLAGS getInstructionFlags(SQ_OP3_INST id);

} // namespace latte
