#include "latte_parser.h"
#include "bitutils.h"
#include <cassert>
#include <set>

static const uint32_t wordsPerCF = 2; // 1 64bit slot
static const uint32_t wordsPerALU = 2; // 1 64bit slot
static const uint32_t wordsPerTEX = 4; // 2 64bit slots (only uses 64+32bits of data though)
static const uint32_t wordsPerMEM = 4; // TODO: Verify
static const uint32_t wordsPerVTX = 4; // TODO: Verify
static const uint32_t indentSize = 2;
static const uint32_t instrNamePad = 16;
static const uint32_t groupCounterSize = 2;

namespace latte
{

bool Parser::parse(shadir::Shader &shader, uint8_t *binary, uint32_t size)
{
   bool result = true;

   assert((size % 4) == 0);
   mWords = reinterpret_cast<uint32_t*>(binary);
   mWordCount = size / 4;
   mShader = &shader;

   mControlFlowCounter = 0;
   mGroupCounter = 0;

   // Step 1: Deserialise
   std::vector<shadir::Instruction *> deserialised;
   deserialise(deserialised);

   // Step 2: Analyse deserialised for control flow statements, like if/while

   // For IF:
      // PUSH
      // PRED_SET
      // JUMP -> false
      // -> true

   // For WHILE:
      // LOOP_BEGIN / LOOP_DX10
      // LOOP_END

      // PUSH
      // PRED_SET *n
      // BREAK *n
      // POP

      // PUSH
      // PRED_SET *n
      // CONTINUE *n
      // POP

   // Step 3: Find incoming lives for attribs/varying/uniform

   // Step 4: Generate code

   return result;
}

bool Parser::deserialise(std::vector<shadir::Instruction *> &deserialised)
{
   // Step 1: Deserialise, inline ALU/TEX clauses
   for (auto i = 0u; i < mWordCount; i += 2) {
      auto cf = *reinterpret_cast<latte::cf::Instruction*>(mWords + i);
      auto id = static_cast<latte::cf::inst>(cf.word1.inst);

      switch (cf.type) {
      case latte::cf::Type::Normal:
         deserialiseNormal(deserialised, id, cf);
         break;
      case latte::cf::Type::Export:
         if (id == latte::exp::EXP || id == latte::exp::EXP_DONE) {
            deserialiseExport(deserialised, id, cf);
         } else {
            assert(false);
         }
         break;
      case latte::cf::Type::Alu:
      case latte::cf::Type::AluExtended:
         deserialiseALU(deserialised, id, cf);
         break;
      }

      mControlFlowCounter++;

      if ((cf.type == latte::cf::Type::Normal || cf.type == latte::cf::Type::Export) && cf.word1.endOfProgram) {
         break;
      }
   }

   return true;
}

bool Parser::deserialiseNormal(std::vector<shadir::Instruction *> &deserialised, latte::cf::inst id, latte::cf::Instruction &cf)
{
   shadir::CfInstruction *ins = nullptr;

   switch (id) {
   case latte::cf::NOP:
   case latte::cf::CALL_FS:
   case latte::cf::END_PROGRAM:
      return true;
   case latte::cf::TEX:
      return deserialiseTEX(deserialised, id, cf);
   case latte::cf::LOOP_START:
   case latte::cf::LOOP_START_DX10:
   case latte::cf::LOOP_START_NO_AL:
   case latte::cf::LOOP_END:
   case latte::cf::JUMP:
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
      ins = new shadir::CfInstruction;
      ins->name = latte::cf::name[id];
      ins->cfPC = mControlFlowCounter;
      ins->id = id;
      ins->addr = cf.word0.addr;
      ins->popCount = cf.word1.popCount;
      ins->loopCfConstant = cf.word1.cfConst;
      ins->cond = static_cast<latte::cf::Cond::Cond>(cf.word1.cond);
      deserialised.push_back(ins);
      return true;
   case latte::cf::VTX:
   case latte::cf::VTX_TC:
   case latte::cf::WAIT_ACK:
   case latte::cf::TEX_ACK:
   case latte::cf::VTX_ACK:
   case latte::cf::VTX_TC_ACK:
   default:
      assert(false);
      return false;
   }
}

bool Parser::deserialiseExport(std::vector<shadir::Instruction *> &deserialised, latte::cf::inst id, latte::cf::Instruction &cf)
{
   return true;
}

static void
getAluSource(shadir::AluSource &source, uint32_t *dwBase, uint32_t counter, uint32_t sel, uint32_t rel, uint32_t chan, uint32_t neg, bool abs)
{
   assert(rel == 0);
   source.absolute = abs;
   source.negate = !!neg;
   source.chan = static_cast<latte::alu::Channel::Channel>(chan);

   if (sel >= latte::alu::Source::RegisterFirst && sel <= latte::alu::Source::RegisterLast) {
      source.type = shadir::AluSource::Register;
      source.id = sel - latte::alu::Source::RegisterFirst;
   } else if (sel >= latte::alu::Source::KcacheBank0First && sel <= latte::alu::Source::KcacheBank0Last) {
      source.type = shadir::AluSource::KcacheBank0;
      source.id = sel - latte::alu::Source::KcacheBank0First;
   } else if (sel >= latte::alu::Source::KcacheBank1First && sel <= latte::alu::Source::KcacheBank1Last) {
      source.type = shadir::AluSource::KcacheBank1;
      source.id = sel - latte::alu::Source::KcacheBank1First;
   } else if (sel == latte::alu::Source::Src1DoubleLSW) {
      assert(false);
   } else if (sel == latte::alu::Source::Src1DoubleMSW) {
      assert(false);
   } else if (sel == latte::alu::Source::Src05DoubleLSW) {
      assert(false);
   } else if (sel == latte::alu::Source::Src05DoubleMSW) {
      assert(false);
   } else if (sel == latte::alu::Source::Src0Float) {
      source.type = shadir::AluSource::ConstantFloat;
      source.floatValue = 0.0f;
   } else if (sel == latte::alu::Source::Src1Float) {
      source.type = shadir::AluSource::ConstantFloat;
      source.floatValue = 1.0f;
   } else if (sel == latte::alu::Source::Src1Integer) {
      source.type = shadir::AluSource::ConstantInt;
      source.floatValue = 1;
   } else if (sel == latte::alu::Source::SrcMinus1Integer) {
      source.type = shadir::AluSource::ConstantInt;
      source.floatValue = -1;
   } else if (sel == latte::alu::Source::Src05Float) {
      source.type = shadir::AluSource::ConstantFloat;
      source.floatValue = 0.5f;
   } else if (sel == latte::alu::Source::SrcLiteral) {
      source.type = shadir::AluSource::ConstantFloat;
      source.floatValue = *reinterpret_cast<float*>(dwBase + chan);
   } else if (sel == latte::alu::Source::SrcPreviousScalar) {
      source.type = shadir::AluSource::PreviousScalar;
      source.id = counter - 1;
   } else if (sel == latte::alu::Source::SrcPreviousVector) {
      source.type = shadir::AluSource::PreviousVector;
      source.id = counter - 1;
   } else if (sel >= latte::alu::Source::CfileConstantsFirst && sel <= latte::alu::Source::CfileConstantsLast) {
      source.type = shadir::AluSource::ConstantFile;
      source.id = sel - latte::alu::Source::CfileConstantsFirst;
   } else {
      assert(false);
   }
}

bool Parser::deserialiseALU(std::vector<shadir::Instruction *> &deserialised, latte::cf::inst id, latte::cf::Instruction &cf)
{
   uint64_t *slots = reinterpret_cast<uint64_t *>(mWords + (wordsPerCF * cf.aluWord0.addr));
   auto aluID = static_cast<latte::alu::inst>(cf.aluWord1.inst);
   bool hasPushedBefore = false;

   switch (aluID) {
   case latte::alu::inst::ALU_BREAK: // Break starts with a push!
   case latte::alu::inst::ALU_CONTINUE: // Continue starts with a push!
      {
         auto push = new shadir::CfInstruction();
         push->id = latte::cf::inst::PUSH;
         push->name = latte::cf::name[push->id];
         deserialised.push_back(push);
      }
      break;
   case latte::alu::inst::ALU_EXT:
      assert(false);
      break;
   }

   for (auto slot = 0u; slot <= cf.aluWord1.count; ) {
      static char unitName[] = { 'x', 'y', 'z', 'w', 't' };
      bool units[5] = { false, false, false, false, false };
      bool last = false;
      uint32_t *literalPtr = reinterpret_cast<uint32_t*>(slots + slot);
      uint32_t literals = 0;

      for (auto i = 0u; i < 5 && !last; ++i) {
         auto alu = *reinterpret_cast<latte::alu::Instruction*>(slots + slot + i);
         literalPtr += 2;
         last = !!alu.word0.last;
      }

      last = false;

      for (auto i = 0u; i < 5 && !last; ++i) {
         auto alu = *reinterpret_cast<latte::alu::Instruction*>(slots + slot);
         auto &opcode = latte::alu::op2info[alu.op2.inst];

         if (alu.word1.encoding == latte::alu::Encoding::OP2 && opcode.id == latte::alu::op2::NOP) {
            slot += 1;
            last = !!alu.word0.last;
            continue;
         }

         auto ins = new shadir::AluInstruction();
         bool abs0 = false, abs1 = false;

         ins->cfPC = mControlFlowCounter;
         ins->groupPC = mGroupCounter;
         ins->predSel = static_cast<latte::alu::PredicateSelect::PredicateSelect>(alu.word0.predSel);

         assert(alu.word1.dstRel == 0);
         ins->dest.clamp = alu.word1.clamp;
         ins->dest.id = alu.word1.dstGpr;
         ins->dest.chan = static_cast<latte::alu::Channel::Channel>(alu.word1.dstChan);

         if (alu.word1.encoding == latte::alu::Encoding::OP2) {
            ins->name = opcode.name;
            ins->opType = shadir::AluInstruction::OP2;
            ins->op2 = static_cast<latte::alu::op2>(opcode.id);
            ins->numSources = opcode.srcs;
            ins->updateExecutionMask = alu.op2.updateExecuteMask;
            ins->updatePredicate = alu.op2.updatePred;
            ins->writeMask = alu.op2.writeMask;
            ins->outputModifier = static_cast<latte::alu::OutputModifier::OutputModifier>(alu.op2.omod);
            abs0 = !!alu.op2.src0Abs;
            abs1 = !!alu.op2.src1Abs;
         } else {
            opcode = latte::alu::op3info[alu.op3.inst];
            ins->name = opcode.name;
            ins->opType = shadir::AluInstruction::OP3;
            ins->op3 = static_cast<latte::alu::op3>(opcode.id);
            ins->numSources = opcode.srcs;
            getAluSource(ins->sources[2], literalPtr, mGroupCounter, alu.op3.src2Sel, alu.op3.src2Rel, alu.op3.src2Chan, alu.op3.src2Neg, false);
         }

         getAluSource(ins->sources[0], literalPtr, mGroupCounter, alu.word0.src0Sel, alu.word0.src0Rel, alu.word0.src0Chan, alu.word0.src0Neg, abs0);
         getAluSource(ins->sources[1], literalPtr, mGroupCounter, alu.word0.src1Sel, alu.word0.src1Rel, alu.word0.src1Chan, alu.word0.src1Neg, abs1);

         // Special ALU ops before
         if (opcode.flags & latte::alu::Opcode::PredSet) {
            switch (aluID) {
            case latte::alu::inst::ALU_PUSH_BEFORE: // PushBefore only push before first PRED_SET
               {
                  if (!hasPushedBefore) {
                     auto push = new shadir::CfInstruction();
                     push->id = latte::cf::inst::PUSH;
                     push->name = latte::cf::name[push->id];
                     deserialised.push_back(push);
                     hasPushedBefore = true;
                  }
               }
               break;
            case latte::alu::inst::ALU_ELSE_AFTER: // ElseAfter push before every PRED_SET
               {
                  auto push = new shadir::CfInstruction();
                  push->id = latte::cf::inst::PUSH;
                  push->name = latte::cf::name[push->id];
                  deserialised.push_back(push);
               }
               break;
            }
         }

         deserialised.push_back(ins);

         // Special ALU ops after
         if (opcode.flags & latte::alu::Opcode::PredSet) {
            switch (aluID) {
            case latte::alu::inst::ALU_BREAK: // Break after every PRED_SET
               {
                  auto loopBreak = new shadir::CfInstruction();
                  loopBreak->id = latte::cf::inst::LOOP_BREAK;
                  loopBreak->name = latte::cf::name[loopBreak->id];
                  loopBreak->popCount = 1;
                  deserialised.push_back(loopBreak);
               }
               break;
            case latte::alu::inst::ALU_CONTINUE: // Continue after every PRED_SET
               {
                  auto loopContinue = new shadir::CfInstruction();
                  loopContinue->id = latte::cf::inst::LOOP_CONTINUE;
                  loopContinue->name = latte::cf::name[loopContinue->id];
                  loopContinue->popCount = 1;
                  deserialised.push_back(loopContinue);
               }
               break;
            case latte::alu::inst::ALU_ELSE_AFTER: // Else after every PRED_SET
               {
                  auto els = new shadir::CfInstruction();
                  els->id = latte::cf::inst::ELSE;
                  els->name = latte::cf::name[els->id];
                  els->popCount = 1;
                  deserialised.push_back(els);
               }
               break;
            }
         }

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

   switch (aluID) {
   case latte::alu::inst::ALU_BREAK: // Break ends with POP
   case latte::alu::inst::ALU_CONTINUE: // Continue ends with POP
   case latte::alu::inst::ALU_POP_AFTER: // Pop after ALU clause
      {
         auto pop = new shadir::CfInstruction();
         pop->id = latte::cf::inst::POP;
         pop->name = latte::cf::name[pop->id];
         pop->popCount = 1;
         deserialised.push_back(pop);
      }
      break;
   case latte::alu::inst::ALU_POP2_AFTER: // Pop2 after ALU clause
      {
         auto pop2 = new shadir::CfInstruction();
         pop2->id = latte::cf::inst::POP;
         pop2->name = latte::cf::name[pop2->id];
         pop2->popCount = 2;
         deserialised.push_back(pop2);
      }
      break;
   }

   return true;
}

bool Parser::deserialiseTEX(std::vector<shadir::Instruction *> &deserialised, latte::cf::inst cfID, latte::cf::Instruction &cf)
{
   uint32_t *ptr = mWords + (wordsPerCF * cf.word0.addr);

   for (auto slot = 0u; slot <= cf.word1.count; ) {
      auto tex = *reinterpret_cast<latte::tex::Instruction*>(ptr);
      auto id = static_cast<latte::tex::inst>(tex.word0.inst);

      if (id == latte::tex::VTX_FETCH || id == latte::tex::VTX_SEMANTIC || id == latte::tex::GET_BUFFER_RESINFO) {
         assert(false);
         ptr += wordsPerVTX;
      } else if (id == latte::tex::MEM) {
         assert(false);
         ptr += wordsPerMEM;
      } else {
         auto ins = new shadir::TexInstruction;
         ins->name = latte::tex::name[id];
         ins->cfPC = mControlFlowCounter;
         ins->groupPC = mGroupCounter;

         ins->id = id;
         ins->bcFracMode = tex.word0.bcFracMode;
         ins->fetchWholeQuad = tex.word0.fetchWholeQuad;
         ins->resourceID = tex.word0.resourceID;
         ins->samplerID = tex.word2.samplerID;
         ins->lodBias = sign_extend<7>(tex.word1.lodBias);

         ins->offsetX = sign_extend<5>(tex.word2.offsetX);
         ins->offsetY = sign_extend<5>(tex.word2.offsetY);
         ins->offsetZ = sign_extend<5>(tex.word2.offsetZ);

         ins->coordNormaliseX = !!tex.word1.coordTypeX;
         ins->coordNormaliseY = !!tex.word1.coordTypeY;
         ins->coordNormaliseZ = !!tex.word1.coordTypeZ;
         ins->coordNormaliseW = !!tex.word1.coordTypeW;

         assert(tex.word0.srcRel == 0);
         ins->src.id = tex.word0.srcReg;
         ins->src.selX = static_cast<latte::alu::Select::Select>(tex.word2.srcSelX);
         ins->src.selY = static_cast<latte::alu::Select::Select>(tex.word2.srcSelY);
         ins->src.selZ = static_cast<latte::alu::Select::Select>(tex.word2.srcSelZ);
         ins->src.selW = static_cast<latte::alu::Select::Select>(tex.word2.srcSelW);

         assert(tex.word1.dstRel == 0);
         ins->dst.id = tex.word1.dstReg;
         ins->dst.selX = static_cast<latte::alu::Select::Select>(tex.word1.dstSelX);
         ins->dst.selY = static_cast<latte::alu::Select::Select>(tex.word1.dstSelY);
         ins->dst.selZ = static_cast<latte::alu::Select::Select>(tex.word1.dstSelZ);
         ins->dst.selW = static_cast<latte::alu::Select::Select>(tex.word1.dstSelW);

         deserialised.push_back(ins);
         ptr += wordsPerTEX;
      }

      mGroupCounter++;
      slot++;
   }

   return true;
}

} // namespace latte
