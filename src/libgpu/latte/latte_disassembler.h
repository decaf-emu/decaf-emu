#pragma once
#include "latte_instructions.h"

#include <fmt/format.h>
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
   fmt::memory_buffer out;
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
disassembleCondition(fmt::memory_buffer &out, const ControlFlowInst &inst);

void
disassembleCfTEX(fmt::memory_buffer &out, const ControlFlowInst &inst);

void
disassembleCfVTX(fmt::memory_buffer &out, const ControlFlowInst &inst);

void
disassembleCF(fmt::memory_buffer &out, const ControlFlowInst &inst);

void
disassembleAluInstruction(fmt::memory_buffer &out,
                          const ControlFlowInst &parent,
                          const AluInst &inst,
                          size_t groupPC,
                          SQ_CHAN unit,
                          const gsl::span<const uint32_t> &literals,
                          int namePad = 0);

void
disassembleCfALUInstruction(fmt::memory_buffer &out,
                            const ControlFlowInst &inst);

void
disassembleExpInstruction(fmt::memory_buffer &out,
                          const ControlFlowInst &inst);

void
disassembleVtxInstruction(fmt::memory_buffer &out,
                          const latte::ControlFlowInst &parent,
                          const VertexFetchInst &tex,
                          int namePad = 0);

void
disassembleTexInstruction(fmt::memory_buffer &out,
                          const latte::ControlFlowInst &parent,
                          const TextureFetchInst &tex,
                          int namePad = 0);

} // namespace disassembler

} // namespace latte
