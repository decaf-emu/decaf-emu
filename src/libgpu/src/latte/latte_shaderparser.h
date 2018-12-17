#pragma once
#include "latte/latte_constants.h"
#include "latte/latte_instructions.h"
#include "latte/latte_registers_spi.h"
#include "latte/latte_registers_sq.h"
#include "latte_decoders.h"

#include <common/decaf_assert.h>
#include <cstdint>
#include <gsl/gsl>

namespace latte
{

class ShaderParser
{
public:
   enum class Type : uint32_t
   {
      Unknown,
      Fetch,
      Vertex,
      Geometry,
      DataCache,
      Pixel
   };

#define TEX_INST(x, ...) \
   virtual void translateTex_##x(const ControlFlowInst &cf, const TextureFetchInst &inst) { \
      decaf_abort("Unimplemented TEX instruction "#x); \
   }
#include "latte/latte_instructions_def.inl"
#undef TEX_INST

#define VTX_INST(x, ...) \
   virtual void translateVtx_##x(const ControlFlowInst &cf, const VertexFetchInst &ist) { \
      decaf_abort("Unimplemented VTX instruction "#x); \
   }
#include "latte/latte_instructions_def.inl"
#undef VTX_INST

#define ALU_OP2(x, ...) \
   virtual void translateAluOp2_##x(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) { \
      decaf_abort("Unimplemented ALU OP2 instruction "#x); \
   }
#include "latte/latte_instructions_def.inl"
#undef ALU_OP2

#define ALU_OP3(x, ...) \
   virtual void translateAluOp3_##x(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst) { \
      decaf_abort("Unimplemented ALU OP3 instruction "#x); \
   }
#include "latte/latte_instructions_def.inl"
#undef ALU_OP3

#define CF_INST(x, ...) \
   virtual void translateCf_##x(const ControlFlowInst &cf) { \
      decaf_abort("Unimplemented CF NORMAL instruction "#x); \
   }
#include "latte/latte_instructions_def.inl"
#undef CF_INST

#define EXP_INST(x, ...) \
   virtual void translateCf_##x(const ControlFlowInst &cf) { \
      decaf_abort("Unimplemented CF EXPORT instruction "#x); \
   }
#include "latte/latte_instructions_def.inl"
#undef EXP_INST

#define ALU_INST(x, ...) \
   virtual void translateCf_##x(const ControlFlowInst &cf) { \
      decaf_abort("Unimplemented CF ALU instruction "#x); \
   }
#include "latte/latte_instructions_def.inl"
#undef ALU_INST

   virtual void translateTexInst(const ControlFlowInst &cf, const TextureFetchInst &inst)
   {
      switch (inst.word0.TEX_INST()) {
#define TEX_INST(x, ...) \
      case latte::SQ_TEX_INST_##x: \
         translateTex_##x(cf, inst); \
         break;
#include "latte/latte_instructions_def.inl"
#undef TEX_INST

      default:
         decaf_abort("Unexpected TEX instruction");
      }
   }

   virtual void translateVtxInst(const ControlFlowInst &cf, const VertexFetchInst &inst)
   {
      switch (inst.word0.VTX_INST()) {
#define VTX_INST(x, ...) \
      case latte::SQ_VTX_INST_##x: \
         translateVtx_##x(cf, inst); \
         break;
#include "latte/latte_instructions_def.inl"
#undef VTX_INST

      default:
         decaf_abort("Unexpected VTX instruction");
      }
   }

   virtual void translateAluInst(const ControlFlowInst &cf, const AluInstructionGroup &group, SQ_CHAN unit, const AluInst &inst)
   {
      const AluInst *instX = &inst;

      bool isReducInst = (unit == 0xffffffff);
      if (isReducInst) {
         instX = group.units[SQ_CHAN::X];
      }

      if (instX->word1.ENCODING() == SQ_ALU_ENCODING::OP2) {
         switch (instX->op2.ALU_INST()) {
#define ALU_OP2(x, ...) \
      case latte::SQ_OP2_INST_##x: \
         translateAluOp2_##x(cf, group, unit, inst); \
         break;
#include "latte/latte_instructions_def.inl"
#undef ALU_OP2

         default:
            decaf_abort("Unexpected ALU OP2 instruction");
         }
      } else {
         switch (instX->op3.ALU_INST()) {
#define ALU_OP3(x, ...) \
      case latte::SQ_OP3_INST_##x: \
         translateAluOp3_##x(cf, group, unit, inst); \
         break;
#include "latte/latte_instructions_def.inl"
#undef ALU_OP3

         default:
            decaf_abort("Unexpected ALU OP3 instruction");
         }
      }
   }

