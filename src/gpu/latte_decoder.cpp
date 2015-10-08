#include <algorithm>
#include <cassert>
#include <vector>
#include "bitutils.h"
#include "latte.h"

namespace latte
{

struct DecodeState
{
   const uint32_t *words;
   std::size_t wordCount;
   uint32_t cfPC;
   uint32_t group;
   Shader *shader;
};

static bool decodeNormal(DecodeState &state, latte::cf::inst id, latte::cf::Instruction &cf);
static bool decodeExport(DecodeState &state, latte::cf::inst id, latte::cf::Instruction &cf);
static bool decodeALU(DecodeState &state, latte::cf::inst id, latte::cf::Instruction &cf);
static bool decodeTEX(DecodeState &state, latte::cf::inst id, latte::cf::Instruction &cf);

bool
decode(Shader &shader, const gsl::array_view<uint8_t> &binary)
{
   DecodeState state;

   assert((binary.size() % 4) == 0);
   state.words = reinterpret_cast<const uint32_t*>(binary.data());
   state.wordCount = binary.size() / 4;
   state.cfPC = 0;
   state.group = 0;
   state.shader = &shader;

   // Step 1: Deserialise, inline ALU/TEX clauses
   for (auto i = 0u; i < state.wordCount; i += 2) {
      auto cf = *reinterpret_cast<const latte::cf::Instruction*>(state.words + i);
      auto id = static_cast<latte::cf::inst>(cf.word1.inst);

      switch (cf.type) {
      case latte::cf::Type::Normal:
         decodeNormal(state, id, cf);
         break;
      case latte::cf::Type::Export:
         if (id == latte::exp::EXP || id == latte::exp::EXP_DONE) {
            decodeExport(state, id, cf);
         } else {
            assert(false);
         }
         break;
      case latte::cf::Type::Alu:
      case latte::cf::Type::AluExtended:
         decodeALU(state, id, cf);
         break;
      }

      state.cfPC++;

      if ((cf.type == latte::cf::Type::Normal || cf.type == latte::cf::Type::Export) && cf.word1.endOfProgram) {
         break;
      }
   }

   return true;
}

static bool
decodeNormal(DecodeState &state, latte::cf::inst id, latte::cf::Instruction &cf)
{
   shadir::CfInstruction *ins = nullptr;

   switch (id) {
   case latte::cf::NOP:
   case latte::cf::CALL_FS:
   case latte::cf::END_PROGRAM:
      return true;
   case latte::cf::TEX:
      return decodeTEX(state, id, cf);
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
      ins->cfPC = state.cfPC;
      ins->id = id;
      ins->addr = cf.word0.addr;
      ins->popCount = cf.word1.popCount;
      ins->loopCfConstant = cf.word1.cfConst;
      ins->cond = static_cast<latte::cf::Cond::Cond>(cf.word1.cond);
      state.shader->code.emplace_back(ins);
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

static bool
decodeExport(DecodeState &state, latte::cf::inst id, latte::cf::Instruction &cf)
{
   auto exp = new latte::shadir::ExportInstruction {};
   exp->id = static_cast<latte::exp::inst>(cf.expWord1.inst);
   exp->name = latte::exp::name[exp->id];
   exp->cfPC = state.cfPC;

   exp->dstReg = cf.expWord0.dstReg;
   exp->src.id = cf.expWord0.srcReg;
   exp->src.selX = static_cast<latte::alu::Select::Select>(cf.expWord1.srcSelX);
   exp->src.selY = static_cast<latte::alu::Select::Select>(cf.expWord1.srcSelY);
   exp->src.selZ = static_cast<latte::alu::Select::Select>(cf.expWord1.srcSelZ);
   exp->src.selW = static_cast<latte::alu::Select::Select>(cf.expWord1.srcSelW);

   exp->type = static_cast<latte::exp::Type::Type>(cf.expWord0.type);
   exp->elemSize = cf.expWord0.elemSize;
   exp->indexGpr = cf.expWord0.indexGpr;
   exp->wholeQuadMode = cf.expWord1.wholeQuadMode;
   exp->barrier = !!cf.expWord1.barrier;

   assert(cf.expWord0.srcRel == 0);
   assert(cf.expWord1.validPixelMode == 0);
   state.shader->exports.push_back(exp);
   state.shader->code.emplace_back(exp);
   return true;
}

static void
getAluSource(shadir::AluSource &source, const uint32_t *dwBase, uint32_t counter, uint32_t sel, uint32_t rel, uint32_t chan, uint32_t neg, bool abs)
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
      source.floatValue = *reinterpret_cast<const float*>(dwBase + chan);
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


static bool
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

static bool
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

static bool
decodeALU(DecodeState &state, latte::cf::inst id, latte::cf::Instruction &cf)
{
   const uint64_t *slots = reinterpret_cast<const uint64_t *>(state.words + (latte::WordsPerCF * cf.aluWord0.addr));
   auto aluID = static_cast<latte::alu::inst>(cf.aluWord1.inst);
   bool hasPushedBefore = false;

   switch (aluID) {
   case latte::alu::inst::ALU_BREAK: // Break starts with a push!
   case latte::alu::inst::ALU_CONTINUE: // Continue starts with a push!
   {
      auto push = new shadir::CfInstruction();
      push->id = latte::cf::inst::PUSH;
      push->name = latte::cf::name[push->id];
      push->cfPC = state.cfPC;
      state.shader->code.emplace_back(push);
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
      const uint32_t *literalPtr = reinterpret_cast<const uint32_t*>(slots + slot);
      auto literals = 0u;
      shadir::AluReductionInstruction *reductionIns = nullptr;

      for (auto i = 0u; i < 5 && !last; ++i) {
         auto alu = *reinterpret_cast<const latte::alu::Instruction*>(slots + slot + i);
         literalPtr += 2;
         last = !!alu.word0.last;
      }

      last = false;

      for (auto i = 0u; i < 5 && !last; ++i) {
         auto alu = *reinterpret_cast<const latte::alu::Instruction*>(slots + slot);
         auto &opcode = latte::alu::op2info[alu.op2.inst];
         auto unit = getUnit(units, alu);

         if (alu.word1.encoding == latte::alu::Encoding::OP2 && opcode.id == latte::alu::op2::NOP) {
            slot += 1;
            last = !!alu.word0.last;
            continue;
         }

         auto ins = new shadir::AluInstruction();
         bool abs0 = false, abs1 = false;

         ins->cfPC = state.cfPC;
         ins->groupPC = state.group;
         ins->predSel = static_cast<latte::alu::PredicateSelect::PredicateSelect>(alu.word0.predSel);

         assert(alu.word1.dstRel == 0);
         ins->dest.clamp = alu.word1.clamp;
         ins->dest.id = alu.word1.dstGpr;
         ins->dest.chan = static_cast<latte::alu::Channel::Channel>(alu.word1.dstChan);
         ins->unit = static_cast<shadir::AluInstruction::Unit>(unit);

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
            getAluSource(ins->sources[2], literalPtr, state.group, alu.op3.src2Sel, alu.op3.src2Rel, alu.op3.src2Chan, alu.op3.src2Neg, false);
         }

         getAluSource(ins->sources[0], literalPtr, state.group, alu.word0.src0Sel, alu.word0.src0Rel, alu.word0.src0Chan, alu.word0.src0Neg, abs0);
         getAluSource(ins->sources[1], literalPtr, state.group, alu.word0.src1Sel, alu.word0.src1Rel, alu.word0.src1Chan, alu.word0.src1Neg, abs1);

         // Check whether to set sources to Int or Uint
         if (opcode.flags & latte::alu::Opcode::IntIn) {
            for (auto j = 0u; j < ins->numSources; ++j) {
               ins->sources[j].valueType = shadir::AluSource::Int;
            }
         } else if (opcode.flags & latte::alu::Opcode::UintIn) {
            for (auto j = 0u; j < ins->numSources; ++j) {
               ins->sources[j].valueType = shadir::AluSource::Uint;
            }
         }

         // Check whether to set dest to Int or Uint
         if (opcode.flags & latte::alu::Opcode::IntOut) {
            ins->dest.valueType = shadir::AluDest::Int;
         } else if (opcode.flags & latte::alu::Opcode::UintOut) {
            ins->dest.valueType = shadir::AluDest::Uint;
         }

         // Special ALU ops before
         if (opcode.flags & latte::alu::Opcode::PredSet) {
            switch (aluID) {
            case latte::alu::inst::ALU_PUSH_BEFORE: // PushBefore only push before first PRED_SET
            {
               if (!hasPushedBefore) {
                  auto push = new shadir::CfInstruction();
                  push->id = latte::cf::inst::PUSH;
                  push->name = latte::cf::name[push->id];
                  push->cfPC = state.cfPC;
                  state.shader->code.emplace_back(push);
                  hasPushedBefore = true;
               }
            }
            break;
            case latte::alu::inst::ALU_ELSE_AFTER: // ElseAfter push before every PRED_SET
            {
               auto push = new shadir::CfInstruction();
               push->id = latte::cf::inst::PUSH;
               push->name = latte::cf::name[push->id];
               push->cfPC = state.cfPC;
               state.shader->code.emplace_back(push);
            }
            break;
            }
         }

         // Check if we need to create a new reduction instruction
         if (!reductionIns) {
            if (ins->opType == shadir::AluInstruction::OP2) {
               if (ins->op2 == latte::alu::op2::DOT4
                   || ins->op2 == latte::alu::op2::DOT4_IEEE
                   || ins->op2 == latte::alu::op2::CUBE
                   || ins->op2 == latte::alu::op2::MAX4) {
                  reductionIns = new latte::shadir::AluReductionInstruction();
                  reductionIns->op2 = ins->op2;
                  reductionIns->name = ins->name;
                  reductionIns->cfPC = ins->cfPC;
                  reductionIns->groupPC = ins->groupPC;
                  state.shader->code.emplace_back(reductionIns);
               }
            }
         }

         if (reductionIns && unit < 4) {
            // Add XYZW to reduction
            assert(!reductionIns->units[unit]);
            reductionIns->units[unit] = std::unique_ptr<shadir::AluInstruction>(ins);
         } else {
            // Append to shader code
            state.shader->code.emplace_back(ins);
         }

         // Special ALU ops after
         if (opcode.flags & latte::alu::Opcode::PredSet) {
            switch (aluID) {
            case latte::alu::inst::ALU_BREAK: // Break after every PRED_SET
            {
               auto loopBreak = new shadir::CfInstruction();
               loopBreak->cfPC = state.cfPC;
               loopBreak->id = latte::cf::inst::LOOP_BREAK;
               loopBreak->name = latte::cf::name[loopBreak->id];
               loopBreak->popCount = 1;
               state.shader->code.emplace_back(loopBreak);
            }
            break;
            case latte::alu::inst::ALU_CONTINUE: // Continue after every PRED_SET
            {
               auto loopContinue = new shadir::CfInstruction();
               loopContinue->cfPC = state.cfPC;
               loopContinue->id = latte::cf::inst::LOOP_CONTINUE;
               loopContinue->name = latte::cf::name[loopContinue->id];
               loopContinue->popCount = 1;
               state.shader->code.emplace_back(loopContinue);
            }
            break;
            case latte::alu::inst::ALU_ELSE_AFTER: // Else after every PRED_SET
            {
               auto els = new shadir::CfInstruction();
               els->cfPC = state.cfPC;
               els->id = latte::cf::inst::ELSE;
               els->name = latte::cf::name[els->id];
               els->popCount = 1;
               state.shader->code.emplace_back(els);
            }
            break;
            }
         }

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

   switch (aluID) {
   case latte::alu::inst::ALU_BREAK: // Break ends with POP
   case latte::alu::inst::ALU_CONTINUE: // Continue ends with POP
   case latte::alu::inst::ALU_POP_AFTER: // Pop after ALU clause
   {
      auto pop = new shadir::CfInstruction();
      pop->cfPC = state.cfPC;
      pop->id = latte::cf::inst::POP;
      pop->name = latte::cf::name[pop->id];
      pop->popCount = 1;
      state.shader->code.emplace_back(pop);
   }
   break;
   case latte::alu::inst::ALU_POP2_AFTER: // Pop2 after ALU clause
   {
      auto pop2 = new shadir::CfInstruction();
      pop2->cfPC = state.cfPC;
      pop2->id = latte::cf::inst::POP;
      pop2->name = latte::cf::name[pop2->id];
      pop2->popCount = 2;
      state.shader->code.emplace_back(pop2);
   }
   break;
   }

   return true;
}

static bool
decodeTEX(DecodeState &state, latte::cf::inst id, latte::cf::Instruction &cf)
{
   const uint32_t *ptr = state.words + (latte::WordsPerCF * cf.word0.addr);

   for (auto slot = 0u; slot <= cf.word1.count; ) {
      auto tex = *reinterpret_cast<const latte::tex::Instruction*>(ptr);
      auto id = static_cast<const latte::tex::inst>(tex.word0.inst);

      if (id == latte::tex::VTX_FETCH || id == latte::tex::VTX_SEMANTIC || id == latte::tex::GET_BUFFER_RESINFO) {
         assert(false);
         ptr += latte::WordsPerVTX;
      } else if (id == latte::tex::MEM) {
         assert(false);
         ptr += latte::WordsPerMEM;
      } else {
         auto ins = new shadir::TexInstruction;
         ins->name = latte::tex::name[id];
         ins->cfPC = state.cfPC;
         ins->groupPC = state.group;

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

         state.shader->code.emplace_back(ins);
         ptr += latte::WordsPerTEX;
      }

      state.group++;
      slot++;
   }

   return true;
}

} // namespace latte
