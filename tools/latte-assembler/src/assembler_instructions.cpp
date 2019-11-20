#include "assembler_instructions.h"

latte::SQ_CF_INST getCfInstructionByName(const std::string &findName)
{
#define CF_INST(name, value) if (findName == #name) { return latte::SQ_CF_INST_##name; }
#include <libgpu/latte/latte_instructions_def.inl>
#undef CF_INST
   return latte::SQ_CF_INST_INVALID;
}

latte::SQ_CF_EXP_INST getCfExpInstructionByName(const std::string &findName)
{
#define EXP_INST(name, value) if (findName == #name) { return latte::SQ_CF_INST_##name; }
#include <libgpu/latte/latte_instructions_def.inl>
#undef EXP_INST
   return latte::SQ_CF_EXP_INST_INVALID;
}

latte::SQ_CF_ALU_INST getCfAluInstructionByName(const std::string &findName)
{
#define ALU_INST(name, value) if (findName == #name) { return latte::SQ_CF_INST_##name; }
#include <libgpu/latte/latte_instructions_def.inl>
#undef ALU_INST
   return latte::SQ_CF_ALU_INST_INVALID;
}

latte::SQ_OP2_INST getAluOp2InstructionByName(const std::string &findName)
{
#define ALU_OP2(name, value, srcs, flags) if (findName == #name) { return latte::SQ_OP2_INST_##name; }
#include <libgpu/latte/latte_instructions_def.inl>
#undef ALU_OP2
   return latte::SQ_OP2_INST_INVALID;
}

latte::SQ_OP3_INST getAluOp3InstructionByName(const std::string &findName)
{
#define ALU_OP3(name, value, srcs, flags) if (findName == #name) { return latte::SQ_OP3_INST_##name; }
#include <libgpu/latte/latte_instructions_def.inl>
#undef ALU_OP3
   return latte::SQ_OP3_INST_INVALID;
}

latte::SQ_TEX_INST getTexInstructionByName(const std::string &findName)
{
#define TEX_INST(name, value) if (findName == #name) { return latte::SQ_TEX_INST_##name; }
#include <libgpu/latte/latte_instructions_def.inl>
#undef TEX_INST
   return latte::SQ_TEX_INST_INVALID;
}

latte::SQ_VTX_INST getVtxInstructionByName(const std::string &findName)
{
#define VTX_INST(name, value) if (findName == #name) { return latte::SQ_VTX_INST_##name; }
#include <libgpu/latte/latte_instructions_def.inl>
#undef VTX_INST
   return latte::SQ_VTX_INST_INVALID;
}
