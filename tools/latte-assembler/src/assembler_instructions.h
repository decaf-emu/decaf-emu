#pragma once
#include <libgpu/latte/latte_instructions.h>
#include <string_view>

latte::SQ_CF_INST getCfInstructionByName(const std::string_view name);
latte::SQ_CF_EXP_INST getCfExpInstructionByName(const std::string_view name);
latte::SQ_CF_ALU_INST getCfAluInstructionByName(const std::string_view name);
latte::SQ_OP2_INST getAluOp2InstructionByName(const std::string_view name);
latte::SQ_OP3_INST getAluOp3InstructionByName(const std::string_view name);
latte::SQ_TEX_INST getTexInstructionByName(const std::string_view name);
latte::SQ_VTX_INST getVtxInstructionByName(const std::string_view name);
