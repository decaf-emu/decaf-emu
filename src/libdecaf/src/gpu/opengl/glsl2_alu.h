#include "glsl2_translate.h"

namespace glsl2
{

void
insertIndexMode(fmt::MemoryWriter &out,
                latte::SQ_INDEX_MODE index);

void
insertChannel(fmt::MemoryWriter &out,
              latte::SQ_CHAN channel);

void
insertSource0(fmt::MemoryWriter &out,
              const gsl::span<const uint32_t> &literals,
              const latte::ControlFlowInst &cf,
              const latte::AluInst &inst);

void
insertSource1(fmt::MemoryWriter &out,
              const gsl::span<const uint32_t> &literals,
              const latte::ControlFlowInst &cf,
              const latte::AluInst &inst);
void
insertSource2(fmt::MemoryWriter &out,
              const gsl::span<const uint32_t> &literals,
              const latte::ControlFlowInst &cf,
              const latte::AluInst &inst);

void
insertSource0Vector(fmt::MemoryWriter &out,
                    const gsl::span<const uint32_t> &literals,
                    const latte::ControlFlowInst &cf,
                    const latte::AluInst &x,
                    const latte::AluInst &y,
                    const latte::AluInst &z,
                    const latte::AluInst &w);

void
insertSource1Vector(fmt::MemoryWriter &out,
                    const gsl::span<const uint32_t> &literals,
                    const latte::ControlFlowInst &cf,
                    const latte::AluInst &x,
                    const latte::AluInst &y,
                    const latte::AluInst &z,
                    const latte::AluInst &w);

void
insertSource2Vector(fmt::MemoryWriter &out,
                    const gsl::span<const uint32_t> &literals,
                    const latte::ControlFlowInst &cf,
                    const latte::AluInst &x,
                    const latte::AluInst &y,
                    const latte::AluInst &z,
                    const latte::AluInst &w);

void
insertSource(fmt::MemoryWriter &out,
             const gsl::span<const uint32_t> &literals,
             const latte::ControlFlowInst &cf,
             const latte::AluInst &inst,
             const latte::SQ_ALU_SRC sel,
             const latte::SQ_REL rel,
             const latte::SQ_CHAN chan,
             const bool abs,
             const bool neg);

void
insertPreviousValueUpdate(fmt::MemoryWriter &out,
                          latte::SQ_CHAN unit);

void
insertDestBegin(fmt::MemoryWriter &out,
                const latte::ControlFlowInst &cf,
                const latte::AluInst &inst,
                latte::SQ_CHAN unit);

void
insertDestEnd(fmt::MemoryWriter &out,
              const latte::ControlFlowInst &cf,
              const latte::AluInst &inst);

void
updatePredicate(State &state,
                const latte::ControlFlowInst &cf,
                const latte::AluInst &inst,
                const std::string &condition);

} // namespace glsl2
