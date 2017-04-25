#include "latte/latte_instructions.h"

namespace latte
{

#undef ALU_REDUC
#undef ALU_VEC
#undef ALU_TRANS
#undef ALU_PRED_SET
#undef ALU_INT
#undef ALU_UINT
#undef ALU_INT_IN
#undef ALU_UINT_IN
#undef ALU_INT_OUT
#undef ALU_UINT_OUT

#define ALU_REDUC    SQ_ALU_FLAG_REDUCTION
#define ALU_VEC      SQ_ALU_FLAG_VECTOR
#define ALU_TRANS    SQ_ALU_FLAG_TRANSCENDENTAL
#define ALU_PRED_SET SQ_ALU_FLAG_PRED_SET

#define ALU_INT_IN   SQ_ALU_FLAG_INT_IN
#define ALU_INT_OUT  SQ_ALU_FLAG_INT_OUT

#define ALU_UINT_IN  SQ_ALU_FLAG_UINT_IN
#define ALU_UINT_OUT SQ_ALU_FLAG_UINT_OUT

#define ALU_INT      SQ_ALU_FLAG_INT_IN  | SQ_ALU_FLAG_INT_OUT
#define ALU_UINT     SQ_ALU_FLAG_UINT_IN | SQ_ALU_FLAG_UINT_OUT

const char *getInstructionName(SQ_CF_INST id)
{
   switch (id) {
#define CF_INST(name, value) case SQ_CF_INST_##name: return #name;
#include "latte/latte_instructions_def.inl"
#undef CF_INST
   default:
      return "UNKNOWN";
   }
}

const char *getInstructionName(SQ_CF_EXP_INST id)
{
   switch (id) {
#define EXP_INST(name, value) case SQ_CF_INST_##name: return #name;
#include "latte/latte_instructions_def.inl"
#undef EXP_INST
   default:
      return "UNKNOWN";
   }
}

const char *getInstructionName(SQ_CF_ALU_INST id)
{
   switch (id) {
#define ALU_INST(name, value) case SQ_CF_INST_##name: return #name;
#include "latte/latte_instructions_def.inl"
#undef ALU_INST
   default:
      return "UNKNOWN";
   }
}

const char *getInstructionName(SQ_OP2_INST id)
{
   switch (id) {
#define ALU_OP2(name, value, srcs, flags) case SQ_OP2_INST_##name: return #name;
#include "latte/latte_instructions_def.inl"
#undef ALU_OP2
   default:
      return "UNKNOWN";
   }
}

const char *getInstructionName(SQ_OP3_INST id)
{
   switch (id) {
#define ALU_OP3(name, value, srcs, flags) case SQ_OP3_INST_##name: return #name;
#include "latte/latte_instructions_def.inl"
#undef ALU_OP3
   default:
      return "UNKNOWN";
   }
}

const char *getInstructionName(SQ_TEX_INST id)
{
   switch (id) {
#define TEX_INST(name, value) case SQ_TEX_INST_##name: return #name;
#include "latte/latte_instructions_def.inl"
#undef TEX_INST
   default:
      return "UNKNOWN";
   }
}

const char *getInstructionName(SQ_VTX_INST id)
{
   switch (id) {
#define VTX_INST(name, value) case SQ_VTX_INST_##name: return #name;
#include "latte/latte_instructions_def.inl"
#undef VTX_INST
   default:
      return "UNKNOWN";
   }
}

uint32_t getInstructionNumSrcs(SQ_OP2_INST id)
{
   switch (id) {
#define ALU_OP2(name, value, srcs, flags) case SQ_OP2_INST_##name: return srcs;
#include "latte/latte_instructions_def.inl"
#undef ALU_OP2
   default:
      return 0;
   }
}

uint32_t getInstructionNumSrcs(SQ_OP3_INST id)
{
   switch (id) {
#define ALU_OP3(name, value, srcs, flags) case SQ_OP3_INST_##name: return srcs;
#include "latte/latte_instructions_def.inl"
#undef ALU_OP3
   default:
      return 0;
   }
}

SQ_ALU_FLAGS getInstructionFlags(SQ_OP2_INST id)
{
   switch (id) {
#define ALU_OP2(name, value, srcs, flags) case SQ_OP2_INST_##name: return static_cast<SQ_ALU_FLAGS>(flags);
#include "latte/latte_instructions_def.inl"
#undef ALU_OP2
   default:
      return static_cast<SQ_ALU_FLAGS>(0);
   }
}

SQ_ALU_FLAGS getInstructionFlags(SQ_OP3_INST id)
{
   switch (id) {
#define ALU_OP3(name, value, srcs, flags) case SQ_OP3_INST_##name: return static_cast<SQ_ALU_FLAGS>(flags);
#include "latte/latte_instructions_def.inl"
#undef ALU_OP3
   default:
      return static_cast<SQ_ALU_FLAGS>(0);
   }
}

} // namespace latte
