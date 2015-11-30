#include <algorithm>
#include <cassert>
#include <vector>
#include "latte.h"
#include "utils/bitutils.h"

namespace latte
{

struct DecodeState
{
   const uint32_t *words;
   std::size_t wordCount;
   int32_t cfPC;
   int32_t group;
   Shader *shader;
};

namespace KcacheMode
{
enum Mode {
   Nop = 0,
   Lock1 = 1,
   Lock2 = 2,
   LockLoopIdx = 3
};
}

static bool decodeNormal(DecodeState &state, cf::inst id, cf::Instruction &cf);
static bool decodeExport(DecodeState &state, cf::inst id, cf::Instruction &cf);
static bool decodeALU(DecodeState &state, cf::inst id, cf::Instruction &cf);
static bool decodeTEX(DecodeState &state, cf::inst id, cf::Instruction &cf);

bool
decode(Shader &shader, Shader::Type type, const gsl::span<uint8_t> &binary)
{
   DecodeState state;

   assert((binary.size() % 4) == 0);
   state.words = reinterpret_cast<const uint32_t*>(binary.data());
   state.wordCount = binary.size() / 4;
   state.cfPC = 0;
   state.group = 0;
   state.shader = &shader;
   shader.type = type;

   // Step 1: Deserialise, inline ALU/TEX clauses
   for (auto i = 0u; i < state.wordCount; i += 2) {
      auto cf = *reinterpret_cast<const cf::Instruction*>(state.words + i);
      auto id = static_cast<cf::inst>(cf.word1.inst);

      switch (cf.type) {
      case cf::Type::Normal:
         decodeNormal(state, id, cf);
         break;
      case cf::Type::Export:
         if (id == exp::EXP || id == exp::EXP_DONE) {
            decodeExport(state, id, cf);
         } else {
            assert(false);
         }
         break;
      case cf::Type::Alu:
      case cf::Type::AluExtended:
         decodeALU(state, id, cf);
         break;
      }

      state.cfPC++;

      if ((cf.type == cf::Type::Normal || cf.type == cf::Type::Export) && cf.word1.endOfProgram) {
         break;
      }
   }

   // Step 2: Reverse control flow to put the code into blocks
   if (!blockify(shader)) {
      return false;
   }

   // Step 3: Analyse for variables!
   if (!analyse(shader)) {
      return false;
   }

   return true;
}

static bool
decodeNormal(DecodeState &state, cf::inst id, cf::Instruction &cf)
{
   shadir::CfInstruction *ins = nullptr;

   switch (id) {
   case cf::NOP:
   case cf::CALL_FS:
   case cf::END_PROGRAM:
      return true;
   case cf::TEX:
      return decodeTEX(state, id, cf);
   case cf::LOOP_START:
   case cf::LOOP_START_DX10:
   case cf::LOOP_START_NO_AL:
   case cf::LOOP_END:
   case cf::JUMP:
   case cf::LOOP_CONTINUE:
   case cf::POP_JUMP:
   case cf::ELSE:
   case cf::CALL:
   case cf::RETURN:
   case cf::EMIT_VERTEX:
   case cf::EMIT_CUT_VERTEX:
   case cf::CUT_VERTEX:
   case cf::KILL:
   case cf::PUSH:
   case cf::PUSH_ELSE:
   case cf::POP:
   case cf::POP_PUSH:
   case cf::POP_PUSH_ELSE:
      ins = new shadir::CfInstruction;
      ins->name = cf::name[id];
      ins->cfPC = state.cfPC;
      ins->id = id;
      ins->addr = cf.word0.addr;
      ins->popCount = cf.word1.popCount;
      ins->loopCfConstant = cf.word1.cfConst;
      ins->cond = static_cast<cf::Cond::Cond>(cf.word1.cond);
      state.shader->code.emplace_back(ins);
      return true;
   case cf::VTX:
   case cf::VTX_TC:
   case cf::WAIT_ACK:
   case cf::TEX_ACK:
   case cf::VTX_ACK:
   case cf::VTX_TC_ACK:
   default:
      assert(false);
      return false;
   }
}

static bool
decodeExport(DecodeState &state, cf::inst id, cf::Instruction &cf)
{
   auto exp = new shadir::ExportInstruction {};
   exp->id = static_cast<exp::inst>(cf.expWord1.inst);
   exp->name = exp::name[exp->id];
   exp->cfPC = state.cfPC;

   exp->dstReg = cf.expWord0.dstReg;
   exp->src.id = cf.expWord0.srcReg;
   exp->src.selX = static_cast<alu::Select::Select>(cf.expWord1.srcSelX);
   exp->src.selY = static_cast<alu::Select::Select>(cf.expWord1.srcSelY);
   exp->src.selZ = static_cast<alu::Select::Select>(cf.expWord1.srcSelZ);
   exp->src.selW = static_cast<alu::Select::Select>(cf.expWord1.srcSelW);

   exp->type = static_cast<exp::Type::Type>(cf.expWord0.type);
   exp->elemSize = cf.expWord0.elemSize;
   exp->indexGpr = cf.expWord0.indexGpr;
   exp->wholeQuadMode = cf.expWord1.wholeQuadMode;
   exp->barrier = !!cf.expWord1.barrier;

   if (exp->type == exp::Type::Position) {
      assert(exp->dstReg >= 60);
      exp->dstReg -= 60;
   }

   assert(cf.expWord0.srcRel == 0);
   assert(cf.expWord1.validPixelMode == 0);
   state.shader->exports.push_back(exp);
   state.shader->code.emplace_back(exp);
   return true;
}

static void
getAluSource(shadir::AluSource &source, const uint32_t *dwBase, uint32_t counter,
             uint32_t sel, uint32_t rel, uint32_t indexMode,
             uint32_t chan, uint32_t neg, bool abs,
             uint32_t kcacheMode0, uint32_t kcacheMode1,
             uint32_t kcacheBank0, uint32_t kcacheBank1,
             uint32_t kcacheAddr0, uint32_t kcacheAddr1)
{
   source.absolute = abs;
   source.negate = !!neg;
   source.chan = static_cast<alu::Channel::Channel>(chan);
   source.rel = !!rel;
   source.indexMode = static_cast<alu::IndexMode::IndexMode>(indexMode);

   if (sel >= alu::Source::RegisterFirst && sel <= alu::Source::RegisterLast) {
      source.type = shadir::AluSource::Register;
      source.id = sel - alu::Source::RegisterFirst;
   } else if (sel >= alu::Source::KcacheBank0First && sel <= alu::Source::KcacheBank0Last) {
      // We do not currently handle kcache's based on a loop index
      assert(kcacheMode0 == KcacheMode::Lock1 || kcacheMode0 == KcacheMode::Lock2);
      source.type = static_cast<shadir::AluSource::Type>(shadir::AluSource::UniformBlock0 + kcacheBank0);
      source.id = (kcacheAddr0 * 16) + (sel - alu::Source::KcacheBank0First);
   } else if (sel >= alu::Source::KcacheBank1First && sel <= alu::Source::KcacheBank1Last) {
      // We do not currently handle kcache's based on a loop index
      assert(kcacheMode1 == KcacheMode::Lock1 || kcacheMode1 == KcacheMode::Lock2);
      source.type = static_cast<shadir::AluSource::Type>(shadir::AluSource::UniformBlock0 + kcacheBank1);
      source.id = (kcacheAddr1 * 16) + (sel - alu::Source::KcacheBank1First);
   } else if (sel == alu::Source::Src1DoubleLSW) {
      assert(false);
   } else if (sel == alu::Source::Src1DoubleMSW) {
      assert(false);
   } else if (sel == alu::Source::Src05DoubleLSW) {
      assert(false);
   } else if (sel == alu::Source::Src05DoubleMSW) {
      assert(false);
   } else if (sel == alu::Source::Src0Float) {
      source.type = shadir::AluSource::ConstantFloat;
      source.floatValue = 0.0f;
   } else if (sel == alu::Source::Src1Float) {
      source.type = shadir::AluSource::ConstantFloat;
      source.floatValue = 1.0f;
   } else if (sel == alu::Source::Src1Integer) {
      source.type = shadir::AluSource::ConstantInt;
      source.intValue = 1;
   } else if (sel == alu::Source::SrcMinus1Integer) {
      source.type = shadir::AluSource::ConstantInt;
      source.intValue = -1;
   } else if (sel == alu::Source::Src05Float) {
      source.type = shadir::AluSource::ConstantFloat;
      source.floatValue = 0.5f;
   } else if (sel == alu::Source::SrcLiteral) {
      source.type = shadir::AluSource::ConstantLiteral;
      source.literalValue = dwBase[chan];
   } else if (sel == alu::Source::SrcPreviousScalar) {
      source.type = shadir::AluSource::PreviousScalar;
      source.id = counter - 1;
   } else if (sel == alu::Source::SrcPreviousVector) {
      source.type = shadir::AluSource::PreviousVector;
      source.id = counter - 1;
   } else if (sel >= alu::Source::CfileConstantsFirst && sel <= alu::Source::CfileConstantsLast) {
      source.type = shadir::AluSource::ConstantFile;
      source.id = sel - alu::Source::CfileConstantsFirst;
   } else {
      assert(false);
   }
}


static bool
isTranscendentalOnly(alu::Instruction &alu)
{
   alu::Opcode opcode;

   if (alu.word1.encoding == alu::Encoding::OP2) {
      opcode = alu::op2info[alu.op2.inst];
   } else {
      opcode = alu::op3info[alu.op3.inst];
   }

   if (opcode.flags & alu::Opcode::Vector) {
      return false;
   }

   if (opcode.flags & alu::Opcode::Transcendental) {
      return true;
   }

   return false;
}

static bool
isVectorOnly(alu::Instruction &alu)
{
   alu::Opcode opcode;

   if (alu.word1.encoding == alu::Encoding::OP2) {
      opcode = alu::op2info[alu.op2.inst];
   } else {
      opcode = alu::op3info[alu.op3.inst];
   }

   if (opcode.flags & alu::Opcode::Transcendental) {
      return false;
   }

   if (opcode.flags & alu::Opcode::Vector) {
      return true;
   }

   return false;
}

static uint32_t
getUnit(bool units[5], alu::Instruction &alu)
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
decodeALU(DecodeState &state, cf::inst id, cf::Instruction &cf)
{
   const uint64_t *slots = reinterpret_cast<const uint64_t *>(state.words + (WordsPerCF * cf.aluWord0.addr));
   auto aluID = static_cast<alu::inst>(cf.aluWord1.inst);
   bool hasPushedBefore = false;

   switch (aluID) {
   case alu::inst::ALU_BREAK: // Break starts with a push!
   case alu::inst::ALU_CONTINUE: // Continue starts with a push!
   {
      auto push = new shadir::CfInstruction();
      push->id = cf::inst::PUSH;
      push->name = cf::name[push->id];
      push->cfPC = state.cfPC;
      state.shader->code.emplace_back(push);
   }
   break;
   case alu::inst::ALU_EXT:
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
         auto alu = *reinterpret_cast<const alu::Instruction*>(slots + slot + i);
         literalPtr += 2;
         last = !!alu.word0.last;
      }

      last = false;

      for (auto i = 0u; i < 5 && !last; ++i) {
         auto alu = *reinterpret_cast<const alu::Instruction*>(slots + slot);
         auto &opcode = alu::op2info[alu.op2.inst];
         auto unit = getUnit(units, alu);

         if (alu.word1.encoding == alu::Encoding::OP2 && opcode.id == alu::op2::NOP) {
            slot += 1;
            last = !!alu.word0.last;
            continue;
         }

         auto ins = new shadir::AluInstruction();
         bool abs0 = false, abs1 = false;

         ins->cfPC = state.cfPC;
         ins->groupPC = state.group;
         ins->predSel = static_cast<alu::PredicateSelect::PredicateSelect>(alu.word0.predSel);

         assert(alu.word1.dstRel == 0);
         ins->dest.clamp = alu.word1.clamp;
         ins->dest.id = alu.word1.dstGpr;
         ins->dest.chan = static_cast<alu::Channel::Channel>(alu.word1.dstChan);
         ins->unit = static_cast<shadir::AluInstruction::Unit>(unit);

         if (alu.word1.encoding == alu::Encoding::OP2) {
            ins->name = opcode.name;
            ins->opType = shadir::AluInstruction::OP2;
            ins->op2 = static_cast<alu::op2>(opcode.id);
            ins->numSources = opcode.srcs;
            ins->updateExecutionMask = alu.op2.updateExecuteMask;
            ins->updatePredicate = alu.op2.updatePred;
            ins->writeMask = alu.op2.writeMask;
            ins->outputModifier = static_cast<alu::OutputModifier::OutputModifier>(alu.op2.omod);
            abs0 = !!alu.op2.src0Abs;
            abs1 = !!alu.op2.src1Abs;
         } else {
            opcode = alu::op3info[alu.op3.inst];
            ins->name = opcode.name;
            ins->opType = shadir::AluInstruction::OP3;
            ins->op3 = static_cast<alu::op3>(opcode.id);
            ins->numSources = opcode.srcs;
            getAluSource(ins->sources[2], literalPtr, state.group,
               alu.op3.src2Sel, alu.op3.src2Rel, alu.word0.indexMode,
               alu.op3.src2Chan, alu.op3.src2Neg, false,
               cf.aluWord0.kcacheMode0, cf.aluWord1.kcacheMode1,
               cf.aluWord0.kcacheBank0, cf.aluWord0.kcacheBank1,
               cf.aluWord1.kcacheAddr0, cf.aluWord1.kcacheAddr1);
         }

         getAluSource(ins->sources[0], literalPtr, state.group,
            alu.word0.src0Sel, alu.word0.src0Rel, alu.word0.indexMode,
            alu.word0.src0Chan, alu.word0.src0Neg, abs0,
            cf.aluWord0.kcacheMode0, cf.aluWord1.kcacheMode1,
            cf.aluWord0.kcacheBank0, cf.aluWord0.kcacheBank1,
            cf.aluWord1.kcacheAddr0, cf.aluWord1.kcacheAddr1);
         getAluSource(ins->sources[1], literalPtr, state.group,
            alu.word0.src1Sel, alu.word0.src1Rel, alu.word0.indexMode,
            alu.word0.src1Chan, alu.word0.src1Neg, abs1,
            cf.aluWord0.kcacheMode0, cf.aluWord1.kcacheMode1,
            cf.aluWord0.kcacheBank0, cf.aluWord0.kcacheBank1,
            cf.aluWord1.kcacheAddr0, cf.aluWord1.kcacheAddr1);

         // Check whether to set sources to Int or Uint
         if (opcode.flags & alu::Opcode::IntIn) {
            for (auto j = 0u; j < ins->numSources; ++j) {
               ins->sources[j].valueType = shadir::AluSource::Int;
            }
         } else if (opcode.flags & alu::Opcode::UintIn) {
            for (auto j = 0u; j < ins->numSources; ++j) {
               ins->sources[j].valueType = shadir::AluSource::Uint;
            }
         }

         // Check whether to set dest to Int or Uint
         if (opcode.flags & alu::Opcode::IntOut) {
            ins->dest.valueType = shadir::AluDest::Int;
         } else if (opcode.flags & alu::Opcode::UintOut) {
            ins->dest.valueType = shadir::AluDest::Uint;
         }

         // Special ALU ops before
         if (opcode.flags & alu::Opcode::PredSet) {
            switch (aluID) {
            case alu::inst::ALU_PUSH_BEFORE: // PushBefore only push before first PRED_SET
            {
               if (!hasPushedBefore) {
                  auto push = new shadir::CfInstruction();
                  push->id = cf::inst::PUSH;
                  push->name = cf::name[push->id];
                  push->cfPC = state.cfPC;
                  state.shader->code.emplace_back(push);
                  hasPushedBefore = true;
               }
            }
            break;
            case alu::inst::ALU_ELSE_AFTER: // ElseAfter push before every PRED_SET
            {
               auto push = new shadir::CfInstruction();
               push->id = cf::inst::PUSH;
               push->name = cf::name[push->id];
               push->cfPC = state.cfPC;
               state.shader->code.emplace_back(push);
            }
            break;
            }
         }

         // Check if we need to create a new reduction instruction
         if (!reductionIns) {
            if (ins->opType == shadir::AluInstruction::OP2) {
               if (ins->op2 == alu::op2::DOT4
                   || ins->op2 == alu::op2::DOT4_IEEE
                   || ins->op2 == alu::op2::CUBE
                   || ins->op2 == alu::op2::MAX4) {
                  reductionIns = new shadir::AluReductionInstruction();
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
            ins->isReduction = true;
         } else {
            // Append to shader code
            state.shader->code.emplace_back(ins);
         }

         // Special ALU ops after
         if (opcode.flags & alu::Opcode::PredSet) {
            switch (aluID) {
            case alu::inst::ALU_BREAK: // Break after every PRED_SET
            {
               auto loopBreak = new shadir::CfInstruction();
               loopBreak->cfPC = state.cfPC;
               loopBreak->id = cf::inst::LOOP_BREAK;
               loopBreak->name = cf::name[loopBreak->id];
               loopBreak->popCount = 1;
               state.shader->code.emplace_back(loopBreak);
            }
            break;
            case alu::inst::ALU_CONTINUE: // Continue after every PRED_SET
            {
               auto loopContinue = new shadir::CfInstruction();
               loopContinue->cfPC = state.cfPC;
               loopContinue->id = cf::inst::LOOP_CONTINUE;
               loopContinue->name = cf::name[loopContinue->id];
               loopContinue->popCount = 1;
               state.shader->code.emplace_back(loopContinue);
            }
            break;
            case alu::inst::ALU_ELSE_AFTER: // Else after every PRED_SET
            {
               auto els = new shadir::CfInstruction();
               els->cfPC = state.cfPC;
               els->id = cf::inst::ELSE;
               els->name = cf::name[els->id];
               els->popCount = 1;
               state.shader->code.emplace_back(els);
            }
            break;
            }
         }

         // Count number of literal used
         if (alu.word0.src0Sel == alu::Source::SrcLiteral) {
            literals = std::max<unsigned>(literals, alu.word0.src0Chan + 1);
         }

         if (alu.word0.src1Sel == alu::Source::SrcLiteral) {
            literals = std::max<unsigned>(literals, alu.word0.src1Chan + 1);
         }

         if ((alu.word1.encoding != alu::Encoding::OP2) && (alu.op3.src2Sel == alu::Source::SrcLiteral)) {
            literals = std::max<unsigned>(literals, alu.op3.src2Chan + 1);
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
   case alu::inst::ALU_BREAK: // Break ends with POP
   case alu::inst::ALU_CONTINUE: // Continue ends with POP
   case alu::inst::ALU_POP_AFTER: // Pop after ALU clause
   {
      auto pop = new shadir::CfInstruction();
      pop->cfPC = state.cfPC;
      pop->id = cf::inst::POP;
      pop->name = cf::name[pop->id];
      pop->popCount = 1;
      state.shader->code.emplace_back(pop);
   }
   break;
   case alu::inst::ALU_POP2_AFTER: // Pop2 after ALU clause
   {
      auto pop2 = new shadir::CfInstruction();
      pop2->cfPC = state.cfPC;
      pop2->id = cf::inst::POP;
      pop2->name = cf::name[pop2->id];
      pop2->popCount = 2;
      state.shader->code.emplace_back(pop2);
   }
   break;
   }

   return true;
}

static bool
decodeTEX(DecodeState &state, cf::inst id, cf::Instruction &cf)
{
   const uint32_t *ptr = state.words + (WordsPerCF * cf.word0.addr);

   for (auto slot = 0u; slot <= cf.word1.count; ) {
      auto tex = *reinterpret_cast<const tex::Instruction*>(ptr);
      auto id = static_cast<const tex::inst>(tex.word0.inst);

      if (id == tex::VTX_FETCH || id == tex::VTX_SEMANTIC || id == tex::GET_BUFFER_RESINFO) {
         assert(false);
         ptr += WordsPerVTX;
      } else if (id == tex::MEM) {
         assert(false);
         ptr += WordsPerMEM;
      } else {
         auto ins = new shadir::TexInstruction;
         ins->name = tex::name[id];
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
         ins->src.selX = static_cast<alu::Select::Select>(tex.word2.srcSelX);
         ins->src.selY = static_cast<alu::Select::Select>(tex.word2.srcSelY);
         ins->src.selZ = static_cast<alu::Select::Select>(tex.word2.srcSelZ);
         ins->src.selW = static_cast<alu::Select::Select>(tex.word2.srcSelW);

         assert(tex.word1.dstRel == 0);
         ins->dst.id = tex.word1.dstReg;
         ins->dst.selX = static_cast<alu::Select::Select>(tex.word1.dstSelX);
         ins->dst.selY = static_cast<alu::Select::Select>(tex.word1.dstSelY);
         ins->dst.selZ = static_cast<alu::Select::Select>(tex.word1.dstSelZ);
         ins->dst.selW = static_cast<alu::Select::Select>(tex.word1.dstSelW);

         state.shader->code.emplace_back(ins);
         ptr += WordsPerTEX;
      }

      state.group++;
      slot++;
   }

   return true;
}

} // namespace latte
