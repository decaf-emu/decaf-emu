#pragma once
#include <exception>
#include <gsl.h>
#include <stdexcept>
#include <string>
#include <spdlog/details/format.h>
#include <vector>
#include "gpu/microcode/latte_instructions.h"

namespace glsl2
{

class translate_exception : public std::runtime_error
{
public:
   translate_exception(const std::string &m) :
      std::runtime_error(m)
   {
   }

private:
   std::string mMessage;
};

enum class SamplerUsage
{
   Invalid,

   Texture,
   Shadow
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
   std::array<latte::SQ_TEX_DIM, 16> samplerDim;
   bool uniformRegistersEnabled = false;
   bool uniformBlocksEnabled = false;

   // Output (maybe)
   std::string fileHeader;
   std::string codeHeader;
   std::string codeBody;
   std::vector<Export> exports;
   std::array<SamplerUsage, 16> samplerUsage;
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
   std::vector<std::string> postGroupWrites;
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
insertSelectValue(fmt::MemoryWriter &out, const std::string &src, latte::SQ_SEL sel);

bool
insertSelectVector(fmt::MemoryWriter &out, const std::string &src, latte::SQ_SEL selX, latte::SQ_SEL selY, latte::SQ_SEL selZ, latte::SQ_SEL selW, unsigned numSels);

std::string
condenseSelections(latte::SQ_SEL &selX, latte::SQ_SEL &selY, latte::SQ_SEL &selZ, latte::SQ_SEL &selW, unsigned &numSels);

} // namespace glsl2
