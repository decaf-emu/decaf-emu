#pragma once
#include <string>
#include <spdlog/spdlog.h>
#include "gpu/microcode/latte_shadir.h"

struct GenerateState
{
   fmt::MemoryWriter out;
   std::string indent;
   uint32_t cfPC = -1;
   uint32_t groupPC = -1;
   latte::Shader *shader = nullptr;
};

using TranslateFuncCF = bool(*)(GenerateState &state, latte::shadir::CfInstruction *ins);
using TranslateFuncALU = bool(*)(GenerateState &state, latte::shadir::AluInstruction *ins);
using TranslateFuncALUReduction = bool(*)(GenerateState &state, latte::shadir::AluReductionInstruction *ins);
using TranslateFuncTEX = bool(*)(GenerateState &state, latte::shadir::TextureFetchInstruction *ins);
using TranslateFuncVTX = bool(*)(GenerateState &state, latte::shadir::VertexFetchInstruction *ins);
using TranslateFuncEXP = bool(*)(GenerateState &state, latte::shadir::ExportInstruction *ins);

namespace gpu
{

namespace opengl
{

namespace glsl
{

void intialise();

bool generateBody(latte::Shader &shader, std::string &body);

void registerGenerator(latte::SQ_CF_INST ins, TranslateFuncCF func);
void registerGenerator(latte::SQ_OP2_INST ins, TranslateFuncALU func);
void registerGenerator(latte::SQ_OP3_INST ins, TranslateFuncALU func);
void registerGenerator(latte::SQ_OP2_INST ins, TranslateFuncALUReduction func);
void registerGenerator(latte::SQ_TEX_INST ins, TranslateFuncTEX func);
void registerGenerator(latte::SQ_VTX_INST ins, TranslateFuncVTX func);
void registerGenerator(latte::SQ_CF_EXP_INST ins, TranslateFuncEXP func);

void registerCf();
void registerTex();
void registerVtx();
void registerExp();
void registerAluOP2();
void registerAluOP3();
void registerAluReduction();

void beginLine(GenerateState &state);
void endLine(GenerateState &state);
void increaseIndent(GenerateState &state);
void decreaseIndent(GenerateState &state);

using latte::shadir::AluSource;
using latte::shadir::AluInstruction;
using latte::shadir::TextureFetchRegister;

void translateAluDestStart(GenerateState &state,
                           AluInstruction *ins);
void translateAluDestEnd(GenerateState &state,
                         AluInstruction *ins);
void translateAluSource(GenerateState &state,
                        const AluInstruction *ins,
                        const AluSource &src);
void translateAluSourceVector(GenerateState &state,
                              const AluInstruction *ins,
                              const AluSource &srcX,
                              const AluSource &srcY,
                              const AluSource &srcZ,
                              const AluSource &srcW);

void translateChannel(GenerateState &state, latte::SQ_CHAN channel);
unsigned translateSelectMask(GenerateState &state, const std::array<latte::SQ_SEL, 4> &sel, size_t maxSel);

} // namespace glsl

} // namespace opengl

} // namespace gpu
