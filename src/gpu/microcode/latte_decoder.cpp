#include <gsl.h>
#include <spdlog/details/format.h>
#include "latte_decoder.h"
#include "utils/bit_cast.h"
#include "utils/bitutils.h"

namespace latte
{

struct DecodeState
{
   gsl::span<const uint8_t> binary;
   std::string indent;
   uint32_t cfPC = 0;
   uint32_t groupPC = 0;
   Shader *shader = nullptr;
};

static void
decodeError(DecodeState &state, const std::string &error)
{
   state.shader->decodeMessages.push_back(fmt::format("cf{}:g{} {}", state.cfPC, state.groupPC, error));
}

static bool
decodeTEX(DecodeState &state, shadir::CfInstruction *parent, uint32_t addr, uint32_t count)
{
   auto result = true;
   auto clause = reinterpret_cast<const TextureFetchInst *>(state.binary.data() + 8 * addr);

   for (auto i = 0u; i < count; ++i) {
      const auto &tex = clause[i];
      auto id = tex.word0.TEX_INST;
      auto name = getInstructionName(id);

      if (id == SQ_TEX_INST_VTX_FETCH || id == SQ_TEX_INST_VTX_SEMANTIC || id == SQ_TEX_INST_GET_BUFFER_RESINFO) {
         decodeError(state, fmt::format("Unexpected vertex fetch instruction in texture fetch clause {} {}", id, name));
         result = false;
      } else if (id == SQ_TEX_INST_MEM) {
         decodeError(state, fmt::format("Unexpected mem instruction in texture fetch clause {} {}", id, name));
         result = false;
      } else {
         auto inst = new shadir::TextureFetchInstruction {};
         inst->name = name;
         inst->cfPC = state.cfPC;
         inst->groupPC = state.groupPC;

         inst->id = id;
         inst->bcFracMode = tex.word0.BC_FRAC_MODE;
         inst->fetchWholeQuad = tex.word0.FETCH_WHOLE_QUAD;
         inst->altConst = tex.word0.ALT_CONST;
         inst->resourceID = tex.word0.RESOURCE_ID;
         inst->samplerID = tex.word2.SAMPLER_ID;

         inst->src.id = tex.word0.SRC_GPR;
         inst->src.rel = tex.word0.SRC_REL;
         inst->src.sel[0] = tex.word2.SRC_SEL_X;
         inst->src.sel[1] = tex.word2.SRC_SEL_Y;
         inst->src.sel[2] = tex.word2.SRC_SEL_Z;
         inst->src.sel[3] = tex.word2.SRC_SEL_W;

         inst->dst.id = tex.word1.DST_GPR;
         inst->dst.rel = tex.word1.DST_REL;
         inst->dst.sel[0] = tex.word1.DST_SEL_X;
         inst->dst.sel[1] = tex.word1.DST_SEL_Y;
         inst->dst.sel[2] = tex.word1.DST_SEL_Z;
         inst->dst.sel[3] = tex.word1.DST_SEL_W;

         inst->lodBias = sign_extend<7>(tex.word1.LOD_BIAS);

         inst->offset[0] = sign_extend<5>(tex.word2.OFFSET_X);
         inst->offset[1] = sign_extend<5>(tex.word2.OFFSET_Y);
         inst->offset[2] = sign_extend<5>(tex.word2.OFFSET_Z);

         inst->coordNormalise[0] = !!tex.word1.COORD_TYPE_X;
         inst->coordNormalise[1] = !!tex.word1.COORD_TYPE_Y;
         inst->coordNormalise[2] = !!tex.word1.COORD_TYPE_Z;
         inst->coordNormalise[3] = !!tex.word1.COORD_TYPE_W;
         parent->clause.emplace_back(inst);
      }

      state.groupPC++;
   }

   return result;
}

static bool
decodeVTX(DecodeState &state, shadir::CfInstruction *parent, uint32_t addr, uint32_t count)
{
   auto result = true;
   auto clause = reinterpret_cast<const VertexFetchInst *>(state.binary.data() + 8 * addr);

   for (auto i = 0u; i < count; ++i) {
      const auto &vtx = clause[i];
      auto id = vtx.word0.VTX_INST;
      auto name = getInstructionName(id);

      if (id != SQ_VTX_INST_FETCH && id != SQ_VTX_INST_SEMANTIC && id != SQ_VTX_INST_BUFINFO) {
         decodeError(state, fmt::format("Unexpected instruction in vertex fetch clause {} {}", id, name));
         result = false;
      } else {
         auto inst = new shadir::VertexFetchInstruction {};
         inst->name = name;
         inst->cfPC = state.cfPC;
         inst->groupPC = state.groupPC;

         inst->id = id;
         inst->fetchType = vtx.word0.FETCH_TYPE;
         inst->fetchWholeQuad = !!vtx.word0.FETCH_WHOLE_QUAD;
         inst->bufferID = vtx.word0.BUFFER_ID;
         inst->src.id = vtx.word0.SRC_GPR;
         inst->src.rel = vtx.word0.SRC_REL;
         inst->src.sel = vtx.word0.SRC_SEL_X;

         if (id == SQ_VTX_INST_SEMANTIC) {
            inst->dst.id = vtx.word1.SEM.SEMANTIC_ID;
            inst->dst.rel = SQ_ABSOLUTE;
         } else {
            inst->dst.id = vtx.word1.GPR.DST_GPR;
            inst->dst.rel = vtx.word1.GPR.DST_REL;
         }

         inst->dst.sel[0] = vtx.word1.DST_SEL_X;
         inst->dst.sel[1] = vtx.word1.DST_SEL_Y;
         inst->dst.sel[2] = vtx.word1.DST_SEL_Z;
         inst->dst.sel[3] = vtx.word1.DST_SEL_W;

         inst->useConstFields = vtx.word1.USE_CONST_FIELDS;
         inst->dataFormat = vtx.word1.DATA_FORMAT;
         inst->numFormat = vtx.word1.NUM_FORMAT_ALL;
         inst->formatComp = vtx.word1.FORMAT_COMP_ALL;
         inst->srfMode = vtx.word1.SRF_MODE_ALL;
         inst->endian = vtx.word2.ENDIAN_SWAP;

         inst->offset = vtx.word2.OFFSET;
         inst->constBufNoStride = vtx.word2.CONST_BUF_NO_STRIDE;
         inst->altConst = vtx.word2.ALT_CONST;

         inst->megaFetch = vtx.word2.MEGA_FETCH;
         inst->megaFetchCount = vtx.word0.MEGA_FETCH_COUNT;
         parent->clause.emplace_back(inst);
      }

      state.groupPC++;
   }

   return result;
}

static bool
decodeNormal(DecodeState &state, const ControlFlowInst &inst)
{
   auto result = true;
   auto id = inst.word1.CF_INST;
   auto name = getInstructionName(id);

   switch (id) {
   case SQ_CF_INST_WAIT_ACK:
   case SQ_CF_INST_TEX_ACK:
   case SQ_CF_INST_VTX_ACK:
   case SQ_CF_INST_VTX_TC_ACK:
      decodeError(state, fmt::format("Unable to decode instruction {} {}", id, name));
      return false;
   }

   auto cf = new shadir::CfInstruction {};
   cf->cfPC = state.cfPC;
   cf->name = name;

   cf->id = id;
   cf->addr = inst.word0.ADDR;
   cf->popCount = inst.word1.POP_COUNT;
   cf->callCount = inst.word1.CALL_COUNT;
   cf->constant = inst.word1.CF_CONST;
   cf->cond = inst.word1.COND;
   cf->barrier = !!inst.word1.BARRIER;
   cf->validPixelMode = !!inst.word1.VALID_PIXEL_MODE;
   cf->wholeQuadMode = !!inst.word1.WHOLE_QUAD_MODE;

   auto addr = inst.word0.ADDR;
   auto count = (inst.word1.COUNT + 1) | (inst.word1.COUNT_3 << 3);

   // Decode instruction clause
   switch (id) {
   case SQ_CF_INST_TEX:
      result &= decodeTEX(state, cf, addr, count);
      break;
   case SQ_CF_INST_VTX:
   case SQ_CF_INST_VTX_TC:
      result &= decodeVTX(state, cf, addr, count);
      break;
   }

   state.shader->code.emplace_back(cf);
   return true;
}

static bool
decodeExport(DecodeState &state, const ControlFlowInst &cf)
{
   auto id = cf.exp.word1.CF_INST;
   auto name = getInstructionName(id);

   auto inst = new shadir::ExportInstruction {};
   inst->cfPC = state.cfPC;
   inst->name = name;

   inst->id = id;
   inst->arrayBase = cf.exp.word0.ARRAY_BASE;
   inst->exportType = cf.exp.word0.TYPE;
   inst->rw.id = cf.exp.word0.RW_GPR;
   inst->rw.rel = cf.exp.word0.RW_REL;
   inst->index = cf.exp.word0.INDEX_GPR;
   inst->elemSize = cf.exp.word0.ELEM_SIZE;
   inst->burstCount = cf.exp.word1.BURST_COUNT;
   inst->validPixelMode = !!cf.exp.word1.VALID_PIXEL_MODE;
   inst->wholeQuadMode = !!cf.exp.word1.WHOLE_QUAD_MODE;
   inst->barrier = !!cf.exp.word1.BARRIER;

   if (id == SQ_CF_INST_EXP || id == SQ_CF_INST_EXP_DONE) {
      inst->isSemantic = true;
      inst->srcSel[0] = cf.exp.swiz.SRC_SEL_X;
      inst->srcSel[1] = cf.exp.swiz.SRC_SEL_Y;
      inst->srcSel[2] = cf.exp.swiz.SRC_SEL_Z;
      inst->srcSel[3] = cf.exp.swiz.SRC_SEL_W;
   } else {
      inst->isSemantic = false;
      inst->arraySize = cf.exp.buf.ARRAY_SIZE;
      inst->compMask = cf.exp.buf.COMP_MASK;
   }

   state.shader->exports.push_back(inst);
   state.shader->code.emplace_back(inst);
   return true;
}

static bool
isTranscendentalOnly(uint32_t flags)
{
   if (flags & SQ_ALU_FLAG_VECTOR) {
      return false;
   }

   if (flags & SQ_ALU_FLAG_TRANSCENDENTAL) {
      return true;
   }

   return false;
}

static bool
isVectorOnly(uint32_t flags)
{
   if (flags & SQ_ALU_FLAG_TRANSCENDENTAL) {
      return false;
   }

   if (flags & SQ_ALU_FLAG_VECTOR) {
      return true;
   }

   return false;
}

static bool
decodeAluClause(DecodeState &state, shadir::CfAluInstruction *parent, uint32_t addr, uint32_t slots)
{
   auto result = true;
   auto clause = reinterpret_cast<const AluInst *>(state.binary.data() + 8 * addr);

   for (auto slot = 0u; slot < slots; ) {
      bool units[5] = { false, false, false, false, false };
      auto group = &clause[slot];
      auto instructionCount = 0u;
      auto literalCount = 0u;

      // Find how many instructions in this group
      for (instructionCount = 1u; instructionCount <= 5u; ++instructionCount) {
         auto &inst = group[instructionCount - 1];
         auto srcCount = 0u;

         if (inst.word1.ENCODING == SQ_ALU_OP2) {
            srcCount = getInstructionNumSrcs(inst.op2.ALU_INST);
         } else {
            srcCount = getInstructionNumSrcs(inst.op3.ALU_INST);
         }

         if (srcCount > 0 && inst.word0.SRC0_SEL == SQ_ALU_SRC_LITERAL) {
            literalCount = std::max<unsigned>(literalCount, inst.word0.SRC0_CHAN + 1);
         }

         if (srcCount > 1 && inst.word0.SRC1_SEL == SQ_ALU_SRC_LITERAL) {
            literalCount = std::max<unsigned>(literalCount, inst.word0.SRC1_CHAN + 1);
         }

         if (srcCount > 2 && inst.op3.SRC2_SEL == SQ_ALU_SRC_LITERAL) {
            literalCount = std::max<unsigned>(literalCount, inst.op3.SRC2_CHAN + 1);
         }

         if (inst.word0.LAST) {
            break;
         }
      }

      auto literals = reinterpret_cast<const uint32_t *>(group + instructionCount);

      for (auto j = 0u; j < instructionCount; ++j) {
         auto &inst = group[j];
         bool src0_abs = false, src1_abs = false;
         const char *name = nullptr;
         auto srcCount = 0u;
         SQ_ALU_FLAGS flags;

         if (inst.word1.ENCODING == SQ_ALU_OP2) {
            name = getInstructionName(inst.op2.ALU_INST);
            flags = getInstructionFlags(inst.op2.ALU_INST);
            srcCount = getInstructionNumSrcs(inst.op2.ALU_INST);
         } else {
            name = getInstructionName(inst.op3.ALU_INST);
            flags = getInstructionFlags(inst.op3.ALU_INST);
            srcCount = getInstructionNumSrcs(inst.op3.ALU_INST);
         }

         // Find execution scalar unit
         auto unit = 0u;

         if (isTranscendentalOnly(flags)) {
            unit = SQ_CHAN_T;
         } else if (isVectorOnly(flags)) {
            unit = inst.word1.DST_CHAN;
         } else if (units[inst.word1.DST_CHAN]) {
            unit = SQ_CHAN_T;
         } else {
            unit = inst.word1.DST_CHAN;
         }

         if (units[unit]) {
            decodeError(state, fmt::format("Clause instruction unit collision for unit {}", unit));
         }

         units[unit] = true;

         auto alu = new shadir::AluInstruction {};
         alu->cfPC = state.cfPC;
         alu->groupPC = state.groupPC;

         alu->name = name;
         alu->flags = flags;
         alu->srcCount = srcCount;

         alu->encoding = inst.word1.ENCODING;
         alu->unit = static_cast<SQ_CHAN>(unit);
         alu->parent = parent;
         alu->bankSwizzle = inst.word1.BANK_SWIZZLE;
         alu->predSel = inst.word0.PRED_SEL;
         alu->indexMode = inst.word0.INDEX_MODE;

         if (inst.word1.ENCODING == SQ_ALU_OP2) {
            alu->op2 = inst.op2.ALU_INST;
            alu->updateExecuteMask = inst.op2.UPDATE_EXECUTE_MASK;
            alu->updatePredicate = inst.op2.UPDATE_PRED;
            alu->writeMask = inst.op2.WRITE_MASK;
            alu->outputModifier = inst.op2.OMOD;
            src0_abs = !!inst.op2.SRC0_ABS;
            src1_abs = !!inst.op2.SRC1_ABS;
         } else {
            alu->op3 = inst.op3.ALU_INST;
            alu->encoding = SQ_ALU_OP3;
            src0_abs = false;
            src1_abs = false;
         }

         auto srcType = shadir::ValueType::Float;

         if (flags & SQ_ALU_FLAG_INT_IN) {
            srcType = shadir::ValueType::Int;
         } else if (flags & SQ_ALU_FLAG_UINT_IN) {
            srcType = shadir::ValueType::Uint;
         }

         // Decode sources
         if (alu->srcCount > 0) {
            auto &src = alu->src[0];
            src.sel = inst.word0.SRC0_SEL;
            src.rel = inst.word0.SRC0_REL;
            src.chan = inst.word0.SRC0_CHAN;
            src.negate = inst.word0.SRC0_NEG;
            src.absolute = src0_abs;
            src.type = srcType;

            if (src.sel == SQ_ALU_SRC_LITERAL) {
               src.literalUint = literals[src.chan];
            }
         }

         if (alu->srcCount > 1) {
            auto &src = alu->src[1];
            src.sel = inst.word0.SRC1_SEL;
            src.rel = inst.word0.SRC1_REL;
            src.chan = inst.word0.SRC1_CHAN;
            src.negate = inst.word0.SRC1_NEG;
            src.absolute = src1_abs;
            src.type = srcType;

            if (src.sel == SQ_ALU_SRC_LITERAL) {
               src.literalUint = literals[src.chan];
            }
         }

         if (alu->srcCount > 2) {
            auto &src = alu->src[2];
            src.sel = inst.op3.SRC2_SEL;
            src.rel = inst.op3.SRC2_REL;
            src.chan = inst.op3.SRC2_CHAN;
            src.negate = inst.op3.SRC2_NEG;
            src.absolute = false;
            src.type = srcType;

            if (src.sel == SQ_ALU_SRC_LITERAL) {
               src.literalUint = literals[src.chan];
            }
         }

         // Decode dest
         alu->dst.sel = inst.word1.DST_GPR;
         alu->dst.rel = inst.word1.DST_REL;
         alu->dst.chan = inst.word1.DST_CHAN;
         alu->dst.clamp = inst.word1.CLAMP;

         if (flags & SQ_ALU_FLAG_INT_OUT) {
            alu->dst.type = shadir::ValueType::Int;
         } else if (flags & SQ_ALU_FLAG_UINT_OUT) {
            alu->dst.type = shadir::ValueType::Uint;
         } else {
            alu->dst.type = shadir::ValueType::Float;
         }

         parent->clause.emplace_back(alu);
      }

      slot += instructionCount;
      slot += (literalCount + 1) / 2;
      state.groupPC++;
   }

   return true;
}

static bool
decodeAlu(DecodeState &state, const ControlFlowInst &cf)
{
   auto parent = new shadir::CfAluInstruction {};
   parent->id = cf.alu.word1.CF_INST;
   parent->name = getInstructionName(cf.alu.word1.CF_INST);
   parent->cfPC = state.cfPC;
   parent->addr = cf.alu.word0.ADDR;
   parent->kcache[0].bank = cf.alu.word0.KCACHE_BANK0;
   parent->kcache[0].mode = cf.alu.word0.KCACHE_MODE0;
   parent->kcache[0].addr = cf.alu.word1.KCACHE_ADDR0;
   parent->kcache[1].bank = cf.alu.word0.KCACHE_BANK1;
   parent->kcache[1].mode = cf.alu.word1.KCACHE_MODE1;
   parent->kcache[1].addr = cf.alu.word1.KCACHE_ADDR1;
   parent->altConst = cf.alu.word1.ALT_CONST;
   parent->wholeQuadMode = cf.alu.word1.WHOLE_QUAD_MODE;
   parent->barrier = cf.alu.word1.BARRIER;
   state.shader->code.emplace_back(parent);

   auto addr = cf.alu.word0.ADDR;
   auto count = cf.alu.word1.COUNT + 1;
   return decodeAluClause(state, parent, addr, count);
}


/**
 * Decode from binary format into a nicer in memory representation of structs.
 */
bool
decode(Shader &shader, const gsl::span<const uint8_t> &binary)
{
   auto result = true;
   DecodeState state;
   state.binary = binary;
   state.shader = &shader;
   state.cfPC = 0;
   state.groupPC = 0;

   for (auto i = 0; i < binary.size(); i += sizeof(ControlFlowInst)) {
      auto cf = *reinterpret_cast<const ControlFlowInst *>(binary.data() + i);
      auto id = cf.word1.CF_INST;

      switch (cf.CF_INST_TYPE) {
      case SQ_CF_INST_TYPE_NORMAL:
         result &= decodeNormal(state, cf);
         break;
      case SQ_CF_INST_TYPE_EXPORT:
         result &= decodeExport(state, cf);
         break;
      case SQ_CF_INST_TYPE_ALU:
      case SQ_CF_INST_TYPE_ALU_EXTENDED:
         result &= decodeAlu(state, cf);
         break;
      }

      if (cf.CF_INST_TYPE == SQ_CF_INST_TYPE_NORMAL || cf.CF_INST_TYPE == SQ_CF_INST_TYPE_EXPORT) {
         if (cf.word1.END_OF_PROGRAM) {
            break;
         }
      }

      state.cfPC++;
   }

   return result;
}

} // namespace latte
