#pragma once
#include <string>
#include <spdlog/spdlog.h>
#include "gpu/latte.h"
#include "gpu/latte_shadir.h"

enum class ShaderType
{
   Geometry,
   Vertex,
   Pixel,
};

struct GenerateState
{
   fmt::MemoryWriter out;
   std::string indent;
   int32_t cfPC = -1;
   int32_t groupPC = -1;
   latte::Shader *shader;
   ShaderType shaderType;
};

using TranslateFuncCF = bool(*)(GenerateState &state, latte::shadir::CfInstruction *ins);
using TranslateFuncALU = bool(*)(GenerateState &state, latte::shadir::AluInstruction *ins);
using TranslateFuncALUReduction = bool(*)(GenerateState &state, latte::shadir::AluReductionInstruction *ins);
using TranslateFuncTEX = bool(*)(GenerateState &state, latte::shadir::TexInstruction *ins);
using TranslateFuncEXP = bool(*)(GenerateState &state, latte::shadir::ExportInstruction *ins);

namespace hlsl
{

void intialise();

bool generateBody(ShaderType shaderType, latte::Shader &shader, std::string &body);

void registerGenerator(latte::cf::inst ins, TranslateFuncCF func);
void registerGenerator(latte::alu::op2 ins, TranslateFuncALU func);
void registerGenerator(latte::alu::op3 ins, TranslateFuncALU func);
void registerGenerator(latte::alu::op2 ins, TranslateFuncALUReduction func);
void registerGenerator(latte::tex::inst ins, TranslateFuncTEX func);
void registerGenerator(latte::exp::inst ins, TranslateFuncEXP func);

void registerCf();
void registerTex();
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
using latte::shadir::SelRegister;
using latte::alu::Channel::Channel;

void translateAluDestStart(GenerateState &state, AluInstruction *ins);
void translateAluDestEnd(GenerateState &state, AluInstruction *ins);
void translateAluSource(GenerateState &state, AluSource &src);
void translateAluSourceVector(GenerateState &state, AluSource &srcX, AluSource &srcY, AluSource &srcZ, AluSource &srcW);

void translateChannel(GenerateState &state, Channel channel);
unsigned translateSelRegister(GenerateState &state, SelRegister &reg);

}
