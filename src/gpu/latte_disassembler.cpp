#include "latte_disassembler.h"

static const uint32_t wordsPerCF = 2;
static const uint32_t wordsPerALU = 2;
static const uint32_t wordsPerTEX = 4;
static const uint32_t wordsPerMEM = 4; // TODO: Verify
static const uint32_t wordsPerVTX = 4; // TODO: Verify
static const uint32_t indentSize = 2;
static const uint32_t instrNamePad = 16;
static const uint32_t groupCounterSize = 2;

namespace latte
{

void Disassembler::increaseIndent()
{
   mIndent.append(indentSize, ' ');
}

void Disassembler::decreaseIndent()
{
   if (mIndent.size() >= indentSize) {
      mIndent.resize(mIndent.size() - indentSize);
   }
}

bool Disassembler::disassemble(uint8_t *binary, uint32_t size)
{
   fmt::MemoryWriter out;
   bool result = true;

   assert((size % 4) == 0);
   mWords = reinterpret_cast<uint32_t*>(binary);
   mWordCount = size / 4;

   mControlFlowCounter = 0;
   mGroupCounter = 0;

   for (auto i = 0u; i < mWordCount; i += 2) {
      auto cf = *reinterpret_cast<latte::cf::Instruction*>(mWords + i);
      auto id = static_cast<latte::cf::inst>(cf.word1.inst);

      out << mIndent.c_str() << fmt::pad(mControlFlowCounter, 2, '0') << ' ';

      switch (cf.type) {
      case latte::cf::Type::Normal:
         result &= cfNormal(out, id, cf);
         break;
      case latte::cf::Type::Export:
         if (id == latte::exp::EXP || id == latte::exp::EXP_DONE) {
            result &= cfExport(out, id, cf);
         } else {
            assert(false);
         }
         break;
      case latte::cf::Type::Alu:
      case latte::cf::Type::AluExtended:
         result &= cfALU(out, id, cf);
         break;
      }

      out << "\n";
      mControlFlowCounter++;

      if ((cf.type == latte::cf::Type::Normal || cf.type == latte::cf::Type::Export) && cf.word1.endOfProgram) {
         out << "END_OF_PROGRAM\n";
         break;
      }
   }

   std::cout << out.str() << std::endl;
   return result;
}

bool Disassembler::cfNormal(fmt::MemoryWriter &out, latte::cf::inst id, latte::cf::Instruction &cf)
{
   auto name = latte::cf::name[id];

   switch (id) {
   case latte::cf::NOP:
      return true;
   case latte::cf::TEX:
      return cfTEX(out, id, cf);
   case latte::cf::VTX:
   case latte::cf::VTX_TC:
      // Vtx
      break;
   case latte::cf::LOOP_START:
   case latte::cf::LOOP_START_DX10:
   case latte::cf::LOOP_START_NO_AL:
      out
         << name
         << " FAIL_JUMP_ADDR(" << cf.word0.addr << ")"
         << "\n";
      increaseIndent();
      break;
   case latte::cf::LOOP_END:
      out
         << name
         << " PASS_JUMP_ADDR(" << cf.word0.addr << ")"
         << "\n";
      decreaseIndent();
      break;
   case latte::cf::JUMP:
      out
         << name
         << " POP_CNT(" << cf.word1.popCount << ")"
         << " ADDR(" << cf.word0.addr << ")"
         << "\n";
      break;
   case latte::cf::END_PROGRAM:
      out
         << name
         << "\n";
      break;
   case latte::cf::CALL_FS:
      out
         << name
         << "\n";
      break;
   case latte::cf::LOOP_CONTINUE:
   case latte::cf::POP_JUMP:
   case latte::cf::ELSE:
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

bool Disassembler::cfExport(fmt::MemoryWriter &out, latte::cf::inst id, latte::cf::Instruction &cf)
{
   return true;
}

static bool
isTranscendentalOnly(latte::alu::Instruction &alu)
{
   latte::alu::Opcode opcode;

   if (alu.word1.encoding == latte::alu::Encoding::OP2) {
      opcode = latte::alu::op2[alu.op2.inst];
   } else {
      opcode = latte::alu::op3[alu.op3.inst];
   }

   if (opcode.units & latte::alu::Opcode::Vector) {
      return false;
   }

   if (opcode.units & latte::alu::Opcode::Transcendental) {
      return true;
   }

   return false;
}

static bool
isVectorOnly(latte::alu::Instruction &alu)
{
   latte::alu::Opcode opcode;

   if (alu.word1.encoding == latte::alu::Encoding::OP2) {
      opcode = latte::alu::op2[alu.op2.inst];
   } else {
      opcode = latte::alu::op3[alu.op3.inst];
   }

   if (opcode.units & latte::alu::Opcode::Transcendental) {
      return false;
   }

   if (opcode.units & latte::alu::Opcode::Vector) {
      return true;
   }

   return false;
}

static uint32_t
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

static void
writeRegisterName(fmt::MemoryWriter &out, uint32_t sel)
{
   if (sel > latte::NumGPR - latte::NumTempRegisters) {
      out << "T" << (latte::NumGPR - sel - 1);
   } else {
      out << "R" << sel;
   }
}

static void
writeAluSource(fmt::MemoryWriter &out, uint32_t *dwBase, uint32_t counter, uint32_t sel, uint32_t rel, uint32_t chan, uint32_t neg, bool abs)
{
   if (rel != 0) {
      // TODO: relative address
      out << "__UNK_REL(" << rel << ")__";
   }

   if (abs) {
      out << "ABS(";
   }

   // Negate
   if (neg) {
      out << "-";
   }

   // Sel
   if (sel >= latte::alu::Source::RegisterFirst && sel <= latte::alu::Source::RegisterLast) {
      writeRegisterName(out, sel);
   } else if (sel >= latte::alu::Source::KcacheBank0First && sel <= latte::alu::Source::KcacheBank0Last) {
      out << "KCACHEBANK0_" << (sel - latte::alu::Source::KcacheBank0First);
   } else if (sel >= latte::alu::Source::KcacheBank1First && sel <= latte::alu::Source::KcacheBank1Last) {
      out << "KCACHEBANK1_" << (sel - latte::alu::Source::KcacheBank1First);
   } else if (sel == latte::alu::Source::Src1DoubleLSW) {
      out << "__UNK_Src1DoubleLSW__";
   } else if (sel == latte::alu::Source::Src1DoubleMSW) {
      out << "__UNK_Src1DoubleMSW__";
   } else if (sel == latte::alu::Source::Src05DoubleLSW) {
      out << "__UNK_Src05DoubleLSW__";
   } else if (sel == latte::alu::Source::Src05DoubleMSW) {
      out << "__UNK_Src05DoubleMSW__";
   } else if (sel == latte::alu::Source::Src0Float) {
      out << "0.0f";
   } else if (sel == latte::alu::Source::Src1Float) {
      out << "1.0f";
   } else if (sel == latte::alu::Source::Src1Integer) {
      out << "1";
   } else if (sel == latte::alu::Source::SrcMinus1Integer) {
      if (neg) {
         out << "1";
      } else {
         out << "-1";
      }
   } else if (sel == latte::alu::Source::Src05Float) {
      out << "0.5f";
   } else if (sel == latte::alu::Source::SrcLiteral) {
      out << *reinterpret_cast<float*>(dwBase + chan) << "f";
   } else if (sel == latte::alu::Source::SrcPreviousScalar) {
      out << "PS" << (counter - 1);
   } else if (sel == latte::alu::Source::SrcPreviousVector) {
      out << "PV" << (counter - 1);
   } else if (sel >= latte::alu::Source::CfileConstantsFirst && sel <= latte::alu::Source::CfileConstantsLast) {
      out << "C" << (sel - latte::alu::Source::CfileConstantsFirst);
   } else {
      assert(false);
   }

   // Chan
   if (chan == latte::alu::Channel::X) {
      out << ".x";
   } else if (chan == latte::alu::Channel::Y) {
      out << ".y";
   } else if (chan == latte::alu::Channel::Z) {
      out << ".z";
   } else if (chan == latte::alu::Channel::W) {
      out << ".w";
   }

   if (abs) {
      out << ")";
   }
}

bool Disassembler::cfALU(fmt::MemoryWriter &out, latte::cf::inst id, latte::cf::Instruction &cf)
{
   uint64_t *slots = reinterpret_cast<uint64_t *>(mWords + (wordsPerCF * cf.aluWord0.addr));

   out
      << latte::alu::name[cf.aluWord1.inst] << ": "
      << "ADDR(" << cf.aluWord0.addr << ") CNT(" << (cf.aluWord1.count + 1) << ")";

   increaseIndent();

   for (auto slot = 0u; slot <= cf.aluWord1.count; ) {
      static char unitName[] = { 'x', 'y', 'z', 'w', 't' };
      bool units[5] = { false, false, false, false, false };
      bool last = false;
      uint32_t *literalPtr = reinterpret_cast<uint32_t*>(slots + slot);
      uint32_t literals = 0;

      out << "\n";

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

         if (alu.word1.encoding == latte::alu::Encoding::OP2) {
            auto &opcode = latte::alu::op2[alu.op2.inst];
            name = opcode.name;
            abs0 = !!alu.op2.src0Abs;
            abs1 = !!alu.op2.src1Abs;
         } else {
            auto &opcode = latte::alu::op3[alu.op3.inst];
            name = opcode.name;
         }

         out << mIndent.c_str();

         if (i == 0) {
            out << fmt::pad(mGroupCounter, groupCounterSize, '0');
         } else {
            out << std::string(groupCounterSize, ' ').c_str();
         }
         
         out
            << ' '
            << unitName[unit] << ": "
            << fmt::pad(name, instrNamePad, ' ');

         if (alu.word1.encoding == latte::alu::Encoding::OP2 && alu.op2.writeMask == 0) {
            out << "____";
         } else {
            writeAluSource(out, literalPtr, mGroupCounter, alu.word1.dstGpr, alu.word1.dstRel, alu.word1.dstChan, 0, false);
         }

         out << ", ";
         writeAluSource(out, literalPtr, mGroupCounter, alu.word0.src0Sel, alu.word0.src0Rel, alu.word0.src0Chan, alu.word0.src0Neg, abs0);
         out << ", ";
         writeAluSource(out, literalPtr, mGroupCounter, alu.word0.src1Sel, alu.word0.src1Rel, alu.word0.src1Chan, alu.word0.src1Neg, abs1);

         if (alu.word1.encoding == latte::alu::Encoding::OP2) {
            if (alu.op2.updateExecuteMask) {
               out << " UPDATE_EXECUTE_MASK";
            }

            if (alu.op2.updatePred) {
               out << " UPDATE_PRED";
            }

            switch (alu.op2.omod) {
            case latte::alu::OutputModifier::Divide2:
               out << " OMOD_D2";
               break;
            case latte::alu::OutputModifier::Multiply2:
               out << " OMOD_M2";
               break;
            case latte::alu::OutputModifier::Multiply4:
               out << " OMOD_M4";
               break;
            }
         } else {
            out << ", ";
            writeAluSource(out, literalPtr, mGroupCounter, alu.op3.src2Sel, alu.op3.src2Rel, alu.op3.src2Chan, alu.op3.src2Neg, false);
         }

         switch (alu.word1.bankSwizzle) {
         case latte::alu::BankSwizzle::Vec021:
            out << " VEC_021";
            break;
         case latte::alu::BankSwizzle::Vec120:
            out << " VEC_120";
            break;
         case latte::alu::BankSwizzle::Vec102:
            out << " VEC_102";
            break;
         case latte::alu::BankSwizzle::Vec201:
            out << " VEC_201";
            break;
         case latte::alu::BankSwizzle::Vec210:
            out << " VEC_210";
            break;
         }

         if (alu.word1.clamp) {
            out << " CLAMP";
         }

         out << '\n';

         // Count number of literal used
         literals += (alu.word0.src0Sel == latte::alu::Source::SrcLiteral) ? 1 : 0;
         literals += (alu.word0.src1Sel == latte::alu::Source::SrcLiteral) ? 1 : 0;
         literals += ((alu.word1.encoding != latte::alu::Encoding::OP2) && (alu.op3.src2Sel == latte::alu::Source::SrcLiteral)) ? 1 : 0;

         // Increase slot id
         slot += 1;
         last = !!alu.word0.last;
      }

      if (literals) {
         slot += (literals + 1) >> 1;
      }
      mGroupCounter++;
   }

   decreaseIndent();
   return true;
}

static void
writeSelectName(fmt::MemoryWriter &out, uint32_t select)
{
   switch (select) {
   case latte::alu::Select::X:
      out << 'x';
      break;
   case latte::alu::Select::Y:
      out << 'y';
      break;
   case latte::alu::Select::Z:
      out << 'z';
      break;
   case latte::alu::Select::W:
      out << 'w';
      break;
   case latte::alu::Select::One:
      out << '1';
      break;
   case latte::alu::Select::Zero:
      out << '0';
      break;
   case latte::alu::Select::Mask:
      out << '_';
      break;
   default:
      assert(false);
      out << '?';
   }
}

bool Disassembler::cfTEX(fmt::MemoryWriter &out, latte::cf::inst cfID, latte::cf::Instruction &cf)
{
   uint32_t *ptr = mWords + (wordsPerCF * cf.word0.addr);

   out
      << latte::cf::name[cfID] << ": "
      << "ADDR(" << cf.word0.addr << ") CNT(" << (cf.word1.count + 1) << ")\n";

   increaseIndent();

   for (auto slot = 0u; slot <= cf.word1.count; ) {
      auto tex = *reinterpret_cast<latte::tex::Instruction*>(ptr);
      auto name = latte::tex::name[tex.word0.inst];
      auto id = tex.word0.inst;

      if (id == latte::tex::VTX_FETCH || id == latte::tex::VTX_SEMANTIC || id == latte::tex::GET_BUFFER_RESINFO) {
         assert(false);
         ptr += wordsPerVTX;
      } else if (id == latte::tex::MEM) {
         assert(false);
         ptr += wordsPerMEM;
      } else {
         out
            << mIndent.c_str()
            << fmt::pad(mGroupCounter, groupCounterSize, '0')
            << ' '
            << name
            << ' ';

         // Write dst
         if (tex.word1.dstRel != 0) {
            // TODO: relative address
            out << "__UNK_REL(" << tex.word1.dstRel << ")__";
         }

         writeRegisterName(out, tex.word1.dstReg);
         out << '.';
         writeSelectName(out, tex.word1.dstSelX);
         writeSelectName(out, tex.word1.dstSelY);
         writeSelectName(out, tex.word1.dstSelZ);
         writeSelectName(out, tex.word1.dstSelW);

         // Write src
         out << ", ";

         if (tex.word0.srcRel != 0) {
            // TODO: relative address
            out << "__UNK_REL(" << tex.word0.srcRel << ")__";
         }

         writeRegisterName(out, tex.word0.srcReg);
         out << '.';
         writeSelectName(out, tex.word2.srcSelX);
         writeSelectName(out, tex.word2.srcSelY);
         writeSelectName(out, tex.word2.srcSelZ);
         writeSelectName(out, tex.word2.srcSelW);

         out
            << ", t" << tex.word0.resourceID
            << ", s" << tex.word2.samplerID;

         if (tex.word2.offsetX || tex.word2.offsetY || tex.word2.offsetZ) {
            out
               << " OFFSET("
               << tex.word2.offsetX
               << ", "
               << tex.word2.offsetY
               << ", "
               << tex.word2.offsetZ
               << ")";
         }

         if (tex.word1.lodBias) {
            out << " LOD_BIAS(" << tex.word1.lodBias << ")";
         }

         if (!tex.word1.coordTypeX) {
            out << " CTX_UNORM";
         }

         if (!tex.word1.coordTypeY) {
            out << " CTY_UNORM";
         }

         if (!tex.word1.coordTypeZ) {
            out << " CTZ_UNORM";
         }

         if (!tex.word1.coordTypeW) {
            out << " CTW_UNORM";
         }

         if (tex.word0.bcFracMode) {
            out << " BFM";
         }

         if (tex.word0.fetchWholeQuad) {
            out << " FWQ";
         }

         out << '\n';
         ptr += wordsPerTEX;
      }

      mGroupCounter++;
      slot++;
   }

   decreaseIndent();
   return true;
}

} // namespace latte
