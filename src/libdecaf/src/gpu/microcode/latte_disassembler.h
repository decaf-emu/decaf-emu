#pragma once
#include "latte_instructions.h"
#include <spdlog/details/format.h>
#include <gsl.h>

namespace latte
{

std::string
disassemble(const gsl::span<const uint8_t> &binary, bool isSubroutine = false);

namespace disassembler
{

struct State
{
   gsl::span<const uint8_t> binary;
   fmt::MemoryWriter out;
   std::string indent;
   size_t cfPC;
   size_t groupPC;
};

void
increaseIndent(State &state);

void
decreaseIndent(State &state);

void
disassembleControlFlowALU(State &state, const ControlFlowInst &inst);

void
disassembleVtxClause(State &state, const latte::ControlFlowInst &parent);

void
disassembleTEXClause(State &state, const ControlFlowInst &inst);

void
disassembleExport(State &state, const ControlFlowInst &inst);

char
disassembleDestMask(SQ_SEL sel);

void
disassembleCondition(fmt::MemoryWriter &out, const ControlFlowInst &inst);

void
disassembleCfTEX(fmt::MemoryWriter &out, const ControlFlowInst &inst);

void
disassembleCfVTX(fmt::MemoryWriter &out, const ControlFlowInst &inst);

void
disassembleCF(fmt::MemoryWriter &out, const ControlFlowInst &inst);

void
disassembleAluInstruction(fmt::MemoryWriter &out,
                          const ControlFlowInst &parent,
                          const AluInst &inst,
                          size_t groupPC,
                          SQ_CHAN unit,
                          const gsl::span<const uint32_t> &literals,
                          int namePad = 0);

void
disassembleCfALUInstruction(fmt::MemoryWriter &out,
                            const ControlFlowInst &inst);

void
disassembleExpInstruction(fmt::MemoryWriter &out,
                          const ControlFlowInst &inst);


void
disassembleTexInstruction(fmt::MemoryWriter &out,
                          const latte::ControlFlowInst &parent,
                          const TextureFetchInst &tex,
                          int namePad = 0);

} // namespace disassembler

} // namespace latte
