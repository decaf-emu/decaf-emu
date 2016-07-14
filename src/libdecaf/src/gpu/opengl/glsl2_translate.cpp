#include "common/decaf_assert.h"
#include "common/log.h"
#include "glsl2_translate.h"
#include "glsl2_alu.h"
#include "glsl2_cf.h"
#include "gpu/microcode/latte_instructions.h"
#include "gpu/microcode/latte_decoders.h"
#include "gpu/microcode/latte_disassembler.h"
#include <map>

using namespace latte;

namespace glsl2
{

static const auto IndentSize = 2u;

static std::map<latte::SQ_CF_INST, TranslateFuncCF>
sInstructionMapCF;

static std::map<latte::SQ_CF_EXP_INST, TranslateFuncEXP>
sInstructionMapEXP;

static std::map<latte::SQ_OP2_INST, TranslateFuncALU>
sInstructionMapOP2;

static std::map<latte::SQ_OP3_INST, TranslateFuncALU>
sInstructionMapOP3;

static std::map<latte::SQ_OP2_INST, TranslateFuncALUReduction>
sInstructionMapOP2Reduction;

static std::map<latte::SQ_OP3_INST, TranslateFuncALUReduction>
sInstructionMapOP3Reduction;

static std::map<latte::SQ_TEX_INST, TranslateFuncTEX>
sInstructionMapTEX;

static void
translateTEX(State &state, const ControlFlowInst &cf)
{
   auto addr = cf.word0.ADDR;
   auto count = (cf.word1.COUNT() + 1) | (cf.word1.COUNT_3() << 3);
   auto clause = reinterpret_cast<const TextureFetchInst *>(state.binary.data() + 8 * addr);

   insertLineStart(state);
   state.out.write("// {:02} ", state.cfPC);
   latte::disassembler::disassembleCF(state.out, cf);
   insertLineEnd(state);

   condStart(state, cf.word1.COND());

   for (auto i = 0u; i < count; ++i) {
      const auto &tex = clause[i];
      auto id = tex.word0.TEX_INST();
      auto name = getInstructionName(id);
      auto itr = sInstructionMapTEX.find(id);

      insertLineStart(state);
      state.out.write("// {:02} ", state.groupPC);
      latte::disassembler::disassembleTexInstruction(state.out, cf, tex);
      insertLineEnd(state);

      if (itr != sInstructionMapTEX.end()) {
         itr->second(state, cf, tex);
      } else {
         throw translate_exception(fmt::format("Unimplemented TEX instruction {} {}", id, name));
      }
   }

   condEnd(state);
   state.out << '\n';
}

static void
translateVTX(State &state, const ControlFlowInst &cf)
{
   throw translate_exception("Unable to decode VTX instruction");
}

static void
translateNormal(State &state, const ControlFlowInst &cf)
{
   auto result = true;
   auto id = cf.word1.CF_INST();
   auto name = getInstructionName(id);

   switch (id) {
   case SQ_CF_INST_WAIT_ACK:
   case SQ_CF_INST_TEX_ACK:
   case SQ_CF_INST_VTX_ACK:
   case SQ_CF_INST_VTX_TC_ACK:
      throw translate_exception(fmt::format("Unable to decode instruction {} {}", id, name));
   }

   if (id == SQ_CF_INST_TEX) {
      translateTEX(state, cf);
   } else if (id == SQ_CF_INST_VTX || id == SQ_CF_INST_VTX_TC) {
      translateVTX(state, cf);
   } else {
      auto itr = sInstructionMapCF.find(id);

      insertLineStart(state);
      state.out.write("// {:02} ", state.cfPC);
      latte::disassembler::disassembleCF(state.out, cf);
      insertLineEnd(state);

      if (itr != sInstructionMapCF.end()) {
         itr->second(state, cf);
      } else {
         throw translate_exception(fmt::format("Unimplemented CF instruction {} {}", id, name));
      }

      state.out << '\n';
   }
}

static void
translateALUReduction(State &state, const ControlFlowInst &cf, AluGroup &group)
{
   auto units = AluGroupUnits {};
   auto reduction = std::array<AluInst, 4> {};
   std::memset(reduction.data(), 0, reduction.size());

   // Ensure we have at least 4 instructions in this group
   if (group.instructions.size() < 4) {
      throw translate_exception(fmt::format("Expected at least 4 instructions in reduction group, found {}", group.instructions.size()));
   }

   // Get all the instructions in this reduction group, sorted by unit
   for (auto i = 0u; i < group.instructions.size(); ++i) {
      auto &inst = group.instructions[i];
      auto unit = units.addInstructionUnit(inst);

      if (unit == SQ_CHAN_T) {
         continue;
      }

      reduction[unit] = inst;
   }

   // For sanity, let's ensure every instruction in this reduction group
   // has the same instruction id and the same CLAMP + OMOD values
   for (auto i = 1u; i < reduction.size(); ++i) {
      if (reduction[0].word1.ENCODING() == SQ_ALU_OP2) {
         if (reduction[i].op2.ALU_INST() != reduction[0].op2.ALU_INST()) {
            throw translate_exception("Expected every instruction in reduction group to be the same.");
         }

         if (reduction[i].op2.OMOD() != reduction[0].op2.OMOD()) {
            throw translate_exception("Expected every instruction in reduction group to have the same output modifier.");
         }
      } else {
         if (reduction[i].op3.ALU_INST() != reduction[0].op3.ALU_INST()) {
            throw translate_exception("Expected every instruction in reduction group to be the same.");
         }
      }

      if (reduction[i].word1.CLAMP() != reduction[0].word1.CLAMP()) {
         throw translate_exception("Expected every instruction in reduction group to have the same clamp value.");
      }
   }

   // Find translate function
   auto func = TranslateFuncALUReduction { nullptr };

   if (reduction[0].word1.ENCODING() == SQ_ALU_OP2) {
      auto id = reduction[0].op2.ALU_INST();
      auto itr = sInstructionMapOP2Reduction.find(id);
      auto name = getInstructionName(id);

      if (itr != sInstructionMapOP2Reduction.end()) {
         func = itr->second;
      } else {
         throw translate_exception(fmt::format("Unimplemented ALU OP2 Reduction instruction {} {}", id, name));
      }
   } else {
      auto id = reduction[0].op3.ALU_INST();
      auto itr = sInstructionMapOP3Reduction.find(id);
      auto name = getInstructionName(id);

      if (itr != sInstructionMapOP3Reduction.end()) {
         func = itr->second;
      } else {
         throw translate_exception(fmt::format("Unimplemented ALU OP3 Reduction instruction {} {}", id, name));
      }
   }

   // Print disassembly
   insertLineStart(state);
   state.out.write("// {:02} Reduction", state.groupPC);
   insertLineEnd(state);

   for (auto i = 0u; i < reduction.size(); ++i) {
      insertLineStart(state);
      state.out.write("// ");
      latte::disassembler::disassembleAluInstruction(state.out, cf, reduction[i], state.groupPC, static_cast<SQ_CHAN>(i), state.literals);
      insertLineEnd(state);
   }

   // Translate the instruction!
   if (func) {
      func(state, cf, reduction);
   }
}

static bool
isReductionInstruction(const AluInst &inst)
{
   auto flags = SQ_ALU_FLAG_NONE;

   if (inst.word1.ENCODING() == SQ_ALU_OP2) {
      auto id = inst.op2.ALU_INST();
      flags = getInstructionFlags(id);
   } else {
      auto id = inst.op3.ALU_INST();
      flags = getInstructionFlags(id);
   }

   return !!(flags & SQ_ALU_FLAG_REDUCTION);
}

static void
translateControlFlowALU(State &state, const ControlFlowInst &cf)
{
   auto id = cf.alu.word1.CF_INST();
   auto name = getInstructionName(cf.alu.word1.CF_INST());
   auto addr = cf.alu.word0.ADDR();
   auto count = cf.alu.word1.COUNT() + 1;
   auto clause = reinterpret_cast<const AluInst *>(state.binary.data() + 8 * addr);
   auto didPushBefore = false;

   insertLineStart(state);
   state.out.write("// {:02} ", state.cfPC);
   latte::disassembler::disassembleCfALUInstruction(state.out, cf);
   insertLineEnd(state);

   switch (id) {
   case SQ_CF_INST_ALU_PUSH_BEFORE:
      insertPush(state);
      state.out << '\n';
      break;
   case SQ_CF_INST_ALU_BREAK:
      throw translate_exception("Unimplemented SQ_CF_INST_ALU_BREAK");
      break;
   case SQ_CF_INST_ALU_CONTINUE:
      throw translate_exception("Unimplemented SQ_CF_INST_ALU_CONTINUE");
      break;
   }

   condStart(state, latte::SQ_CF_COND_ACTIVE);

   for (size_t slot = 0u; slot < count; ) {
      auto units = AluGroupUnits {};
      auto group = AluGroup { clause + slot };
      auto didReduction = false;
      auto updatePreviousVector = false;
      auto updatePreviousScalar = false;
      std::vector<std::string> destWrites;
      state.literals = group.literals;

      for (auto j = 0u; j < group.instructions.size(); ++j) {
         auto &inst = group.instructions[j];
         auto unit = units.addInstructionUnit(inst);
         auto func = TranslateFuncALU { nullptr };
         auto flags = SQ_ALU_FLAG_NONE;
         auto writeMask = true;
         const char *name = nullptr;
         state.unit = unit;

         // Read the writeMask
         if (inst.word1.ENCODING() == SQ_ALU_OP2) {
            if (inst.op2.ALU_INST() == latte::SQ_OP2_INST_MOVA_INT
             || inst.op2.ALU_INST() == latte::SQ_OP2_INST_MOVA_FLOOR) {
               // These don't write through PV.
               writeMask = false;
            } else {
               writeMask = inst.op2.WRITE_MASK();
            }
         }

         // Add the output register to destWrites
         if (writeMask) {
            fmt::MemoryWriter writeBuf;
            auto gpr = inst.word1.DST_GPR().get();
            auto writeUnit = inst.word1.DST_CHAN().get();

            writeBuf << "R[" << gpr << "].";

            // Special case: DOT4 instructions always write to X.
            if (inst.word1.ENCODING() == SQ_ALU_OP2
                && (inst.op2.ALU_INST() == latte::SQ_OP2_INST_DOT4
                 || inst.op2.ALU_INST() == latte::SQ_OP2_INST_DOT4_IEEE)) {
               writeUnit = SQ_CHAN_X;
            }

            insertChannel(writeBuf, writeUnit);
            writeBuf << " = ";

            switch (unit) {
            case SQ_CHAN_X:
               writeBuf << "PVo.x;";
               break;
            case SQ_CHAN_Y:
               writeBuf << "PVo.y;";
               break;
            case SQ_CHAN_Z:
               writeBuf << "PVo.z;";
               break;
            case SQ_CHAN_W:
               writeBuf << "PVo.w;";
               break;
            case SQ_CHAN_T:
               writeBuf << "PSo;";
               break;
            }

            destWrites.push_back(writeBuf.str());
         }

         // Process all reduction instructions once as a group
         if (isReductionInstruction(inst)) {
            if (!didReduction) {
               translateALUReduction(state, cf, group);
            }

            didReduction = true;
            updatePreviousVector = true;
            continue;
         }

         if (inst.word1.ENCODING() == SQ_ALU_OP2) {
            auto id = inst.op2.ALU_INST();
            auto itr = sInstructionMapOP2.find(id);
            flags = getInstructionFlags(id);
            name = getInstructionName(id);

            if (itr != sInstructionMapOP2.end()) {
               func = itr->second;
            } else {
               throw translate_exception(fmt::format("Unimplemented ALU OP2 instruction {} {}", id, name));
            }
         } else {
            auto id = inst.op3.ALU_INST();
            auto itr = sInstructionMapOP3.find(id);
            flags = getInstructionFlags(id);
            name = getInstructionName(id);

            if (itr != sInstructionMapOP3.end()) {
               func = itr->second;
            } else {
               throw translate_exception(fmt::format("Unimplemented ALU OP3 instruction {} {}", id, name));
            }
         }

         if (unit < SQ_CHAN_T) {
            updatePreviousVector = true;
         } else {
            updatePreviousScalar = true;
         }

         insertLineStart(state);
         state.out.write("// {:02} ", state.groupPC);
         latte::disassembler::disassembleAluInstruction(state.out, cf, inst, state.groupPC, state.unit, state.literals);
         insertLineEnd(state);

         if (func) {
            func(state, cf, inst);
         }
      }

      for (auto &writeStmt : destWrites) {
         insertLineStart(state);
         state.out << writeStmt;
         insertLineEnd(state);
      }

      if (updatePreviousVector) {
         insertLineStart(state);
         state.out << "PV = PVo;";
         insertLineEnd(state);
      }

      if (updatePreviousScalar) {
         insertLineStart(state);
         state.out << "PS = PSo;";
         insertLineEnd(state);
      }

      slot = group.getNextSlot(slot);
      state.groupPC++;

      if (slot < count) {
         state.out << '\n';
      }
   }

   condEnd(state);

   switch (id) {
   case SQ_CF_INST_ALU_POP_AFTER:
      state.out << '\n';
      insertPop(state, 1);
      break;
   case SQ_CF_INST_ALU_POP2_AFTER:
      state.out << '\n';
      insertPop(state, 2);
      break;
   case SQ_CF_INST_ALU_ELSE_AFTER:
      state.out << '\n';
      insertElse(state);
      break;
   }

   state.out << '\n';
}

static void
initialise()
{
   static bool didRegister = false;

   if (didRegister) {
      return;
   }

   didRegister = true;
   registerCfFunctions();
   registerExpFunctions();
   registerTexFunctions();
   registerVtxFunctions();
   registerOP2Functions();
   registerOP3Functions();
   registerOP2ReductionFunctions();
   registerOP3ReductionFunctions();
}

void
registerInstruction(SQ_CF_INST id, TranslateFuncCF func)
{
   sInstructionMapCF[id] = func;
}

void
registerInstruction(SQ_CF_EXP_INST id, TranslateFuncCF func)
{
   sInstructionMapEXP[id] = func;
}

void
registerInstruction(SQ_TEX_INST id, TranslateFuncTEX func)
{
   sInstructionMapTEX[id] = func;
}

void
registerInstruction(SQ_OP2_INST id, TranslateFuncALU func)
{
   sInstructionMapOP2[id] = func;
}

void
registerInstruction(SQ_OP3_INST id, TranslateFuncALU func)
{
   sInstructionMapOP3[id] = func;
}

void
registerInstruction(latte::SQ_OP2_INST id,
                    TranslateFuncALUReduction func)
{
   sInstructionMapOP2Reduction[id] = func;
}

void
registerInstruction(latte::SQ_OP3_INST id,
                    TranslateFuncALUReduction func)
{
   sInstructionMapOP3Reduction[id] = func;
}

void
insertLineStart(State &state)
{
   state.out << state.indent;
}

void
insertLineEnd(State &state)
{
   state.out << '\n';
}

void
increaseIndent(State &state)
{
   state.indent.append(IndentSize, ' ');
}

void
decreaseIndent(State &state)
{
   if (state.indent.size() >= IndentSize) {
      state.indent.resize(state.indent.size() - IndentSize);
   }
}

static void
translateExport(State &state, const ControlFlowInst &cf)
{
   auto id = cf.exp.word1.CF_INST();
   auto itr = sInstructionMapEXP.find(id);

   insertLineStart(state);
   state.out.write("// {:02} ", state.cfPC);
   latte::disassembler::disassembleExpInstruction(state.out, cf);
   insertLineEnd(state);

   if (itr != sInstructionMapEXP.end()) {
      itr->second(state, cf);
   } else {
      throw translate_exception(fmt::format("Unimplemented EXP instruction {} {}", id, getInstructionName(id)));
   }

   state.out << '\n';
}

static void
insertFileHeader(State &state)
{
   auto &out = state.outFileHeader;

   out
      << "#version 420 core\n"
      << "#extension GL_ARB_texture_gather : enable\n"
      << "#define PUSH(stack, stackIndex, activeMask) stack[stackIndex++] = activeMask\n"
      << "#define POP(stack, stackIndex, activeMask) activeMask = stack[--stackIndex]\n"
      << "\n"
      << "bool activeMask;\n"
      << "bool predicateRegister;\n"
      << "int stackIndex;\n"
      << "bool stack[16];\n"
      << "\n";

   if (!state.shader) {
      return;
   }

   if (state.shader->uniformRegistersEnabled) {
      if (state.shader->type == Shader::PixelShader) {
         out << "uniform vec4 PR[256];\n";
      } else if (state.shader->type == Shader::VertexShader) {
         out << "uniform vec4 VR[256];\n";
      } else if (state.shader->type == Shader::GeometryShader) {
         out << "uniform vec4 GR[256];\n";
      }
   }

   if (state.shader->uniformBlocksEnabled) {
      for (auto id = 0u; id < 8; ++id) {
         out << "layout (binding = ";

         if (state.shader->type == Shader::VertexShader) {
            out << id;
         } else {
            out << (16 + id);
         }

         out
            << ") uniform "
            << "UniformBlock_"
            << id
            << " {\n"
            << "   vec4 values[];\n"
            << "} UB_"
            << id << ";\n";
      }
   }

   // Samplers
   for (auto id = 0u; id < state.shader->samplers.size(); ++id) {
      auto type = state.shader->samplers[id];

      if (!state.shader->samplerUsed[id]) {
         continue;
      }

      out << "layout (binding = " << id << ") uniform ";

      switch (type) {
      case SamplerType::Sampler1D:
         out << "sampler1D";
         break;
      case SamplerType::Sampler2D:
         out << "sampler2D";
         break;
      case SamplerType::Sampler3D:
         out << "sampler3D";
         break;
      case SamplerType::SamplerCube:
         out << "samplerCube";
         break;
      case SamplerType::Sampler2DRect:
         out << "sampler2DRect";
         break;
      case SamplerType::Sampler1DArray:
         out << "sampler1DArray";
         break;
      case SamplerType::Sampler2DArray:
         out << "sampler2DArray";
         break;
      case SamplerType::SamplerCubeArray:
         out << "samplerCubeArray";
         break;
      case SamplerType::SamplerBuffer:
         out << "samplerBuffer";
         break;
      case SamplerType::Sampler2DMS:
         out << "sampler2DMS";
         break;
      case SamplerType::Sampler2DMSArray:
         out << "sampler2DMSArray";
         break;
      case SamplerType::Sampler1DShadow:
         out << "sampler1DShadow";
         break;
      case SamplerType::Sampler2DShadow:
         out << "sampler2DShadow";
         break;
      case SamplerType::SamplerCubeShadow:
         out << "samplerCubeShadow";
         break;
      case SamplerType::Sampler2DRectShadow:
         out << "sampler2DRectShadow";
         break;
      case SamplerType::Sampler1DArrayShadow:
         out << "sampler1DArrayShadow";
         break;
      case SamplerType::Sampler2DArrayShadow:
         out << "sampler2DArrayShadow";
         break;
      case SamplerType::SamplerCubeArrayShadow:
         out << "samplerCubeArrayShadow";
         break;
      default:
         throw translate_exception(fmt::format("Unsupported sampler type: {}", static_cast<unsigned>(type)));
      }

      out << " sampler_" << id << ";\n";
   }

   if (state.shader->type == Shader::VertexShader) {
      out
         << "out gl_PerVertex {\n"
         << "   vec4 gl_Position;\n"
         << "};\n";
   }
}

static void
insertCodeHeader(State &state)
{
   auto &out = state.outCodeHeader;

   out
      << "vec4 R[128];\n"
      << "vec4 PV;\n"
      << "vec4 PVo;\n"
      << "float PS;\n"
      << "float PSo;\n"
      << "vec4 texTmp;\n"
      << "ivec4 AR;\n"
      << "int AL;\n";

   if (state.shader) {
      for (auto &exp : state.shader->exports) {
         switch (exp.type) {
         case SQ_EXPORT_POS:
            out << "vec4 exp_position_" << exp.id << ";\n";
            break;
         case SQ_EXPORT_PARAM:
            out << "vec4 exp_param_" << exp.id << ";\n";
            break;
         case SQ_EXPORT_PIXEL:
            out << "vec4 exp_pixel_" << exp.id << ";\n";
            break;
         }
      }
   }

   out
      << "\n"
      << "activeMask = true;\n"
      << "stackIndex = 0;\n";

   if (state.shader && state.shader->type == Shader::VertexShader) {
      out << "R[0] = vec4(intBitsToFloat(gl_VertexID), intBitsToFloat(gl_InstanceID), 0.0, 0.0);\n";
   }
}

bool
translate(Shader &shader, const gsl::span<const uint8_t> &binary)
{
   State state;
   state.binary = binary;
   state.shader = &shader;
   state.shader->samplerUsed.fill(false);
   initialise();

   try {
      for (auto i = 0; i < binary.size(); i += sizeof(ControlFlowInst)) {
         auto cf = *reinterpret_cast<const ControlFlowInst *>(binary.data() + i);
         auto id = cf.word1.CF_INST();

         switch (cf.word1.CF_INST_TYPE().get()) {
         case SQ_CF_INST_TYPE_NORMAL:
            translateNormal(state, cf);
            break;
         case SQ_CF_INST_TYPE_EXPORT:
            translateExport(state, cf);
            break;
         case SQ_CF_INST_TYPE_ALU:
         case SQ_CF_INST_TYPE_ALU_EXTENDED:
            translateControlFlowALU(state, cf);
            break;
         default:
            throw translate_exception("Invalid top level instruction type");
         }

         if (cf.word1.CF_INST_TYPE() == SQ_CF_INST_TYPE_NORMAL
          || cf.word1.CF_INST_TYPE() == SQ_CF_INST_TYPE_EXPORT) {
            if (cf.word1.END_OF_PROGRAM()) {
               break;
            }
         }

         state.cfPC++;
      }
   } catch (translate_exception e) {
      auto assembly = disassemble(binary);
      gLog->critical("GLSL translate exception: {}\nDisassembly:\n{}", e.what(), assembly);
      decaf_abort(fmt::format("GLSL translate exception: {}", e.what()));
   }

   insertFileHeader(state);
   insertCodeHeader(state);

   shader.codeBody = state.out.str();
   shader.fileHeader = state.outFileHeader.str();
   shader.codeHeader = state.outCodeHeader.str();
   return true;
}

} // namespace glsl2
