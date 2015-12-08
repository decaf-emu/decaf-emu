#pragma once
#include <spdlog/details/format.h>
#include "latte_shadir.h"

namespace latte
{

namespace disassembler
{

struct State
{
   fmt::MemoryWriter out;
   std::string indent;
   Shader *shader;
};

void
increaseIndent(State &state);

void
decreaseIndent(State &state);

bool
disassembleCondition(State &state, shadir::CfInstruction *inst);

bool
disassembleControlFlowALU(State &state, shadir::CfAluInstruction *inst);

bool
disassembleVTX(State &state, shadir::CfInstruction *inst);

bool
disassembleTEX(State &state, shadir::CfInstruction *inst);

bool
disassembleExport(State &state, shadir::ExportInstruction *inst);

} // namespace disassembler

} // namespace latte
