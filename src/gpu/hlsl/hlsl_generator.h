#pragma once
#include <string>
#include <spdlog/spdlog.h>
#include "gpu/latte.h"
#include "gpu/latte_shadir.h"

struct GenerateState
{
   fmt::MemoryWriter out;
   std::string indent;
};

using TranslateFuncCF = bool(*)(GenerateState &state, latte::shadir::CfInstruction *ins);
using TranslateFuncALU = bool(*)(GenerateState &state, latte::shadir::AluInstruction *ins);
using TranslateFuncALUReduction = bool(*)(GenerateState &state, latte::shadir::AluReductionInstruction *ins);
using TranslateFuncTEX = bool(*)(GenerateState &state, latte::shadir::TexInstruction *ins);

namespace hlsl
{

void intialise();

void registerGenerator(latte::cf::inst ins, TranslateFuncCF func);
void registerGenerator(latte::alu::op2 ins, TranslateFuncALU func);
void registerGenerator(latte::alu::op3 ins, TranslateFuncALU func);
void registerGenerator(latte::alu::op2 ins, TranslateFuncALUReduction func);
void registerGenerator(latte::tex::inst ins, TranslateFuncTEX func);

void registerCf();
void registerTex();
void registerAluOP2();
void registerAluOP3();
void registerAluReduction();

void beginLine(GenerateState &state);
void endLine(GenerateState &state);
void increaseIndent(GenerateState &state);
void decreaseIndent(GenerateState &state);

void translateChannel(GenerateState &state, latte::alu::Channel::Channel channel);
void translateAluDestStart(GenerateState &state, latte::shadir::AluInstruction *ins);
void translateAluDestEnd(GenerateState &state, latte::shadir::AluInstruction *ins);
void translateAluSource(GenerateState &state, latte::shadir::AluSource &src);
void translateAluSourceVector(GenerateState &state, latte::shadir::AluSource &srcX, latte::shadir::AluSource &srcY, latte::shadir::AluSource &srcZ, latte::shadir::AluSource &srcW);

}
