#pragma once
#include "common/bitfield.h"
#include "common/types.h"
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
struct SQ_CF_WORD1 : Bitfield<SQ_CF_WORD1, uint32_t>
{
   BITFIELD_ENTRY(0, 3, uint32_t, POP_COUNT);
   BITFIELD_ENTRY(3, 5, uint32_t, CF_CONST);
   BITFIELD_ENTRY(8, 2, SQ_CF_COND, COND);
   BITFIELD_ENTRY(10, 3, uint32_t, COUNT);
   BITFIELD_ENTRY(13, 6, uint32_t, CALL_COUNT);
   BITFIELD_ENTRY(19, 1, uint32_t, COUNT_3);
   BITFIELD_ENTRY(21, 1, bool, END_OF_PROGRAM);
   BITFIELD_ENTRY(22, 1, bool, VALID_PIXEL_MODE);
   BITFIELD_ENTRY(23, 7, SQ_CF_INST, CF_INST);
   BITFIELD_ENTRY(28, 2, SQ_CF_INST_TYPE, CF_INST_TYPE);
   BITFIELD_ENTRY(30, 1, bool, WHOLE_QUAD_MODE);
   BITFIELD_ENTRY(31, 1, bool, BARRIER);
};

// Control flow ALU clause instruction word 0
struct SQ_CF_ALU_WORD0 : Bitfield<SQ_CF_ALU_WORD0, uint32_t>
{
   BITFIELD_ENTRY(0, 22, uint32_t, ADDR);
   BITFIELD_ENTRY(22, 4, uint32_t, KCACHE_BANK0);
   BITFIELD_ENTRY(26, 4, uint32_t, KCACHE_BANK1);
   BITFIELD_ENTRY(30, 2, SQ_CF_KCACHE_MODE, KCACHE_MODE0);
};

// Control flow ALU clause instruction word 1
struct SQ_CF_ALU_WORD1 : Bitfield<SQ_CF_ALU_WORD1, uint32_t>
{
   BITFIELD_ENTRY(0, 2, SQ_CF_KCACHE_MODE, KCACHE_MODE1);
   BITFIELD_ENTRY(2, 8, uint32_t, KCACHE_ADDR0);
   BITFIELD_ENTRY(10, 8, uint32_t, KCACHE_ADDR1);
   BITFIELD_ENTRY(18, 7, uint32_t, COUNT);
   BITFIELD_ENTRY(25, 1, bool, ALT_CONST);
   BITFIELD_ENTRY(26, 4, SQ_CF_ALU_INST, CF_INST);
   BITFIELD_ENTRY(30, 1, bool, WHOLE_QUAD_MODE);
   BITFIELD_ENTRY(31, 1, bool, BARRIER);
};

// ALU instruction word 0
struct SQ_ALU_WORD0 : Bitfield<SQ_ALU_WORD0, uint32_t>
{
   BITFIELD_ENTRY(0, 9, SQ_ALU_SRC, SRC0_SEL);
   BITFIELD_ENTRY(9, 1, SQ_REL, SRC0_REL);
   BITFIELD_ENTRY(10, 2, SQ_CHAN, SRC0_CHAN);
   BITFIELD_ENTRY(12, 1, bool, SRC0_NEG);
   BITFIELD_ENTRY(13, 9, SQ_ALU_SRC, SRC1_SEL);
   BITFIELD_ENTRY(22, 1, SQ_REL, SRC1_REL);
   BITFIELD_ENTRY(23, 2, SQ_CHAN, SRC1_CHAN);
   BITFIELD_ENTRY(25, 1, bool, SRC1_NEG);
   BITFIELD_ENTRY(26, 3, SQ_INDEX_MODE, INDEX_MODE);
   BITFIELD_ENTRY(29, 2, SQ_PRED_SEL, PRED_SEL);
   BITFIELD_ENTRY(31, 1, bool, LAST);
};

// ALU instruction word 1
struct SQ_ALU_WORD1 : Bitfield<SQ_ALU_WORD1, uint32_t>
{
   BITFIELD_ENTRY(15, 3, SQ_ALU_ENCODING, ENCODING);
   BITFIELD_ENTRY(18, 3, SQ_ALU_VEC_BANK_SWIZZLE, BANK_SWIZZLE);
   BITFIELD_ENTRY(21, 7, uint32_t, DST_GPR);
   BITFIELD_ENTRY(28, 1, SQ_REL, DST_REL);
   BITFIELD_ENTRY(29, 2, SQ_CHAN, DST_CHAN);
   BITFIELD_ENTRY(31, 1, bool, CLAMP);
};

