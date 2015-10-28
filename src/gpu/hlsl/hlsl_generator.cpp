#include <spdlog/spdlog.h>
#include <array_view.h>
#include "gpu/latte.h"
#include "gpu/hlsl/hlsl.h"
#include "gpu/hlsl/hlsl_generator.h"
#include "modules/gx2/gx2_shaders.h"

static const auto indentSize = 3u;

namespace hlsl
{

static bool translateBlocks(GenerateState &state, latte::shadir::BlockList &blocks);
static bool generateEpilog(fmt::MemoryWriter &output);
static bool generatePixelParams(const latte::Shader &vertexShader, fmt::MemoryWriter &output);
static bool generateVertexParams(const gsl::array_view<GX2AttribStream> &attribs, fmt::MemoryWriter &output);
static bool generateLocals(const latte::Shader &shader, fmt::MemoryWriter &output);
static bool generateProlog(const char *functionName, const char *inputStruct, const char *outputStruct, fmt::MemoryWriter &output);
static bool generateExports(const latte::Shader &shader, fmt::MemoryWriter &output);
static bool generateAttribs(const gsl::array_view<GX2AttribStream> &attribs, fmt::MemoryWriter &output);
static int getGX2AttribFormatElems(GX2AttribFormat::Value format);
static std::string getGX2AttribFormatType(GX2AttribFormat::Value format);

static std::map<latte::cf::inst, TranslateFuncCF> sGeneratorTableCf;
static std::map<latte::alu::op2, TranslateFuncALU> sGeneratorTableAluOp2;
static std::map<latte::alu::op3, TranslateFuncALU> sGeneratorTableAluOp3;
static std::map<latte::alu::op2, TranslateFuncALUReduction> sGeneratorTableAluOp2Reduction;
static std::map<latte::tex::inst, TranslateFuncTEX> sGeneratorTableTex;
static std::map<latte::exp::inst, TranslateFuncEXP> sGeneratorTableExport;


void
intialise()
{
   static bool initialised = false;

   if (!initialised) {
      registerCf();
      registerAluOP2();
      registerAluOP3();
      registerAluReduction();
      registerTex();
      registerExp();
   }
}


void
registerGenerator(latte::cf::inst ins, TranslateFuncCF func)
{
   sGeneratorTableCf[ins] = func;
}


void
registerGenerator(latte::alu::op2 ins, TranslateFuncALU func)
{
   sGeneratorTableAluOp2[ins] = func;
}


void
registerGenerator(latte::alu::op3 ins, TranslateFuncALU func)
{
   sGeneratorTableAluOp3[ins] = func;
}


void
registerGenerator(latte::alu::op2 ins, TranslateFuncALUReduction func)
{
   sGeneratorTableAluOp2Reduction[ins] = func;
}


void
registerGenerator(latte::tex::inst ins, TranslateFuncTEX func)
{
   sGeneratorTableTex[ins] = func;
}


void
registerGenerator(latte::exp::inst ins, TranslateFuncEXP func)
{
   sGeneratorTableExport[ins] = func;
}


void
beginLine(GenerateState &state)
{
   state.out << state.indent.c_str();
}


void
endLine(GenerateState &state)
{
   state.out << '\n';
}


void
increaseIndent(GenerateState &state)
{
   state.indent.append(indentSize, ' ');
}


void
decreaseIndent(GenerateState &state)
{
   if (state.indent.size() >= indentSize) {
      state.indent.resize(state.indent.size() - indentSize);
   }
}


static bool
translateInstruction(GenerateState &state, latte::shadir::Instruction *ins)
{
   switch (ins->insType) {
   case latte::shadir::Instruction::ControlFlow:
      {
         auto ins2 = reinterpret_cast<latte::shadir::CfInstruction *>(ins);
         auto itr = sGeneratorTableCf.find(ins2->id);

         if (itr == sGeneratorTableCf.end()) {
            state.out << "// Unimplemented " << latte::cf::name[ins2->id];
            return false;
         } else {
            return (*itr->second)(state, ins2);
         }
      }
      break;
   case latte::shadir::Instruction::TEX:
      {
         auto ins2 = reinterpret_cast<latte::shadir::TexInstruction *>(ins);
         auto itr = sGeneratorTableTex.find(ins2->id);

         if (itr == sGeneratorTableTex.end()) {
            state.out << "// Unimplemented " << latte::tex::name[ins2->id];
            return false;
         } else {
            return (*itr->second)(state, ins2);
         }
      }
      break;
   case latte::shadir::Instruction::AluReduction:
      {
         auto ins2 = reinterpret_cast<latte::shadir::AluReductionInstruction *>(ins);
         auto itr = sGeneratorTableAluOp2Reduction.find(ins2->op2);

         if (itr == sGeneratorTableAluOp2Reduction.end()) {
            state.out << "// Unimplemented " << latte::alu::op2info[ins2->op2].name;
            return false;
         } else {
            return (*itr->second)(state, ins2);
         }
      }
      break;
   case latte::shadir::Instruction::ALU:
      {
         auto ins2 = reinterpret_cast<latte::shadir::AluInstruction *>(ins);

         if (ins2->opType == latte::shadir::AluInstruction::OP2) {
            auto itr = sGeneratorTableAluOp2.find(ins2->op2);

            if (itr == sGeneratorTableAluOp2.end()) {
               state.out << "// Unimplemented " << latte::alu::op2info[ins2->op2].name;
               return false;
            } else {
               return (*itr->second)(state, ins2);
            }
         } else {
            auto itr = sGeneratorTableAluOp3.find(ins2->op3);

            if (itr == sGeneratorTableAluOp3.end()) {
               state.out << "// Unimplemented " << latte::alu::op3info[ins2->op3].name;
               return false;
            } else {
               return (*itr->second)(state, ins2);
            }
         }
      }
      break;
   case latte::shadir::Instruction::Export:
      {
         auto ins2 = reinterpret_cast<latte::shadir::ExportInstruction *>(ins);
         auto itr = sGeneratorTableExport.find(ins2->id);

         if (itr == sGeneratorTableExport.end()) {
            state.out << "// Unimplemented " << latte::exp::name[ins2->id];
            return false;
         } else {
            return (*itr->second)(state, ins2);
         }
      }
      break;
   }

   return false;
}


static bool
translateCodeBlock(GenerateState &state, latte::shadir::CodeBlock *block)
{
   auto result = true;

   for (auto &ins : block->code) {
      if (ins->groupPC != -1 && state.groupPC < ins->groupPC) {
         // If the last group both read and writes PV then we use the temp PVo / PSo to prevent overwrites of read
         if (state.shader->pvUsed.find(ins->groupPC - 1) != state.shader->pvUsed.end()) {
            if (state.shader->pvUsed.find(ins->groupPC - 2) != state.shader->pvUsed.end()) {
               beginLine(state);
               state.out << "PV = PVo;";
               endLine(state);
            }
         }

         if (state.shader->psUsed.find(ins->groupPC - 1) != state.shader->psUsed.end()) {
            if (state.shader->pvUsed.find(ins->groupPC - 2) != state.shader->pvUsed.end()) {
               beginLine(state);
               state.out << "PS = PSo;";
               endLine(state);
            }
         }

         // Begin new group
         beginLine(state);
         state.out << "// groupPC = " << ins->groupPC;
         endLine(state);
      }

      state.cfPC = ins->cfPC;
      state.groupPC = ins->groupPC;

      beginLine(state);
      result &= translateInstruction(state, ins);
      state.out << ";";
      endLine(state);
   }

   return result;
}


static bool
translateConditionalBlock(GenerateState &state, latte::shadir::ConditionalBlock *block)
{
   auto result = true;

   beginLine(state);
   state.out << "if (";
   result &= translateInstruction(state, block->condition);
   state.out << ") {";
   endLine(state);

   increaseIndent(state);
   result &= translateBlocks(state, block->inner);
   decreaseIndent(state);

   if (block->innerElse.size()) {
      beginLine(state);
      state.out << "} else {";
      endLine(state);

      increaseIndent(state);
      result &= translateBlocks(state, block->innerElse);
      decreaseIndent(state);

      beginLine(state);
      state.out << "}";
      endLine(state);
   } else {
      beginLine(state);
      state.out << "}";
      endLine(state);
   }

   return result;
}


static bool
translateLoopBlock(GenerateState &state, latte::shadir::LoopBlock *block)
{
   auto result = true;
   beginLine(state);
   state.out << "while (true) {";
   endLine(state);

   increaseIndent(state);
   result &= translateBlocks(state, block->inner);
   decreaseIndent(state);

   beginLine(state);
   state.out << "}";
   endLine(state);
   return result;
}


static bool
translateBlock(GenerateState &state, latte::shadir::Block *block)
{
   switch (block->type) {
   case latte::shadir::Block::CodeBlock:
      return translateCodeBlock(state, reinterpret_cast<latte::shadir::CodeBlock *>(block));
   case latte::shadir::Block::Conditional:
      return translateConditionalBlock(state, reinterpret_cast<latte::shadir::ConditionalBlock *>(block));
   case latte::shadir::Block::Loop:
      return translateLoopBlock(state, reinterpret_cast<latte::shadir::LoopBlock *>(block));
   }

   return false;
}


static bool
translateBlocks(GenerateState &state, latte::shadir::BlockList &blocks)
{
   auto result = true;

   for (auto &block : blocks) {
      result &= translateBlock(state, block.get());
   }

   return result;
}

static std::string
getGX2AttribFormatType(GX2AttribFormat::Value format)
{
   switch (format) {
   case GX2AttribFormat::UNORM_8:
      return "unorm float";
   case GX2AttribFormat::UNORM_8_8:
      return "unorm float2";
   case GX2AttribFormat::UNORM_8_8_8_8:
      return "unorm float4";
   case GX2AttribFormat::UINT_8:
      return "uint";
   case GX2AttribFormat::UINT_8_8:
      return "uint2";
   case GX2AttribFormat::UINT_8_8_8_8:
      return "uint4";
   case GX2AttribFormat::SNORM_8:
      return "snorm float";
   case GX2AttribFormat::SNORM_8_8:
      return "snorm float2";
   case GX2AttribFormat::SNORM_8_8_8_8:
      return "snorm float4";
   case GX2AttribFormat::SINT_8:
      return "int";
   case GX2AttribFormat::SINT_8_8:
      return "int2";
   case GX2AttribFormat::SINT_8_8_8_8:
      return "int4";
   case GX2AttribFormat::FLOAT_32:
      return "float";
   case GX2AttribFormat::FLOAT_32_32:
      return "float2";
   case GX2AttribFormat::FLOAT_32_32_32:
      return "float3";
   case GX2AttribFormat::FLOAT_32_32_32_32:
      return "float4";
   default:
      assert(0);
      return "float4";
   }
}

static int
getGX2AttribFormatElems(GX2AttribFormat::Value format)
{
   switch (format) {
   case GX2AttribFormat::UNORM_8:
      return 1;
   case GX2AttribFormat::UNORM_8_8:
      return 2;
   case GX2AttribFormat::UNORM_8_8_8_8:
      return 4;
   case GX2AttribFormat::UINT_8:
      return 1;
   case GX2AttribFormat::UINT_8_8:
      return 2;
   case GX2AttribFormat::UINT_8_8_8_8:
      return 4;
   case GX2AttribFormat::SNORM_8:
      return 1;
   case GX2AttribFormat::SNORM_8_8:
      return 2;
   case GX2AttribFormat::SNORM_8_8_8_8:
      return 4;
   case GX2AttribFormat::SINT_8:
      return 1;
   case GX2AttribFormat::SINT_8_8:
      return 2;
   case GX2AttribFormat::SINT_8_8_8_8:
      return 4;
   case GX2AttribFormat::FLOAT_32:
      return 1;
   case GX2AttribFormat::FLOAT_32_32:
      return 2;
   case GX2AttribFormat::FLOAT_32_32_32:
      return 3;
   case GX2AttribFormat::FLOAT_32_32_32_32:
      return 4;
   default:
      assert(0);
      return 4;
   }
}

static bool
generateAttribs(const gsl::array_view<GX2AttribStream> &attribs, fmt::MemoryWriter &output)
{
   for (const auto &attrib : attribs) {
      output
         << "   "
         << getGX2AttribFormatType(attrib.format)
         << " param" << attrib.location
         << " : POSITION" << attrib.location
         << ";\n";
   }

   return true;
}

static bool
generateExports(const latte::Shader &shader, fmt::MemoryWriter &output)
{
   for (const auto &exp : shader.exports) {
      output << "   ";

      switch (exp->type) {
      case latte::exp::Type::Position:
         if (exp->dstReg == 0) {
            output
               << "float4 position"
               << exp->dstReg
               << " : SV_POSITION"
               << ";\n";
         } else {
            output
               << "float4 position"
               << exp->dstReg
               << " : POSITION"
               << exp->dstReg
               << ";\n";
         }

         break;
      case latte::exp::Type::Parameter:
         output
            << "float4 param"
            << exp->dstReg
            << " : TEXCOORD"
            << exp->dstReg
            << ";\n";
         break;
      case latte::exp::Type::Pixel:
         output
            << "float4 color"
            << exp->dstReg
            << " : SV_TARGET"
            << exp->dstReg
            << ";\n";
         break;
      }
   }

   return true;
}

static bool
generateProlog(const char *functionName, const char *inputStruct, const char *outputStruct, fmt::MemoryWriter &output)
{
   output
      << "\n"
      << outputStruct << " " << functionName << "(" << inputStruct << " input)\n"
      << "{\n"
      << outputStruct << " result;\n";

   return true;
}

static bool
generateLocals(const latte::Shader &shader, fmt::MemoryWriter &output)
{
   for (auto id : shader.gprsUsed) {
      output
         << "float4 R" << id << ";\n";
   }

   if (shader.psUsed.size()) {
      output
         << "float PS;\n"
         << "float PSo;\n";
   }

   if (shader.pvUsed.size()) {
      output
         << "float4 PV;\n"
         << "float4 PVo;\n";
   }

   // TODO: Only print AR if needed
   output << "int4 AR;\n";
   return true;
}

static bool
generateVertexParams(const gsl::array_view<GX2AttribStream> &attribs, fmt::MemoryWriter &output)
{
   auto result = true;

   for (const auto &attrib : attribs) {
      int attribElems = getGX2AttribFormatElems(attrib.format);

      output
         << "R" << (attrib.location + 1)
         << " = ";

      switch (attribElems) {
      case 1:
         output << "float4(input.param" << attrib.location << ", 0, 0, 0);\n";
         break;
      case 2:
         output << "float4(input.param" << attrib.location << ", 0, 0);\n";
         break;
      case 3:
         output << "float4(input.param" << attrib.location << ", 0);\n";
         break;
      case 4:
         output << "input.param" << attrib.location << ";\n";
         break;
      default:
         assert(0);
         result = false;
      }
   }

   return result;
}

static bool
generatePixelParams(const latte::Shader &vertexShader, fmt::MemoryWriter &output)
{
   for (const auto &exp : vertexShader.exports) {
      if (exp->type == latte::exp::Type::Parameter) {
         output << "R" << exp->dstReg << " = (float4)input.param" << exp->dstReg << ";\n";
      }
   }

   return true;
}

static bool
generateEpilog(fmt::MemoryWriter &output)
{
   output
      << "return result;\n"
      << "}\n";

   return true;
}

bool
generateBody(latte::Shader &shader, std::string &body)
{
   auto result = true;
   auto state = GenerateState {};
   state.shader = &shader;

   intialise();
   result &= translateBlocks(state, shader.blocks);
   body = state.out.c_str();

   return result;
}

static size_t
getUniformTypeSlots(GX2UniformType::Value type)
{
   switch (type) {
   case GX2UniformType::Int:
      return 1;
   case GX2UniformType::Float:
      return 1;
   case GX2UniformType::Float2:
      return 1;
   case GX2UniformType::Float3:
      return 1;
   case GX2UniformType::Float4:
      return 1;
   case GX2UniformType::Int2:
      return 1;
   case GX2UniformType::Int3:
      return 1;
   case GX2UniformType::Int4:
      return 1;
   case GX2UniformType::Matrix4x4:
      return 4;
   default:
      assert(false);
      return 1;
   }
}

static size_t
getUniformCount(latte::Shader &shader, uint32_t varCount, GX2UniformVar *vars)
{
   if (varCount == 0 && shader.uniformsUsed.size() == 0) {
      return 0;
   }

   size_t highestOffset = 0, max = 0;

   for (auto i = 0u; i < varCount; ++i) {
      if (vars[i].offset >= highestOffset) {
         highestOffset = (vars[i].offset / 4) + getUniformTypeSlots(vars[i].type);
      }
   }

   if (shader.uniformsUsed.size()) {
      max = 1 + *std::max_element(shader.uniformsUsed.begin(), shader.uniformsUsed.end());
   }

   assert(max <= highestOffset);
   return std::max(max, highestOffset);
}

bool
generateHLSL(const gsl::array_view<GX2AttribStream> &attribs,
             GX2VertexShader *gx2Vertex,
             latte::Shader &vertexShader,
             GX2PixelShader *gx2Pixel,
             latte::Shader &pixelShader,
             std::string &hlsl)
{
   auto result = true;
   fmt::MemoryWriter output;

   output << "struct VSInput {\n";
   result &= generateAttribs(attribs, output);
   output << "};\n\n";

   output << "struct PSInput {\n";
   generateExports(vertexShader, output);
   output << "};\n\n";

   output << "struct PSOutput {\n";
   generateExports(pixelShader, output);
   output << "};\n\n";

   if (gx2Vertex->mode == GX2ShaderMode::UniformRegister) {
      auto count = getUniformCount(vertexShader, gx2Vertex->uniformVarCount, gx2Vertex->uniformVars);

      if (count) {
         output << "cbuffer VertUniforms {\n";
         output << "  float4 VC[" << count << "];\n";
         output << "};\n\n";
      }
   } else if (gx2Vertex->mode == GX2ShaderMode::UniformBlock) {
      for (auto i = 0u; i < gx2Vertex->uniformBlockCount; ++i) {
         output << "cbuffer VertBlock" << i << " {\n";
         output << "  float4 VUB" << i << "[" << (gx2Vertex->uniformBlocks[i].size / 16) << "];\n";
         output << "};\n\n";
      }
   } else {
      assert(false);
   }

   if (gx2Pixel->mode == GX2ShaderMode::UniformRegister) {
      auto count = getUniformCount(pixelShader, gx2Pixel->uniformVarCount, gx2Pixel->uniformVars);

      if (count) {
         output << "cbuffer PixUniforms {\n";
         output << "  float4 PC[" << count << "];\n";
         output << "};\n\n";
      }
   } else if (gx2Pixel->mode == GX2ShaderMode::UniformBlock) {
      for (auto i = 0u; i < gx2Pixel->uniformBlockCount; ++i) {
         output << "cbuffer PixBlock" << i << " {\n";
         output << "  float4 PUB" << i << "[" << (gx2Pixel->uniformBlocks[i].size / 16) << "];\n";
         output << "};\n\n";
      }
   } else {
      assert(false);
   }

   for (auto id : pixelShader.samplersUsed) {
      output
         << "SamplerState g_sampler" << id
         << " : register(s" << id << ");\n";
   }

   for (auto id : pixelShader.resourcesUsed) {
      output
         << "Texture2D g_texture" << id
         << " : register(t" << id << ");\n";
   }

   auto vertexBody = std::string { };
   result &= generateProlog("VSMain", "VSInput", "PSInput", output);
   result &= generateLocals(vertexShader, output);
   result &= generateVertexParams(attribs, output);
   result &= generateBody(vertexShader, vertexBody);
   output << vertexBody;
   result &= generateEpilog(output);

   auto pixelBody = std::string { };
   result &= generateProlog("PSMain", "PSInput", "PSOutput", output);
   result &= generateLocals(pixelShader, output);
   result &= generatePixelParams(vertexShader, output);
   result &= generateBody(pixelShader, pixelBody);
   output << pixelBody;
   result &= generateEpilog(output);

   hlsl = output.str();
   return result;
}


} // namespace latte