   virtual void translateTexClause(const ControlFlowInst &cf)
   {
      auto addr = cf.word0.ADDR();
      auto count = (cf.word1.COUNT() | (cf.word1.COUNT_3() << 3)) + 1;

      auto texInstData = reinterpret_cast<const TextureFetchInst *>(mBinary.data() + 8 * addr);
      auto texInsts = gsl::make_span(texInstData, count);

      for (auto i = 0; i < texInsts.size(); ++i) {
         translateTexInst(cf, texInsts[i]);

         mTexVtxPC++;
      }
   }

   virtual void translateVtxClause(const ControlFlowInst &cf)
   {
      auto addr = cf.word0.ADDR();
      auto count = (cf.word1.COUNT() | (cf.word1.COUNT_3() << 3)) + 1;

      auto vtxInstData = reinterpret_cast<const VertexFetchInst *>(mBinary.data() + 8 * addr);
      auto vtxInsts = gsl::make_span(vtxInstData, count);

      for (auto i = 0; i < vtxInsts.size(); ++i) {
         translateVtxInst(cf, vtxInsts[i]);

         mTexVtxPC++;
      }
   }

   virtual void translateAluGroup(const ControlFlowInst &cf, const AluInstructionGroup &group)
   {
      auto aluUnitIdx = 0;

      if (isReductionInst(*group.units[0])) {
         // For sanity, let's ensure every instruction in this reduction group
         // has the same instruction id and the same CLAMP + OMOD values
         for (auto i = 1u; i < 4; ++i) {
            if (group.units[0]->word1.ENCODING() == SQ_ALU_ENCODING::OP2) {
               if (group.units[i]->op2.ALU_INST() != group.units[0]->op2.ALU_INST()) {
                  decaf_abort("Expected every instruction in reduction group to be the same.");
               }

               if (group.units[i]->op2.OMOD() != group.units[0]->op2.OMOD()) {
                  decaf_abort("Expected every instruction in reduction group to have the same output modifier.");
               }
            } else {
               if (group.units[i]->op3.ALU_INST() != group.units[0]->op3.ALU_INST()) {
                  decaf_abort("Expected every instruction in reduction group to be the same.");
               }
            }

            if (group.units[i]->word1.CLAMP() != group.units[0]->word1.CLAMP()) {
               decaf_abort("Expected every instruction in reduction group to have the same clamp value.");
            }
         }

         // Translate the reduction by doing executing the ALU instruction only
         //  once for the whole reduction.  We pass an invalid instruction to try
         //  and avoid having people accidentally use it...
         static const uint32_t InvalidInstData[] = { 0xffffffff, 0xffffffff };
         static const auto InvalidInstPtr = reinterpret_cast<const AluInst *>(InvalidInstData);
         translateAluInst(cf, group, static_cast<SQ_CHAN>(0xffffffff), *InvalidInstPtr);

         // Skip the 4 units we just processed as a reduction instruction
         aluUnitIdx += 4;
      }

      for (; aluUnitIdx < 5; ++aluUnitIdx) {
         auto &inst = *group.units[aluUnitIdx];

         // Should never encounter reduction instructions during normal ALU
         //  instruction processing after the above logic.
         decaf_check(!isReductionInst(inst));

         // Translate the ALU instruction
         translateAluInst(cf, group, static_cast<SQ_CHAN>(aluUnitIdx), inst);

         if (inst.word0.LAST()) {
            break;
         }
      }
   }