// ALU instruction word 1. This subencoding is used for instructions taking 0-2 operands.
struct SQ_ALU_WORD1_OP2 : Bitfield<SQ_ALU_WORD1_OP2, uint32_t>
{
   BITFIELD_ENTRY(0, 1, bool, SRC0_ABS);
   BITFIELD_ENTRY(1, 1, bool, SRC1_ABS);
   BITFIELD_ENTRY(2, 1, bool, UPDATE_EXECUTE_MASK);
   BITFIELD_ENTRY(3, 1, bool, UPDATE_PRED);
   BITFIELD_ENTRY(4, 1, bool, WRITE_MASK);
   BITFIELD_ENTRY(5, 2, SQ_ALU_OMOD, OMOD);
   BITFIELD_ENTRY(5, 2, SQ_ALU_EXECUTE_MASK_OP, EXECUTE_MASK_OP);
   BITFIELD_ENTRY(7, 11, SQ_OP2_INST, ALU_INST);
};

// ALU instruction word 1. This subencoding is used for instructions taking 3 operands.
struct SQ_ALU_WORD1_OP3 : Bitfield<SQ_ALU_WORD1_OP3, uint32_t>
{
   BITFIELD_ENTRY(0, 9, SQ_ALU_SRC, SRC2_SEL);
   BITFIELD_ENTRY(9, 1, SQ_REL, SRC2_REL);
   BITFIELD_ENTRY(10, 2, SQ_CHAN, SRC2_CHAN);
   BITFIELD_ENTRY(12, 1, bool, SRC2_NEG);
   BITFIELD_ENTRY(13, 5, SQ_OP3_INST, ALU_INST);
};

// Word 0 of the control flow instruction for alloc/export.
struct SQ_CF_ALLOC_EXPORT_WORD0 : Bitfield<SQ_CF_ALLOC_EXPORT_WORD0, uint32_t>
{
   BITFIELD_ENTRY(0, 13, uint32_t, ARRAY_BASE);
   BITFIELD_ENTRY(13, 2, SQ_EXPORT_TYPE, TYPE);
   BITFIELD_ENTRY(15, 7, uint32_t, RW_GPR);
   BITFIELD_ENTRY(22, 1, SQ_REL, RW_REL);
   BITFIELD_ENTRY(23, 7, uint32_t, INDEX_GPR);
   BITFIELD_ENTRY(30, 2, uint32_t, ELEM_SIZE);
};

// Word 1 of the control flow instruction
struct SQ_CF_ALLOC_EXPORT_WORD1 : Bitfield<SQ_CF_ALLOC_EXPORT_WORD1, uint32_t>
{
   BITFIELD_ENTRY(17, 4, uint32_t, BURST_COUNT);
   BITFIELD_ENTRY(21, 1, bool, END_OF_PROGRAM);
   BITFIELD_ENTRY(22, 1, bool, VALID_PIXEL_MODE);
   BITFIELD_ENTRY(23, 7, SQ_CF_EXP_INST, CF_INST);
   BITFIELD_ENTRY(30, 1, bool, WHOLE_QUAD_MODE);
   BITFIELD_ENTRY(31, 1, bool, BARRIER);
};

// Word 1 of the control flow instruction. This subencoding is used by alloc/exports
// for all input / outputs to scratch / ring / stream / reduction buffers.
struct SQ_CF_ALLOC_EXPORT_WORD1_BUF : Bitfield<SQ_CF_ALLOC_EXPORT_WORD1_BUF, uint32_t>
{
   BITFIELD_ENTRY(0, 12, uint32_t, ARRAY_SIZE);
   BITFIELD_ENTRY(12, 4, uint32_t, COMP_MASK);
};

// Word 1 of the control flow instruction. This subencoding is used by
// alloc/exports for PIXEL, POS, and PARAM.
struct SQ_CF_ALLOC_EXPORT_WORD1_SWIZ : Bitfield<SQ_CF_ALLOC_EXPORT_WORD1_SWIZ, uint32_t>
{
   BITFIELD_ENTRY(0, 3, SQ_SEL, SRC_SEL_X);
   BITFIELD_ENTRY(3, 3, SQ_SEL, SRC_SEL_Y);
   BITFIELD_ENTRY(6, 3, SQ_SEL, SRC_SEL_Z);
   BITFIELD_ENTRY(9, 3, SQ_SEL, SRC_SEL_W);
};

// Texture fetch clause instruction word 0
struct SQ_TEX_WORD0 : Bitfield<SQ_TEX_WORD0, uint32_t>
{
   BITFIELD_ENTRY(0, 5, SQ_TEX_INST, TEX_INST);
   BITFIELD_ENTRY(5, 1, bool, BC_FRAC_MODE);
   BITFIELD_ENTRY(7, 1, bool, FETCH_WHOLE_QUAD);
   BITFIELD_ENTRY(8, 8, uint32_t, RESOURCE_ID);
   BITFIELD_ENTRY(16, 7, uint32_t, SRC_GPR);
   BITFIELD_ENTRY(23, 1, SQ_REL, SRC_REL);
   BITFIELD_ENTRY(24, 1, bool, ALT_CONST);
};

