#define NOMINMAX
#include <spdlog/spdlog.h>
#include "latte.h"
#include "latte_opcodes.h"

static const uint32_t indentSize = 2;
static const uint32_t instrNamePad = 16;
static const uint32_t groupCounterSize = 2;

namespace latte
{

struct DisassembleState
{
   fmt::MemoryWriter out;
   std::string indent;
   uint32_t *words;
   uint32_t wordCount;
   uint32_t group;
   uint32_t cfPC;
};

static bool disassembleNormal(DisassembleState &state, latte::cf::inst id, latte::cf::Instruction &cf);
static bool disassembleExport(DisassembleState &state, latte::cf::inst id, latte::cf::Instruction &cf);
static bool disassembleALU(DisassembleState &state, latte::cf::inst id, latte::cf::Instruction &cf);
static bool disassembleTEX(DisassembleState &state, latte::cf::inst id, latte::cf::Instruction &cf);

static void beginLine(DisassembleState &state);
static void endLine(DisassembleState &state);
static void increaseIndent(DisassembleState &state);
static void decreaseIndent(DisassembleState &state);

static void writeSelectName(DisassembleState &state, uint32_t select);
static void writeAluSource(DisassembleState &state, uint32_t *dwBase, uint32_t sel, uint32_t rel, uint32_t chan, uint32_t neg, bool abs);
static void writeRegisterName(DisassembleState &state, uint32_t sel);

static uint32_t getUnit(bool units[5], latte::alu::Instruction &alu);
static bool isVectorOnly(latte::alu::Instruction &alu);
static bool isTranscendentalOnly(latte::alu::Instruction &alu);

bool disassemble(std::string &out, uint8_t *binary, uint32_t size)
{
   bool result = true;
   auto state = DisassembleState { };

   assert((size % 4) == 0);
   state.words = reinterpret_cast<uint32_t*>(binary);
   state.wordCount = size / 4;

   state.cfPC = 0;
   state.group = 0;

   for (auto i = 0u; i < state.wordCount; i += 2) {
      auto cf = *reinterpret_cast<latte::cf::Instruction*>(state.words + i);
      auto id = static_cast<latte::cf::inst>(cf.word1.inst);

      beginLine(state);
      state.out << fmt::pad(state.cfPC, 2, '0') << ' ';

      switch (cf.type) {
      case latte::cf::Type::Normal:
         result &= disassembleNormal(state, id, cf);
         break;
      case latte::cf::Type::Export:
         if (id == latte::exp::EXP || id == latte::exp::EXP_DONE) {
            result &= disassembleExport(state, id, cf);
         } else {
            assert(false);
         }
         break;
      case latte::cf::Type::Alu:
      case latte::cf::Type::AluExtended:
         result &= disassembleALU(state, id, cf);
         break;
      }

      endLine(state);
      state.cfPC++;

      if ((cf.type == latte::cf::Type::Normal || cf.type == latte::cf::Type::Export) && cf.word1.endOfProgram) {
         state.out << "END_OF_PROGRAM";
         endLine(state);
         break;
      }
   }

   out = state.out.c_str();
   return result;
}

void beginLine(DisassembleState &state)
{
   state.out << state.indent.c_str();
}

void endLine(DisassembleState &state)
{
   state.out << '\n';
}

void increaseIndent(DisassembleState &state)
{
   state.indent.append(indentSize, ' ');
}

void decreaseIndent(DisassembleState &state)
{
   if (state.indent.size() >= indentSize) {
      state.indent.resize(state.indent.size() - indentSize);
   }
}

void
writeSelectName(DisassembleState &state, uint32_t select)
{
   switch (select) {
   case latte::alu::Select::X:
      state.out << 'x';
      break;
   case latte::alu::Select::Y:
      state.out << 'y';
      break;
   case latte::alu::Select::Z:
      state.out << 'z';
      break;
   case latte::alu::Select::W:
      state.out << 'w';
      break;
   case latte::alu::Select::One:
      state.out << '1';
      break;
   case latte::alu::Select::Zero:
      state.out << '0';
      break;
   case latte::alu::Select::Mask:
      state.out << '_';
      break;
   default:
      state.out << '?';
      assert(false);
   }
}

bool
isTranscendentalOnly(latte::alu::Instruction &alu)
{
   latte::alu::Opcode opcode;

   if (alu.word1.encoding == latte::alu::Encoding::OP2) {
      opcode = latte::alu::op2info[alu.op2.inst];
   } else {
      opcode = latte::alu::op3info[alu.op3.inst];
   }

   if (opcode.flags & latte::alu::Opcode::Vector) {
      return false;
   }

   if (opcode.flags & latte::alu::Opcode::Transcendental) {
      return true;
   }

   return false;
}

bool
isVectorOnly(latte::alu::Instruction &alu)
{
   latte::alu::Opcode opcode;

   if (alu.word1.encoding == latte::alu::Encoding::OP2) {
      opcode = latte::alu::op2info[alu.op2.inst];
   } else {
      opcode = latte::alu::op3info[alu.op3.inst];
   }

   if (opcode.flags & latte::alu::Opcode::Transcendental) {
      return false;
   }

   if (opcode.flags & latte::alu::Opcode::Vector) {
      return true;
   }

   return false;
}

uint32_t
getUnit(bool units[5], latte::alu::Instruction &alu)
{
   bool isTrans = false;
   auto elem = alu.word1.dstChan;

   if (isTranscendentalOnly(alu)) {
      isTrans = true;
   } else if (isVectorOnly(alu)) {
      isTrans = false;
   } else if (units[elem]) {
      isTrans = true;
   } else {
      isTrans = false;
   }

   if (isTrans) {
      assert(units[4] == false);
      units[4] = true;
      return 4;
   } else {
      units[elem] = true;
      return elem;
   }
}

void
writeRegisterName(DisassembleState &state, uint32_t sel)
{
   if (sel > latte::NumGPR - latte::NumTempRegisters) {
      state.out << "T" << (latte::NumGPR - sel - 1);
   } else {
      state.out << "R" << sel;
   }
}

void
writeAluSource(DisassembleState &state, uint32_t *dwBase, uint32_t sel, uint32_t rel, uint32_t chan, uint32_t neg, bool abs)
{
   if (rel != 0) {
      // TODO: relative address
      state.out << "__UNK_REL(" << rel << ")__";
   }

   if (abs) {
      state.out << "ABS(";
   }

   // Negate
   if (neg) {
      state.out << "-";
   }

   // Sel
   if (sel >= latte::alu::Source::RegisterFirst && sel <= latte::alu::Source::RegisterLast) {
      writeRegisterName(state, sel);
   } else if (sel >= latte::alu::Source::KcacheBank0First && sel <= latte::alu::Source::KcacheBank0Last) {
      state.out << "KCACHEBANK0_" << (sel - latte::alu::Source::KcacheBank0First);
   } else if (sel >= latte::alu::Source::KcacheBank1First && sel <= latte::alu::Source::KcacheBank1Last) {
      state.out << "KCACHEBANK1_" << (sel - latte::alu::Source::KcacheBank1First);
   } else if (sel == latte::alu::Source::Src1DoubleLSW) {
      state.out << "__UNK_Src1DoubleLSW__";
   } else if (sel == latte::alu::Source::Src1DoubleMSW) {
      state.out << "__UNK_Src1DoubleMSW__";
   } else if (sel == latte::alu::Source::Src05DoubleLSW) {
      state.out << "__UNK_Src05DoubleLSW__";
   } else if (sel == latte::alu::Source::Src05DoubleMSW) {
      state.out << "__UNK_Src05DoubleMSW__";
   } else if (sel == latte::alu::Source::Src0Float) {
      state.out << "0.0f";
   } else if (sel == latte::alu::Source::Src1Float) {
      state.out << "1.0f";
   } else if (sel == latte::alu::Source::Src1Integer) {
      state.out << "1";
   } else if (sel == latte::alu::Source::SrcMinus1Integer) {
      if (neg) {
         state.out << "1";
      } else {
         state.out << "-1";
      }
   } else if (sel == latte::alu::Source::Src05Float) {
      state.out << "0.5f";
   } else if (sel == latte::alu::Source::SrcLiteral) {
      state.out << *reinterpret_cast<float*>(dwBase + chan) << "f";
   } else if (sel == latte::alu::Source::SrcPreviousScalar) {
      state.out << "PS" << (state.group - 1);
   } else if (sel == latte::alu::Source::SrcPreviousVector) {
      state.out << "PV" << (state.group - 1);
   } else if (sel >= latte::alu::Source::CfileConstantsFirst && sel <= latte::alu::Source::CfileConstantsLast) {
      state.out << "C" << (sel - latte::alu::Source::CfileConstantsFirst);
   } else {
      assert(false);
   }

   // Chan
   if (chan == latte::alu::Channel::X) {
      state.out << ".x";
   } else if (chan == latte::alu::Channel::Y) {
      state.out << ".y";
   } else if (chan == latte::alu::Channel::Z) {
      state.out << ".z";
   } else if (chan == latte::alu::Channel::W) {
      state.out << ".w";
   }

   if (abs) {
      state.out << ")";
   }
}

bool disassembleNormal(DisassembleState &state, latte::cf::inst id, latte::cf::Instruction &cf)
{
   auto name = latte::cf::name[id];

   switch (id) {
   case latte::cf::TEX:
      return disassembleTEX(state, id, cf);
   case latte::cf::LOOP_START:
   case latte::cf::LOOP_START_DX10:
   case latte::cf::LOOP_START_NO_AL:
      state.out
         << name
         << " FAIL_JUMP_ADDR(" << cf.word0.addr << ")";
      endLine(state);
      increaseIndent(state);
      break;
   case latte::cf::LOOP_END:
      state.out
         << name
         << " PASS_JUMP_ADDR(" << cf.word0.addr << ")";
      endLine(state);
      decreaseIndent(state);
      break;
   case latte::cf::ELSE:
   case latte::cf::JUMP:
      state.out
         << name
         << " POP_CNT(" << cf.word1.popCount << ")"
         << " ADDR(" << cf.word0.addr << ")";
      endLine(state);
      break;
   case latte::cf::NOP:
   case latte::cf::CALL_FS:
   case latte::cf::END_PROGRAM:
      state.out
         << name;
      endLine(state);
      break;
   case latte::cf::VTX:
   case latte::cf::VTX_TC:
   case latte::cf::LOOP_CONTINUE:
   case latte::cf::LOOP_BREAK:
   case latte::cf::POP_JUMP:
   case latte::cf::CALL:
   case latte::cf::RETURN:
   case latte::cf::EMIT_VERTEX:
   case latte::cf::EMIT_CUT_VERTEX:
   case latte::cf::CUT_VERTEX:
   case latte::cf::KILL:
   case latte::cf::PUSH:
   case latte::cf::PUSH_ELSE:
   case latte::cf::POP:
   case latte::cf::POP_PUSH:
   case latte::cf::POP_PUSH_ELSE:
   case latte::cf::WAIT_ACK:
   case latte::cf::TEX_ACK:
   case latte::cf::VTX_ACK:
   case latte::cf::VTX_TC_ACK:
   default:
      assert(false);
      break;
   }

   return true;
}

bool disassembleExport(DisassembleState &state, latte::cf::inst id, latte::cf::Instruction &cf)
{
   auto eid = static_cast<latte::exp::inst>(cf.expWord1.inst);
   auto name = latte::exp::name[id];
   auto type = static_cast<latte::exp::Type::Type>(cf.expWord0.type);

   state.out
      << name
      << ": ";

   switch (type) {
   case latte::exp::Type::Pixel:
      state.out << "PIX";
      break;
   case latte::exp::Type::Position:
      state.out << "POS";
      break;
   case latte::exp::Type::Parameter:
      state.out << "PARAM";
      break;
   }

   state.out
      << cf.expWord0.dstReg;

   state.out
      << ", R"
      << cf.expWord0.srcReg
      << ".";

   writeSelectName(state, cf.expWord1.srcSelX);
   writeSelectName(state, cf.expWord1.srcSelY);
   writeSelectName(state, cf.expWord1.srcSelZ);
   writeSelectName(state, cf.expWord1.srcSelW);

   endLine(state);
   return true;
}

bool disassembleALU(DisassembleState &state, latte::cf::inst id, latte::cf::Instruction &cf)
{
   uint64_t *slots = reinterpret_cast<uint64_t *>(state.words + (latte::WordsPerCF * cf.aluWord0.addr));

   state.out
      << latte::alu::name[cf.aluWord1.inst] << ": "
      << "ADDR(" << cf.aluWord0.addr << ") CNT(" << (cf.aluWord1.count + 1) << ")";

   increaseIndent(state);

   for (auto slot = 0u; slot <= cf.aluWord1.count; ) {
      static char unitName[] = { 'x', 'y', 'z', 'w', 't' };
      bool units[5] = { false, false, false, false, false };
      bool last = false;
      uint32_t *literalPtr = reinterpret_cast<uint32_t*>(slots + slot);
      auto literals = 0u;

      endLine(state);

      for (auto i = 0u; i < 5 && !last; ++i) {
         auto alu = *reinterpret_cast<latte::alu::Instruction*>(slots + slot + i);
         literalPtr += 2;
         last = !!alu.word0.last;
      }

      last = false;

      for (auto i = 0u; i < 5 && !last; ++i) {
         auto alu = *reinterpret_cast<latte::alu::Instruction*>(slots + slot);
         auto unit = getUnit(units, alu);
         const char *name = nullptr;
         bool abs0 = false, abs1 = false;
         auto &opcode = latte::alu::op2info[alu.op2.inst];

         if (alu.word1.encoding == latte::alu::Encoding::OP2) {
            opcode = latte::alu::op2info[alu.op2.inst];
            name = opcode.name;
            abs0 = !!alu.op2.src0Abs;
            abs1 = !!alu.op2.src1Abs;
         } else {
            opcode = latte::alu::op3info[alu.op3.inst];
            name = opcode.name;
         }

         beginLine(state);

         if (i == 0) {
            state.out << fmt::pad(state.group, groupCounterSize, '0');
         } else {
            state.out << std::string(groupCounterSize, ' ').c_str();
         }
         
         state.out
            << ' '
            << unitName[unit] << ": "
            << fmt::pad(name, instrNamePad, ' ');

         if (alu.word1.encoding == latte::alu::Encoding::OP2 && alu.op2.writeMask == 0) {
            state.out << "____";
         } else {
            writeAluSource(state, literalPtr, alu.word1.dstGpr, alu.word1.dstRel, alu.word1.dstChan, 0, false);
         }

         if (opcode.srcs > 0) {
            state.out << ", ";
            writeAluSource(state, literalPtr, alu.word0.src0Sel, alu.word0.src0Rel, alu.word0.src0Chan, alu.word0.src0Neg, abs0);
         }

         if (opcode.srcs > 1) {
            state.out << ", ";
            writeAluSource(state, literalPtr, alu.word0.src1Sel, alu.word0.src1Rel, alu.word0.src1Chan, alu.word0.src1Neg, abs1);
         }

         if (alu.word1.encoding == latte::alu::Encoding::OP2) {
            if (alu.op2.updateExecuteMask) {
               state.out << " UPDATE_EXECUTE_MASK";
            }

            if (alu.op2.updatePred) {
               state.out << " UPDATE_PRED";
            }

            switch (alu.op2.omod) {
            case latte::alu::OutputModifier::Divide2:
               state.out << " OMOD_D2";
               break;
            case latte::alu::OutputModifier::Multiply2:
               state.out << " OMOD_M2";
               break;
            case latte::alu::OutputModifier::Multiply4:
               state.out << " OMOD_M4";
               break;
            }
         } else {
            if (opcode.srcs > 2) {
               state.out << ", ";
               writeAluSource(state, literalPtr, alu.op3.src2Sel, alu.op3.src2Rel, alu.op3.src2Chan, alu.op3.src2Neg, false);
            }
         }

         switch (alu.word1.bankSwizzle) {
         case latte::alu::BankSwizzle::Vec021:
            state.out << " VEC_021";
            break;
         case latte::alu::BankSwizzle::Vec120:
            state.out << " VEC_120";
            break;
         case latte::alu::BankSwizzle::Vec102:
            state.out << " VEC_102";
            break;
         case latte::alu::BankSwizzle::Vec201:
            state.out << " VEC_201";
            break;
         case latte::alu::BankSwizzle::Vec210:
            state.out << " VEC_210";
            break;
         }

         if (alu.word1.clamp) {
            state.out << " CLAMP";
         }

         endLine(state);

         // Count number of literal used
         if (alu.word0.src0Sel == latte::alu::Source::SrcLiteral) {
            literals = std::max(literals, alu.word0.src0Chan + 1);
         }

         if (alu.word0.src1Sel == latte::alu::Source::SrcLiteral) {
            literals = std::max(literals, alu.word0.src1Chan + 1);
         }

         if ((alu.word1.encoding != latte::alu::Encoding::OP2) && (alu.op3.src2Sel == latte::alu::Source::SrcLiteral)) {
            literals = std::max(literals, alu.op3.src2Chan + 1);
         }

         // Increase slot id
         slot += 1;
         last = !!alu.word0.last;
      }

      if (literals) {
         slot += (literals + 1) >> 1;
      }

      state.group++;
   }

   decreaseIndent(state);
   return true;
}

bool disassembleTEX(DisassembleState &state, latte::cf::inst cfID, latte::cf::Instruction &cf)
{
   uint32_t *ptr = state.words + (latte::WordsPerCF * cf.word0.addr);

   state.out
      << latte::cf::name[cfID] << ": "
      << "ADDR(" << cf.word0.addr << ") CNT(" << (cf.word1.count + 1) << ")";

   endLine(state);
   increaseIndent(state);

   for (auto slot = 0u; slot <= cf.word1.count; ) {
      auto tex = *reinterpret_cast<latte::tex::Instruction*>(ptr);
      auto name = latte::tex::name[tex.word0.inst];
      auto id = tex.word0.inst;

      if (id == latte::tex::VTX_FETCH || id == latte::tex::VTX_SEMANTIC || id == latte::tex::GET_BUFFER_RESINFO) {
         assert(false);
         ptr += latte::WordsPerVTX;
      } else if (id == latte::tex::MEM) {
         assert(false);
         ptr += latte::WordsPerMEM;
      } else {
         beginLine(state);

         state.out
            << fmt::pad(state.group, groupCounterSize, '0')
            << ' '
            << name
            << ' ';

         // Write dst
         if (tex.word1.dstRel != 0) {
            // TODO: relative address
            state.out << "__UNK_REL(" << tex.word1.dstRel << ")__";
         }

         writeRegisterName(state, tex.word1.dstReg);
         state.out << '.';
         writeSelectName(state, tex.word1.dstSelX);
         writeSelectName(state, tex.word1.dstSelY);
         writeSelectName(state, tex.word1.dstSelZ);
         writeSelectName(state, tex.word1.dstSelW);

         // Write src
         state.out << ", ";

         if (tex.word0.srcRel != 0) {
            // TODO: relative address
            state.out << "__UNK_REL(" << tex.word0.srcRel << ")__";
         }

         writeRegisterName(state, tex.word0.srcReg);
         state.out << '.';
         writeSelectName(state, tex.word2.srcSelX);
         writeSelectName(state, tex.word2.srcSelY);
         writeSelectName(state, tex.word2.srcSelZ);
         writeSelectName(state, tex.word2.srcSelW);

         state.out
            << ", t" << tex.word0.resourceID
            << ", s" << tex.word2.samplerID;

         if (tex.word2.offsetX || tex.word2.offsetY || tex.word2.offsetZ) {
            state.out
               << " OFFSET("
               << tex.word2.offsetX
               << ", "
               << tex.word2.offsetY
               << ", "
               << tex.word2.offsetZ
               << ")";
         }

         if (tex.word1.lodBias) {
            state.out << " LOD_BIAS(" << tex.word1.lodBias << ")";
         }

         if (!tex.word1.coordTypeX) {
            state.out << " CTX_UNORM";
         }

         if (!tex.word1.coordTypeY) {
            state.out << " CTY_UNORM";
         }

         if (!tex.word1.coordTypeZ) {
            state.out << " CTZ_UNORM";
         }

         if (!tex.word1.coordTypeW) {
            state.out << " CTW_UNORM";
         }

         if (tex.word0.bcFracMode) {
            state.out << " BFM";
         }

         if (tex.word0.fetchWholeQuad) {
            state.out << " FWQ";
         }

         endLine(state);
         ptr += latte::WordsPerTEX;
      }

      state.group++;
      slot++;
   }

   decreaseIndent(state);
   return true;
}

} // namespace latte
