#include "glsl2_translate.h"
#include <fmt/format.h>

namespace glsl2
{

void
insertIndexMode(fmt::memory_buffer &out,
                latte::SQ_INDEX_MODE index);

void
insertChannel(fmt::memory_buffer &out,
              latte::SQ_CHAN channel);

void
insertSource0(State &state,
              fmt::memory_buffer &out,
              const latte::ControlFlowInst &cf,
              const latte::AluInst &inst);

void
insertSource1(State &state,
              fmt::memory_buffer &out,
              const latte::ControlFlowInst &cf,
              const latte::AluInst &inst);
void
insertSource2(State &state,
              fmt::memory_buffer &out,
              const latte::ControlFlowInst &cf,
              const latte::AluInst &inst);

void
insertSource0Vector(State &state,
                    fmt::memory_buffer &out,
                    const latte::ControlFlowInst &cf,
                    const latte::AluInst &x,
                    const latte::AluInst &y,
                    const latte::AluInst &z,
                    const latte::AluInst &w);

void
insertSource1Vector(State &state,
                    fmt::memory_buffer &out,
                    const latte::ControlFlowInst &cf,
                    const latte::AluInst &x,
                    const latte::AluInst &y,
                    const latte::AluInst &z,
                    const latte::AluInst &w);

void
insertSource2Vector(State &state,
                    fmt::memory_buffer &out,
                    const latte::ControlFlowInst &cf,
                    const latte::AluInst &x,
                    const latte::AluInst &y,
                    const latte::AluInst &z,
                    const latte::AluInst &w);

void
insertSource(State &state,
             fmt::memory_buffer &out,
             const latte::ControlFlowInst &cf,
             const latte::AluInst &inst,
             const latte::SQ_ALU_SRC sel,
             const latte::SQ_REL rel,
             const latte::SQ_CHAN chan,
             const bool abs,
             const bool neg);

void
insertPreviousValueUpdate(fmt::memory_buffer &out,
                          latte::SQ_CHAN unit);

void
insertDestBegin(State &state,
                const latte::ControlFlowInst &cf,
                const latte::AluInst &inst,
                latte::SQ_CHAN unit);

void
insertDestEnd(State &state,
              const latte::ControlFlowInst &cf,
              const latte::AluInst &inst);

void
updatePredicate(State &state,
                const latte::ControlFlowInst &cf,
                const latte::AluInst &inst,
                const std::string &condition);

} // namespace glsl2