// Texture fetch clause instruction word 1
struct SQ_TEX_WORD1 : Bitfield<SQ_TEX_WORD1, uint32_t>
{
   BITFIELD_ENTRY(0, 7, uint32_t, DST_GPR);
   BITFIELD_ENTRY(7, 1, SQ_REL, DST_REL);
   BITFIELD_ENTRY(9, 3, SQ_SEL, DST_SEL_X);
   BITFIELD_ENTRY(12, 3, SQ_SEL, DST_SEL_Y);
   BITFIELD_ENTRY(15, 3, SQ_SEL, DST_SEL_Z);
   BITFIELD_ENTRY(18, 3, SQ_SEL, DST_SEL_W);
   BITFIELD_ENTRY(21, 7, uint32_t, LOD_BIAS);
   BITFIELD_ENTRY(28, 1, SQ_TEX_COORD_TYPE, COORD_TYPE_X);
   BITFIELD_ENTRY(29, 1, SQ_TEX_COORD_TYPE, COORD_TYPE_Y);
   BITFIELD_ENTRY(30, 1, SQ_TEX_COORD_TYPE, COORD_TYPE_Z);
   BITFIELD_ENTRY(31, 1, SQ_TEX_COORD_TYPE, COORD_TYPE_W);
};

// Texture fetch clause instruction word 2
struct SQ_TEX_WORD2 : Bitfield<SQ_TEX_WORD2, uint32_t>
{
   BITFIELD_ENTRY(0, 5, uint32_t, OFFSET_X);
   BITFIELD_ENTRY(5, 5, uint32_t, OFFSET_Y);
   BITFIELD_ENTRY(10, 5, uint32_t, OFFSET_Z);
   BITFIELD_ENTRY(15, 5, uint32_t, SAMPLER_ID);
   BITFIELD_ENTRY(20, 3, SQ_SEL, SRC_SEL_X);
   BITFIELD_ENTRY(23, 3, SQ_SEL, SRC_SEL_Y);
   BITFIELD_ENTRY(26, 3, SQ_SEL, SRC_SEL_Z);
   BITFIELD_ENTRY(29, 3, SQ_SEL, SRC_SEL_W);
};

// Vertex fetch clause instruction word 0.
struct SQ_VTX_WORD0 : Bitfield<SQ_VTX_WORD0, uint32_t>
{
   BITFIELD_ENTRY(0, 5, SQ_VTX_INST, VTX_INST);
   BITFIELD_ENTRY(5, 2, SQ_VTX_FETCH_TYPE, FETCH_TYPE);
   BITFIELD_ENTRY(7, 1, uint32_t, FETCH_WHOLE_QUAD);
   BITFIELD_ENTRY(8, 8, uint32_t, BUFFER_ID);
   BITFIELD_ENTRY(16, 7, uint32_t, SRC_GPR);
   BITFIELD_ENTRY(23, 1, SQ_REL, SRC_REL);
   BITFIELD_ENTRY(24, 2, SQ_SEL, SRC_SEL_X);
   BITFIELD_ENTRY(26, 6, uint32_t, MEGA_FETCH_COUNT);
};

// Vertex fetch clause instruction word 1
struct SQ_VTX_WORD1_SEM : Bitfield<SQ_VTX_WORD1_SEM, uint32_t>
{
   BITFIELD_ENTRY(0, 8, uint32_t, SEMANTIC_ID);
};

struct SQ_VTX_WORD1_GPR : Bitfield<SQ_VTX_WORD1_GPR, uint32_t>
{
   BITFIELD_ENTRY(0, 7, uint32_t, DST_GPR);
   BITFIELD_ENTRY(7, 1, SQ_REL, DST_REL);
};

struct SQ_VTX_WORD1 : Bitfield<SQ_VTX_WORD1, uint32_t>
{
   BITFIELD_ENTRY(9, 3, SQ_SEL, DST_SEL_X);
   BITFIELD_ENTRY(12, 3, SQ_SEL, DST_SEL_Y);
   BITFIELD_ENTRY(15, 3, SQ_SEL, DST_SEL_Z);
   BITFIELD_ENTRY(18, 3, SQ_SEL, DST_SEL_W);
   BITFIELD_ENTRY(21, 1, bool, USE_CONST_FIELDS);
   BITFIELD_ENTRY(22, 6, SQ_DATA_FORMAT, DATA_FORMAT);
   BITFIELD_ENTRY(28, 2, SQ_NUM_FORMAT, NUM_FORMAT_ALL);
   BITFIELD_ENTRY(30, 1, SQ_FORMAT_COMP, FORMAT_COMP_ALL);
   BITFIELD_ENTRY(31, 1, SQ_SRF_MODE, SRF_MODE_ALL);
};

// Vertex fetch clause instruction word 2
struct SQ_VTX_WORD2 : Bitfield<SQ_VTX_WORD2, uint32_t>
{
   BITFIELD_ENTRY(0, 16, uint32_t, OFFSET);
   BITFIELD_ENTRY(16, 2, SQ_ENDIAN, ENDIAN_SWAP);
   BITFIELD_ENTRY(18, 1, bool, CONST_BUF_NO_STRIDE);
   BITFIELD_ENTRY(19, 1, bool, MEGA_FETCH);
   BITFIELD_ENTRY(20, 1, bool, ALT_CONST);
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
