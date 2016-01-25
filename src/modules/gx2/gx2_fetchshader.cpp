#include "gx2_shaders.h"
#include "gx2_enum.h"
#include "gx2_format.h"
#include "utils/align.h"
#include "gpu/microcode/latte_instructions.h"

namespace gx2
{

struct IndexMapEntry
{
   uint32_t gpr;
   latte::SQ_CHAN chan;
};

static const auto
FetchesPerControlFlow = 16u;

static uint32_t
GX2FetchInstsPerAttrib(GX2FetchShaderType type);

static uint32_t
GX2FSCalcNumFetchInsts(uint32_t attribs, GX2FetchShaderType type);

static uint32_t
GX2FSCalcNumTessALUInsts(GX2FetchShaderType type, GX2TessellationMode mode);

static uint32_t
GX2FSCalcNumAluInsts(GX2FetchShaderType type, GX2TessellationMode mode);

static uint32_t
GX2FSCalcNumFetchCFInsts(uint32_t fetches);

static uint32_t
GX2FSCalcNumCFInsts(uint32_t fetches, GX2FetchShaderType type);

static const IndexMapEntry *
GX2FSGetIndexGprMap(GX2FetchShaderType type,
                    GX2TessellationMode mode);

static const IndexMapEntry IndexMapNoTess[] = {
   { 0, latte::SQ_CHAN_X },
   { 0, latte::SQ_CHAN_X },
   { 0, latte::SQ_CHAN_X },
   { 0, latte::SQ_CHAN_X },
};

static const IndexMapEntry IndexMapLineTess[] = {
   { 0, latte::SQ_CHAN_Y },
   { 0, latte::SQ_CHAN_Z },
   { 0, latte::SQ_CHAN_Y },
   { 0, latte::SQ_CHAN_Y },
};

static const IndexMapEntry IndexMapLineTessAdaptive[] = {
   { 6, latte::SQ_CHAN_X },
   { 6, latte::SQ_CHAN_Y },
   { 6, latte::SQ_CHAN_Z },
   { 6, latte::SQ_CHAN_X },
};

static const IndexMapEntry IndexMapTriTess[] = {
   { 1, latte::SQ_CHAN_X },
   { 1, latte::SQ_CHAN_Y },
   { 1, latte::SQ_CHAN_Z },
   { 1, latte::SQ_CHAN_X },
};

static const IndexMapEntry IndexMapTriTessAdaptive[] = {
   { 6, latte::SQ_CHAN_X },
   { 6, latte::SQ_CHAN_Y },
   { 6, latte::SQ_CHAN_Z },
   { 6, latte::SQ_CHAN_X },
};

static const IndexMapEntry IndexMapQuadTess[] = {
   { 0, latte::SQ_CHAN_Z },
   { 1, latte::SQ_CHAN_X },
   { 1, latte::SQ_CHAN_Y },
   { 1, latte::SQ_CHAN_Z },
};

static const IndexMapEntry IndexMapQuadTessAdaptive[] = {
   { 6, latte::SQ_CHAN_X },
   { 6, latte::SQ_CHAN_Y },
   { 6, latte::SQ_CHAN_Z },
   { 6, latte::SQ_CHAN_W },
};

uint32_t
GX2FetchInstsPerAttrib(GX2FetchShaderType type)
{
   switch (type) {
   case GX2FetchShaderType::NoTessellation:
      return 1;
   case GX2FetchShaderType::LineTessellation:
      return 2;
   case GX2FetchShaderType::TriangleTessellation:
      return 3;
   case GX2FetchShaderType::QuadTessellation:
      return 4;
   default:
      throw std::logic_error("Unexpected fetch shader type.");
   }
}

uint32_t
GX2FSCalcNumFetchInsts(uint32_t attribs, GX2FetchShaderType type)
{
   switch (type) {
   case GX2FetchShaderType::NoTessellation:
      return attribs;
   case GX2FetchShaderType::LineTessellation:
   case GX2FetchShaderType::TriangleTessellation:
   case GX2FetchShaderType::QuadTessellation:
      return GX2FetchInstsPerAttrib(type) * (attribs - 2);
   default:
      throw std::logic_error("Unexpected fetch shader type.");
   }
}

uint32_t
GX2FSCalcNumTessALUInsts(GX2FetchShaderType type, GX2TessellationMode mode)
{
   if (mode == GX2TessellationMode::Adaptive) {
      switch (type) {
      case GX2FetchShaderType::NoTessellation:
         break;
      case GX2FetchShaderType::LineTessellation:
         return 11;
      case GX2FetchShaderType::TriangleTessellation:
         return 57;
      case GX2FetchShaderType::QuadTessellation:
         return 43;
      }
   }

   return 4;
}

uint32_t
GX2FSCalcNumAluInsts(GX2FetchShaderType type, GX2TessellationMode mode)
{
   if (type == GX2FetchShaderType::NoTessellation) {
      return 0;
   } else {
      return GX2FSCalcNumTessALUInsts(type, mode) + 4;
   }
}

uint32_t
GX2FSCalcNumFetchCFInsts(uint32_t fetches)
{
   return (fetches + (FetchesPerControlFlow - 1)) / FetchesPerControlFlow;
}

uint32_t
GX2FSCalcNumCFInsts(uint32_t fetches, GX2FetchShaderType type)
{
   auto count = GX2FSCalcNumFetchCFInsts(fetches);

   if (type != GX2FetchShaderType::NoTessellation) {
      count += 2;
   }

   return count + 1;
}

const IndexMapEntry *
GX2FSGetIndexGprMap(GX2FetchShaderType type,
                    GX2TessellationMode mode)
{
   switch (type) {
   case GX2FetchShaderType::LineTessellation:
      if (mode == GX2TessellationMode::Adaptive) {
         return IndexMapLineTessAdaptive;
      } else {
         return IndexMapLineTess;
      }
   case GX2FetchShaderType::TriangleTessellation:
      if (mode == GX2TessellationMode::Adaptive) {
         return IndexMapTriTessAdaptive;
      } else {
         return IndexMapTriTess;
      }
   case GX2FetchShaderType::QuadTessellation:
      if (mode == GX2TessellationMode::Adaptive) {
         return IndexMapQuadTessAdaptive;
      } else {
         return IndexMapQuadTess;
      }
   case GX2FetchShaderType::NoTessellation:
   default:
      return IndexMapNoTess;
   }
}

uint32_t
GX2CalcFetchShaderSizeEx(uint32_t attribs,
                         GX2FetchShaderType type,
                         GX2TessellationMode mode)
{
   auto fetch = GX2FSCalcNumFetchInsts(attribs, type);
   auto aluBytes = sizeof(latte::AluInst) * GX2FSCalcNumAluInsts(type, mode);
   auto cfBytes = sizeof(latte::ControlFlowInst) * GX2FSCalcNumCFInsts(fetch, type);

   return static_cast<uint32_t>(sizeof(latte::VertexFetchInst) * fetch + align_up(cfBytes + aluBytes, 16));
}

void
GX2InitFetchShaderEx(GX2FetchShader *fetchShader,
                     uint8_t *buffer,
                     uint32_t attribCount,
                     GX2AttribStream *attribs,
                     GX2FetchShaderType type,
                     GX2TessellationMode tessMode)
{
   if (type != GX2FetchShaderType::NoTessellation) {
      throw std::logic_error("Unsupported fetch shader type.");
   }

   if (tessMode != GX2TessellationMode::Discrete) {
      throw std::logic_error("Unsupported fetch shader tessellation mode.");
   }

   auto someTessVar1 = 128u;
   auto someTessVar2 = 128u;
   auto numGPRs = 0u;
   auto barrier = false;

   // Calculate instruction pointers
   auto fetchCount = GX2FSCalcNumFetchInsts(attribCount, type);
   auto aluCount = 0; // GX2FSCalcNumAluInsts(type, tessMode);
   auto cfCount = GX2FSCalcNumCFInsts(fetchCount, type);

   auto fetchSize = fetchCount * sizeof(latte::VertexFetchInst);
   auto cfSize = cfCount * sizeof(latte::ControlFlowInst);
   auto aluSize = aluCount * sizeof(latte::AluInst);

   auto cfOffset = 0u;
   auto aluOffset = cfSize;
   auto fetchOffset = align_up(cfSize + aluSize, 0x10u);

   auto cfPtr = reinterpret_cast<latte::ControlFlowInst *>(buffer + cfOffset);
   auto aluPtr = reinterpret_cast<latte::AluInst *>(buffer + aluOffset);
   auto fetchPtr = reinterpret_cast<latte::VertexFetchInst *>(buffer + fetchOffset);

   // Setup fetch shader
   fetchShader->type = type;
   fetchShader->attribCount = attribCount;
   fetchShader->data = buffer;
   fetchShader->size = GX2CalcFetchShaderSizeEx(attribCount, type, tessMode);

   // Generate fetch instructions
   auto indexMap = GX2FSGetIndexGprMap(type, tessMode);

   for (auto i = 0u; i < attribCount; ++i) {
      latte::VertexFetchInst vfetch = { 0, 0, 0, 0 };
      auto &attrib = attribs[i];

      if (attrib.buffer == 16) {
         // TODO: Figure out what these vars are for
         if (attrib.offset) {
            if (attrib.offset == 1) {
               someTessVar1 = attrib.location;
            }
         } else {
            someTessVar2 = attrib.location;
         }
      } else {
         // Semantic vertex fetch
         vfetch.word0.VTX_INST = latte::SQ_TEX_INST_VTX_SEMANTIC;

         // Setup source
         vfetch.word0.BUFFER_ID = attrib.buffer;
         vfetch.word2.OFFSET = attribs[i].offset;

         if (attrib.type) {
            if (attrib.type == GX2AttribIndexType::PerInstance) {
               if (attrib.aluDivisor == 1) {
                  vfetch.word0.FETCH_TYPE = latte::SQ_VTX_FETCH_TYPE::SQ_VTX_FETCH_INSTANCE_DATA;
                  vfetch.word0.SRC_SEL_X = latte::SQ_SEL_W;
               } else if (attrib.aluDivisor == fetchShader->divisors[0]) {
                  vfetch.word0.FETCH_TYPE = latte::SQ_VTX_FETCH_TYPE::SQ_VTX_FETCH_INSTANCE_DATA;
                  vfetch.word0.SRC_SEL_X = latte::SQ_SEL_Y;
               } else if (attrib.aluDivisor == fetchShader->divisors[1]) {
                  vfetch.word0.FETCH_TYPE = latte::SQ_VTX_FETCH_TYPE::SQ_VTX_FETCH_INSTANCE_DATA;
                  vfetch.word0.SRC_SEL_X = latte::SQ_SEL_Z;
               } else {
                  fetchShader->divisors[fetchShader->numDivisors] = attrib.aluDivisor;

                  if (fetchShader->numDivisors == 0) {
                     vfetch.word0.SRC_SEL_X = latte::SQ_SEL_Y;
                  } else if (fetchShader->numDivisors == 1) {
                     vfetch.word0.SRC_SEL_X = latte::SQ_SEL_Z;
                  }

                  fetchShader->numDivisors++;
               }
            }
         } else {
            vfetch.word0.SRC_GPR = indexMap[0].gpr;
            vfetch.word0.SRC_SEL_X = static_cast<latte::SQ_SEL>(indexMap[0].chan);
         }

         // Setup dest
         vfetch.gpr.DST_GPR = attrib.location;
         vfetch.word1.DST_SEL_W = static_cast<latte::SQ_SEL>(attrib.mask & 0x7);
         vfetch.word1.DST_SEL_Z = static_cast<latte::SQ_SEL>((attrib.mask >> 8) & 0x7);
         vfetch.word1.DST_SEL_Y = static_cast<latte::SQ_SEL>((attrib.mask >> 16) & 0x7);
         vfetch.word1.DST_SEL_X = static_cast<latte::SQ_SEL>((attrib.mask >> 24) & 0x7);

         // Setup format
         vfetch.word2.MEGA_FETCH = 1;
         vfetch.word0.MEGA_FETCH_COUNT = GX2GetAttribFormatBytes(attrib.format) - 1;
         vfetch.word1.DATA_FORMAT = GX2GetAttribFormatDataFormat(attrib.format);

         if (attribs[i].format & GX2AttribFormatFlags::SCALED) {
            vfetch.word1.NUM_FORMAT_ALL = latte::SQ_NUM_FORMAT_SCALED;
         } else if (attribs[i].format & GX2AttribFormatFlags::INTEGER) {
            vfetch.word1.NUM_FORMAT_ALL = latte::SQ_NUM_FORMAT_INT;
         } else if (attribs[i].format) {
            vfetch.word1.NUM_FORMAT_ALL = latte::SQ_NUM_FORMAT_NORM;
         }

         if (attribs[i].format & GX2AttribFormatFlags::SIGNED) {
            vfetch.word1.FORMAT_COMP_ALL = latte::SQ_FORMAT_COMP_SIGNED;
         }

         if (attribs[i].endianSwap == latte::SQ_ENDIAN_AUTO) {
            vfetch.word2.ENDIAN_SWAP = static_cast<latte::SQ_ENDIAN>(GX2GetAttribFormatSwapMode(attribs[i].format));
         } else {
            vfetch.word2.ENDIAN_SWAP = static_cast<latte::SQ_ENDIAN>(attribs[i].endianSwap & 3);
         }

         // Append to program
         *(fetchPtr++) = vfetch;

         // Add extra tesselation vertex fetches
         if (type != GX2FetchShaderType::NoTessellation && attrib.type != GX2AttribIndexType::PerInstance) {
            auto perAttrib = GX2FetchInstsPerAttrib(type);

            for (auto j = 1u; j < perAttrib; ++j) {
               latte::VertexFetchInst vfetch2 = vfetch;

               // Update src/dst
               vfetch2.word0.SRC_GPR = indexMap[j].gpr;
               vfetch2.word0.SRC_SEL_X = static_cast<latte::SQ_SEL>(indexMap[j].chan);
               vfetch2.gpr.DST_GPR = j + attrib.location;

               // Append to program
               *(fetchPtr++) = vfetch;
            }
         }
      }
   }

   // Generate tessellation ALU ops
   if (type != GX2FetchShaderType::NoTessellation) {
      numGPRs = 2;

      if (tessMode == GX2TessellationMode::Adaptive) {
         switch (type) {
         case GX2FetchShaderType::NoTessellation:
            break;
         case GX2FetchShaderType::LineTessellation:
            numGPRs = 3;
            break;
         case GX2FetchShaderType::TriangleTessellation:
            numGPRs = 7;
            break;
         case GX2FetchShaderType::QuadTessellation:
            numGPRs = 7;
            break;
         }
      }

      // TODO: GX2FSGenTessAluOps
      barrier = true;
   }

   // Generate a VTX CF per 16 VFETCH
   if (fetchCount) {
      for (auto i = 0u; i < cfCount - 1; ++i) {
         auto fetches = FetchesPerControlFlow;

         if (fetchCount < (i + 1) * FetchesPerControlFlow) {
            // Don't overrun our fetches!
            fetches = fetchCount % FetchesPerControlFlow;
         }

         latte::ControlFlowInst inst = { 0, 0 };
         inst.word0.ADDR = static_cast<uint32_t>((fetchOffset + sizeof(latte::VertexFetchInst) * i * FetchesPerControlFlow) / 8);
         inst.word1.COUNT = fetches & 0x7;
         inst.word1.COUNT_3 = (fetches >> 3) & 0x1;
         inst.word1.CF_INST = latte::SQ_CF_INST_VTX_TC;
         inst.word1.BARRIER = barrier ? 1 : 0;
         *(cfPtr++) = inst;
      }
   }

   // Generate tessellation "post" ALU ops
   if (numGPRs) {
      // TODO: GX2FSGenPostAluOps
   }

   // Generate an EOP
   latte::ControlFlowInst eop = { 0, 0 };
   eop.word1.BARRIER = 1;
   eop.word1.CF_INST = latte::SQ_CF_INST_RETURN;
   *(cfPtr++) = eop;

   // Set sq_pgm_resources_fs
   auto sq_pgm_resources_fs = fetchShader->regs.sq_pgm_resources_fs.value();
   sq_pgm_resources_fs.NUM_GPRS = numGPRs;
   fetchShader->regs.sq_pgm_resources_fs = sq_pgm_resources_fs;
}

} // namespace gx2
