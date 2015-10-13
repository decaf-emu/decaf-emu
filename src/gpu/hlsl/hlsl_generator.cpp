#include <spdlog/spdlog.h>
#include <array_view.h>
#include "gpu/latte.h"
#include "gpu/hlsl/hlsl_generator.h"
#include "modules/gx2/gx2_shaders.h"

static const auto indentSize = 3u;

namespace hlsl
{

static bool translateBlocks(GenerateState &state, latte::shadir::BlockList &blocks);

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

} // namespace hlsl

namespace latte
{

bool
generateBody(Shader &shader, std::string &body)
{
   auto result = true;
   auto state = GenerateState {};
   state.shader = &shader;

   hlsl::intialise();
   result &= hlsl::translateBlocks(state, shader.blocks);
   body = state.out.c_str();

   return result;
}

static std::string
getGX2AttribFormatType(GX2AttribFormat::Format format)
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

bool
generateHLSL(const gsl::array_view<GX2AttribStream> &attribs, Shader &vertexShader, Shader &pixelShader, std::string &hlsl)
{
   auto result = true;
   std::string vertexBody, pixelBody;
   fmt::MemoryWriter shader;

   shader << "struct VSInput {\n";

   static const char *semantics[] = { "POSITION", "TEXCOORD", "PSIZE", "NORMAL" };

   for (auto &attrib : attribs) {
      shader
         << "   "
         << getGX2AttribFormatType(attrib.format)
         << " param" << attrib.location
         << " : "
         << semantics[attrib.location]
         << ";\n";
   }

   shader << "};\n\n";

   shader << "struct PSInput {\n";

   for (auto &exp : vertexShader.exports) {
      shader << "   ";
      switch (exp->type) {
      case latte::exp::Type::Position:
         shader
            << "float4 position"
            << (exp->dstReg - 60)
            << " : POSITION"
            << (exp->dstReg - 60)
            << ";\n";
         break;
      case latte::exp::Type::Parameter:
         shader
            << "float4 param"
            << exp->dstReg
            << " : TEXCOORD"
            << exp->dstReg
            << ";\n";
         break;
      case latte::exp::Type::Pixel:
         shader
            << "float4 color"
            << exp->dstReg
            << " : COLOR"
            << exp->dstReg
            << ";\n";
         break;
      }
   }

   shader << "};\n\n";

   shader << "struct PSOutput {\n";

   for (auto &exp : pixelShader.exports) {
      shader << "   ";
      switch (exp->type) {
      case latte::exp::Type::Position:
         shader
            << "float4 position"
            << (exp->dstReg - 60)
            << " : POSITION"
            << (exp->dstReg - 60)
            << ";\n";
         break;
      case latte::exp::Type::Parameter:
         shader
            << "float4 param"
            << exp->dstReg
            << " : TEXCOORD"
            << exp->dstReg
            << ";\n";
         break;
      case latte::exp::Type::Pixel:
         shader
            << "float4 color"
            << exp->dstReg
            << " : SV_TARGET"
            << exp->dstReg
            << ";\n";
         break;
      }
   }

   shader << "};\n\n";

   for (auto id : pixelShader.samplersUsed) {
      shader
         << "SamplerState g_sampler" << id
         << " : register(s" << id << ");\n";
   }

   for (auto id : pixelShader.resourcesUsed) {
      shader
         << "Texture2D g_texture" << id
         << " : register(t" << id << ");\n";
   }

   shader
      << "\n"
      << "PSInput VSMain(VSInput input)\n"
      << "{\n"
      << "PSInput out;\n";

   for (auto id : vertexShader.gprsUsed) {
      shader
         << "float4 R" << id << ";\n";
   }

   if (vertexShader.psUsed.size()) {
      shader
         << "float4 PS;\n"
         << "float4 PSo;\n";
   }

   if (vertexShader.pvUsed.size()) {
      shader
         << "float4 PV;\n"
         << "float4 PVo;\n";
   }

   for (auto &attrib : attribs) {
      shader << "R" << (attrib.location + 1) << " = float4(input.param" << attrib.location << ");\n";
   }

   result &= generateBody(vertexShader, vertexBody);
   shader << vertexBody;

   shader
      << "return out;\n"
      << "}\n";

   shader
      << "\n"
      << "PSOutput PSMain(PSInput input)\n"
      << "{\n"
      << "PSOutput out;\n";

   for (auto id : pixelShader.gprsUsed) {
      shader
         << "float4 R" << id << ";\n";
   }

   if (pixelShader.psUsed.size()) {
      shader
         << "float4 PS;\n"
         << "float4 PSo;\n";
   }

   if (pixelShader.pvUsed.size()) {
      shader
         << "float4 PV;\n"
         << "float4 PVo;\n";
   }

   for (auto &exp : vertexShader.exports) {
      if (exp->type == latte::exp::Type::Parameter) {
         shader << "R" << exp->dstReg << " = float4(input.param" << exp->dstReg << ");\n";
      }
   }

   result &= generateBody(pixelShader, pixelBody);
   shader << pixelBody;

   shader
      << "return out;\n"
      << "}\n";

   hlsl = shader.str();
   return result;
}


} // namespace latte