   virtual void translateAluClause(const ControlFlowInst &cf)
   {
      auto addr = cf.alu.word0.ADDR();
      auto count = cf.alu.word1.COUNT() + 1;

      auto aluInstData = reinterpret_cast<const AluInst *>(mBinary.data() + 8 * addr);
      auto aluInsts = gsl::make_span(aluInstData, count);

      AluClauseParser clauseParser(aluInsts, mAluInstPreferVector);
      while (!clauseParser.isEndOfClause()) {
         auto group = clauseParser.readOneGroup();

         translateAluGroup(cf, group);

         mGroupPC++;
      }
   }

   virtual void translateCfNormalInst(const ControlFlowInst &cf)
   {
      switch (cf.word1.CF_INST()) {
#define CF_INST(x, ...) \
      case latte::SQ_CF_INST_##x: \
         translateCf_##x(cf); \
         break;
#include "latte/latte_instructions_def.inl"
#undef CF_INST

      default:
         decaf_abort("Unexpected CF NORMAL instruction id");
      }

      // We need special handling for a couple of the instruction types
      //  as they affect the control flow of the parser.
      switch (cf.word1.CF_INST()) {
      case SQ_CF_INST_RETURN:
         // RETURN inside a non-function block doesn't make any sense
         decaf_check(mIsFunction);

         // Mark EOP as having been reached
         mReachedEop = true;
      }

      // Handle potential END_OF_PROGRAM bit being set
      if (cf.word1.END_OF_PROGRAM()) {
         mReachedEop = true;
      }
   }

   virtual void translateCfExportInst(const ControlFlowInst& cf)
   {
      switch (cf.exp.word1.CF_INST()) {
#define EXP_INST(x, ...) \
      case latte::SQ_CF_INST_##x: \
         translateCf_##x(cf); \
         break;
#include "latte/latte_instructions_def.inl"
#undef EXP_INST

      default:
         decaf_abort("Unexpected CF EXPORT instruction id");
      }

      // Handle potential END_OF_PROGRAM bit being set
      if (cf.word1.END_OF_PROGRAM()) {
         mReachedEop = true;
      }
   }

   virtual void translateCfAluInst(const ControlFlowInst &cf)
   {
      switch (cf.alu.word1.CF_INST()) {
#define ALU_INST(x, ...) \
      case latte::SQ_CF_INST_##x: \
         translateCf_##x(cf); \
         break;
#include "latte/latte_instructions_def.inl"
#undef ALU_INST

      default:
         decaf_abort("Unexpected CF ALU instruction id");
      }
   }

   virtual void translateCfInst(const ControlFlowInst& cf)
   {
      switch (cf.word1.CF_INST_TYPE()) {
      case SQ_CF_INST_TYPE_NORMAL:
         translateCfNormalInst(cf);
         break;
      case SQ_CF_INST_TYPE_EXPORT:
         translateCfExportInst(cf);
         break;
      case SQ_CF_INST_TYPE_ALU:
      case SQ_CF_INST_TYPE_ALU_EXTENDED:
         translateCfAluInst(cf);
         break;
      default:
         decaf_abort("Unexpected CF instruction type");
      }
   }

   virtual void translate()
   {
      decaf_check(mType != Type::Unknown);
      decaf_check(!mBinary.empty());

      if (mType == Type::Fetch) {
         mIsFunction = true;
      }

      for (auto i = 0; i < mBinary.size() && !mReachedEop; i += sizeof(ControlFlowInst)) {
         auto cf = *reinterpret_cast<const ControlFlowInst *>(mBinary.data() + i);
         translateCfInst(cf);

         mCfPC++;
      }
   }

   virtual void reset()
   {
      mIsFunction = false;
      mReachedEop = false;
      mCfPC = 0;
      mGroupPC = 0;
      mTexVtxPC = 0;
   }

protected:
   // Input
   Type mType = Type::Unknown;
   gsl::span<const uint8_t> mBinary;
   bool mAluInstPreferVector;

   // Temporaries
   bool mIsFunction = false;
   bool mReachedEop = false;
   uint32_t mCfPC = 0;
   uint32_t mGroupPC = 0;
   uint32_t mTexVtxPC = 0;

};

} // namespace latte
