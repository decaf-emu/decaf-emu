#pragma once
#include <gsl.h>
#include <string>
#include <spdlog/details/format.h>
#include <vector>
#include "gpu/microcode/latte_instructions.h"

namespace glsl2
{

enum class SamplerType
{
   Invalid,

   Sampler1D,
   Sampler2D,
   Sampler3D,
   SamplerCube,
   Sampler2DRect,
   Sampler1DArray,
   Sampler2DArray,
   SamplerCubeArray,
   SamplerBuffer,
   Sampler2DMS,
   Sampler2DMSArray,

   Sampler1DShadow,
   Sampler2DShadow,
   SamplerCubeShadow,
   Sampler2DRectShadow,
   Sampler1DArrayShadow,
   Sampler2DArrayShadow,
   SamplerCubeArrayShadow,
};

struct Export
{
   latte::SQ_EXPORT_TYPE type;
   unsigned id;
};

struct Shader
{
   enum Type
   {
      Invalid,
      PixelShader,
      VertexShader,
      GeometryShader,
   };

   // Input
   Type type = Invalid;
   std::array<SamplerType, 16> samplers;
   bool uniformRegistersEnabled = false;
   bool uniformBlocksEnabled = false;

   // Output (maybe)
   std::string fileHeader;
   std::string codeHeader;
   std::string codeBody;
   std::vector<Export> exports;
   std::array<bool, 16> samplerUsed;
};

struct State
{
   Shader *shader = nullptr;
   gsl::span<const uint8_t> binary;
   uint32_t cfPC = 0;
   uint32_t groupPC = 0;
   fmt::MemoryWriter out;
   fmt::MemoryWriter outFileHeader;
   fmt::MemoryWriter outCodeHeader;
   std::string indent;
   latte::SQ_CHAN unit;
   gsl::span<const uint32_t> literals;
};

bool
translate(Shader &shader, const gsl::span<const uint8_t> &binary);

using TranslateFuncCF = void(*)(State &state, const latte::ControlFlowInst &cf);
using TranslateFuncEXP = void(*)(State &state, const latte::ControlFlowInst &cf);
using TranslateFuncALU = void(*)(State &state, const latte::ControlFlowInst &cf, const latte::AluInst &inst);
using TranslateFuncALUReduction = void(*)(State &state, const latte::ControlFlowInst &cf, const std::array<latte::AluInst, 4> &group);
using TranslateFuncTEX = void(*)(State &state, const latte::ControlFlowInst &cf, const latte::TextureFetchInst &inst);

void
registerInstruction(latte::SQ_CF_INST id,
                    TranslateFuncCF func);

void
registerInstruction(latte::SQ_CF_EXP_INST id,
                    TranslateFuncCF func);

void
registerInstruction(latte::SQ_TEX_INST id,
                    TranslateFuncTEX func);

void
registerInstruction(latte::SQ_OP2_INST id,
                    TranslateFuncALU func);

void
registerInstruction(latte::SQ_OP3_INST id,
                    TranslateFuncALU func);

void
registerInstruction(latte::SQ_OP2_INST id,
                    TranslateFuncALUReduction func);

void
registerInstruction(latte::SQ_OP3_INST id,
                    TranslateFuncALUReduction func);

void
insertLineStart(State &state);

void
insertLineEnd(State &state);

void
increaseIndent(State &state);

void
decreaseIndent(State &state);

void
registerCfFunctions();

void
registerExpFunctions();

void
registerOP2Functions();

void
registerOP2ReductionFunctions();

void
registerOP3ReductionFunctions();

void
registerOP3Functions();

void
registerTexFunctions();

void
registerVtxFunctions();

void
insertExportRegister(fmt::MemoryWriter &out, uint32_t gpr, latte::SQ_REL rel);

std::string
getExportRegister(uint32_t gpr, latte::SQ_REL rel);

bool
insertSelectVector(fmt::MemoryWriter &out, const std::string &src, latte::SQ_SEL selX, latte::SQ_SEL selY, latte::SQ_SEL selZ, latte::SQ_SEL selW, uint32_t numSels);

std::string
condenseSelections(latte::SQ_SEL &selX, latte::SQ_SEL &selY, latte::SQ_SEL &selZ, latte::SQ_SEL &selW, uint32_t &numSels);

} // namespace glsl2
