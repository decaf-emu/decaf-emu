#include "gx2.h"
#include "gx2_fetchshader.h"
#include "gx2_shaders.h"
#include "gx2_enum.h"
#include "gx2_enum_string.h"
#include "gx2_format.h"
#include "gx2_memory.h"

#include <common/align.h>
#include <common/decaf_assert.h>
#include <fmt/format.h>
#include <libgpu/latte/latte_instructions.h>

namespace cafe::gx2
{

struct IndexMapEntry
{
   uint32_t gpr;
   latte::SQ_CHAN chan;
};

static const auto
FetchesPerControlFlow = 16u;

static const IndexMapEntry IndexMapNoTess[] = {
   { 0, latte::SQ_CHAN::X },
   { 0, latte::SQ_CHAN::X },
   { 0, latte::SQ_CHAN::X },
   { 0, latte::SQ_CHAN::X },
};

static const IndexMapEntry IndexMapLineTess[] = {
   { 0, latte::SQ_CHAN::Y },
   { 0, latte::SQ_CHAN::Z },
   { 0, latte::SQ_CHAN::Y },
   { 0, latte::SQ_CHAN::Y },
};

static const IndexMapEntry IndexMapLineTessAdaptive[] = {
   { 6, latte::SQ_CHAN::X },
   { 6, latte::SQ_CHAN::Y },
   { 6, latte::SQ_CHAN::Z },
   { 6, latte::SQ_CHAN::X },
};

static const IndexMapEntry IndexMapTriTess[] = {
   { 1, latte::SQ_CHAN::X },
   { 1, latte::SQ_CHAN::Y },
   { 1, latte::SQ_CHAN::Z },
   { 1, latte::SQ_CHAN::X },
};

static const IndexMapEntry IndexMapTriTessAdaptive[] = {
   { 6, latte::SQ_CHAN::X },
   { 6, latte::SQ_CHAN::Y },
   { 6, latte::SQ_CHAN::Z },
   { 6, latte::SQ_CHAN::X },
};

static const IndexMapEntry IndexMapQuadTess[] = {
   { 0, latte::SQ_CHAN::Z },
   { 1, latte::SQ_CHAN::X },
   { 1, latte::SQ_CHAN::Y },
   { 1, latte::SQ_CHAN::Z },
};

static const IndexMapEntry IndexMapQuadTessAdaptive[] = {
   { 6, latte::SQ_CHAN::X },
   { 6, latte::SQ_CHAN::Y },
   { 6, latte::SQ_CHAN::Z },
   { 6, latte::SQ_CHAN::W },
};

static uint32_t
fetchInstsPerAttrib(GX2FetchShaderType type)
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
      decaf_abort(fmt::format("Invalid GX2FetchShaderType {}", to_string(type)));
   }
}

static uint32_t
calcNumFetchInsts(uint32_t attribs,
                  GX2FetchShaderType type)
{
   switch (type) {
   case GX2FetchShaderType::NoTessellation:
      return attribs;
   case GX2FetchShaderType::LineTessellation:
   case GX2FetchShaderType::TriangleTessellation:
   case GX2FetchShaderType::QuadTessellation:
      return fetchInstsPerAttrib(type) * (attribs - 2);
   default:
      decaf_abort(fmt::format("Invalid GX2FetchShaderType {}", to_string(type)));
   }
}

static uint32_t
calcNumTessALUInsts(GX2FetchShaderType type,
                    GX2TessellationMode mode)
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

static uint32_t
calcNumAluInsts(GX2FetchShaderType type,
                GX2TessellationMode mode)
{
   if (type == GX2FetchShaderType::NoTessellation) {
      return 0;
   } else {
      return calcNumTessALUInsts(type, mode) + 4;
   }
}

static uint32_t
calcNumFetchCFInsts(uint32_t fetches)
{
   return (fetches + (FetchesPerControlFlow - 1)) / FetchesPerControlFlow;
}

static uint32_t
calcNumCFInsts(uint32_t fetches,
               GX2FetchShaderType type)
{
   auto count = calcNumFetchCFInsts(fetches);

   if (type != GX2FetchShaderType::NoTessellation) {
      count += 2;
   }

   return count + 1;
}

static const IndexMapEntry *
getIndexGprMap(GX2FetchShaderType type,
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
   auto fetch = calcNumFetchInsts(attribs, type);
   auto aluBytes = sizeof(latte::AluInst) * calcNumAluInsts(type, mode);
   auto cfBytes = sizeof(latte::ControlFlowInst) * calcNumCFInsts(fetch, type);

   return static_cast<uint32_t>(sizeof(latte::VertexFetchInst) * fetch + align_up(cfBytes + aluBytes, 16));
}

void
GX2InitFetchShaderEx(virt_ptr<GX2FetchShader> fetchShader,
                     virt_ptr<uint8_t> buffer,
                     uint32_t attribCount,
                     virt_ptr<GX2AttribStream> attribs,
                     GX2FetchShaderType type,
                     GX2TessellationMode tessMode)
{
   if (type != GX2FetchShaderType::NoTessellation) {
      decaf_abort(fmt::format("Invalid GX2FetchShaderType {}", to_string(type)));
   }

   if (tessMode != GX2TessellationMode::Discrete) {
      decaf_abort(fmt::format("Invalid GX2TessellationMode {}", to_string(tessMode)));
   }

   auto someTessVar1 = 128u;
   auto someTessVar2 = 128u;
   auto numGPRs = 0u;
   auto barrier = false;

   // Calculate instruction pointers
   auto fetchCount = calcNumFetchInsts(attribCount, type);
   auto aluCount = 0; // calcNumAluInsts(type, tessMode);
   auto cfCount = calcNumCFInsts(fetchCount, type);

   auto fetchSize = fetchCount * sizeof(latte::VertexFetchInst);
   auto cfSize = cfCount * sizeof(latte::ControlFlowInst);
   auto aluSize = aluCount * sizeof(latte::AluInst);

   auto cfOffset = 0u;
   auto aluOffset = cfSize;
   auto fetchOffset = align_up(cfSize + aluSize, 0x10u);

   auto cfPtr = virt_cast<latte::ControlFlowInst *>(buffer + cfOffset);
   auto aluPtr = virt_cast<latte::AluInst *>(buffer + aluOffset);
   auto fetchPtr = virt_cast<latte::VertexFetchInst *>(buffer + fetchOffset);

   // Setup fetch shader
   fetchShader->type = type;
   fetchShader->attribCount = attribCount;
   fetchShader->data = buffer;
   fetchShader->size = GX2CalcFetchShaderSizeEx(attribCount, type, tessMode);

   // Generate fetch instructions
   auto indexMap = getIndexGprMap(type, tessMode);

   for (auto i = 0u; i < attribCount; ++i) {
      latte::VertexFetchInst vfetch;
      auto &attrib = attribs[i];
      std::memset(&vfetch, 0, sizeof(vfetch));

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
         vfetch.word0 = vfetch.word0
            .VTX_INST(latte::SQ_VTX_INST_SEMANTIC)
            .BUFFER_ID(latte::SQ_RES_OFFSET::VS_ATTRIB_RESOURCE_0 + attrib.buffer - latte::SQ_RES_OFFSET::VS_TEX_RESOURCE_0);

         vfetch.word2 = vfetch.word2
            .OFFSET(attribs[i].offset);

         if (attrib.type) {
            auto selX = latte::SQ_SEL::SEL_X;
            auto fetchType = latte::SQ_VTX_FETCH_TYPE::VERTEX_DATA;

            if (attrib.type == GX2AttribIndexType::PerInstance) {
               if (attrib.aluDivisor == 1) {
                  fetchType = latte::SQ_VTX_FETCH_TYPE::INSTANCE_DATA;
                  selX = latte::SQ_SEL::SEL_W;
               } else if (attrib.aluDivisor == fetchShader->divisors[0]) {
                  fetchType = latte::SQ_VTX_FETCH_TYPE::INSTANCE_DATA;
                  selX = latte::SQ_SEL::SEL_Y;
               } else if (attrib.aluDivisor == fetchShader->divisors[1]) {
                  fetchType = latte::SQ_VTX_FETCH_TYPE::INSTANCE_DATA;
                  selX = latte::SQ_SEL::SEL_Z;
               } else {
                  fetchShader->divisors[fetchShader->numDivisors] = attrib.aluDivisor;

                  if (fetchShader->numDivisors == 0) {
                     selX = latte::SQ_SEL::SEL_Y;
                  } else if (fetchShader->numDivisors == 1) {
                     selX = latte::SQ_SEL::SEL_Z;
                  }

                  fetchShader->numDivisors++;
               }
            }

            vfetch.word0 = vfetch.word0
               .FETCH_TYPE(fetchType)
               .SRC_SEL_X(selX);
         } else {
            vfetch.word0 = vfetch.word0
               .SRC_GPR(indexMap[0].gpr)
               .SRC_SEL_X(static_cast<latte::SQ_SEL>(indexMap[0].chan));
         }

         // Setup dest
         vfetch.gpr = vfetch.gpr
            .DST_GPR(attrib.location);

         vfetch.word1 = vfetch.word1
            .DST_SEL_W(static_cast<latte::SQ_SEL>(attrib.mask & 0x7))
            .DST_SEL_Z(static_cast<latte::SQ_SEL>((attrib.mask >> 8) & 0x7))
            .DST_SEL_Y(static_cast<latte::SQ_SEL>((attrib.mask >> 16) & 0x7))
            .DST_SEL_X(static_cast<latte::SQ_SEL>((attrib.mask >> 24) & 0x7));

         // Setup mega fetch
         vfetch.word2 = vfetch.word2
            .MEGA_FETCH(1);

         vfetch.word0 = vfetch.word0
            .MEGA_FETCH_COUNT(internal::getAttribFormatBytes(attrib.format) - 1);

         // Setup format
         auto dataFormat = internal::getAttribFormatDataFormat(attrib.format);
         auto numFormat = latte::SQ_NUM_FORMAT::NORM;
         auto formatComp = latte::SQ_FORMAT_COMP::UNSIGNED;

         if (attribs[i].format & GX2AttribFormatFlags::SCALED) {
            numFormat = latte::SQ_NUM_FORMAT::SCALED;
         } else if (attribs[i].format & GX2AttribFormatFlags::INTEGER) {
            numFormat = latte::SQ_NUM_FORMAT::INT;
         }

         if (attribs[i].format & GX2AttribFormatFlags::SIGNED) {
            formatComp = latte::SQ_FORMAT_COMP::SIGNED;
         }

         vfetch.word1 = vfetch.word1
            .DATA_FORMAT(dataFormat)
            .NUM_FORMAT_ALL(numFormat)
            .FORMAT_COMP_ALL(formatComp);

         auto swapMode = internal::getSwapModeEndian(static_cast<GX2EndianSwapMode>(attribs[i].endianSwap & 3));

         if (attribs[i].endianSwap == latte::SQ_ENDIAN::AUTO) {
            swapMode = internal::getAttribFormatEndian(attribs[i].format);
         }

         vfetch.word2 = vfetch.word2
            .ENDIAN_SWAP(swapMode);

         // Append to program
         *(fetchPtr++) = vfetch;

         // Add extra tesselation vertex fetches
         if (type != GX2FetchShaderType::NoTessellation && attrib.type != GX2AttribIndexType::PerInstance) {
            auto perAttrib = fetchInstsPerAttrib(type);

            for (auto j = 1u; j < perAttrib; ++j) {
               latte::VertexFetchInst vfetch2 = vfetch;

               // Update src/dst
               vfetch2.word0 = vfetch2.word0
                  .SRC_GPR(indexMap[j].gpr)
                  .SRC_SEL_X(static_cast<latte::SQ_SEL>(indexMap[j].chan));

               vfetch2.gpr = vfetch2.gpr
                  .DST_GPR(j + attrib.location);

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

         latte::ControlFlowInst inst;
         std::memset(&inst, 0, sizeof(inst));
         inst.word0 = inst.word0
            .ADDR(static_cast<uint32_t>((fetchOffset + sizeof(latte::VertexFetchInst) * i * FetchesPerControlFlow) / 8));
         inst.word1 = inst.word1
            .COUNT((fetches - 1) & 0x7)
            .COUNT_3(((fetches - 1) >> 3) & 0x1)
            .CF_INST(latte::SQ_CF_INST_VTX_TC)
            .BARRIER(barrier ? 1 : 0);
         *(cfPtr++) = inst;
      }
   }

   // Generate tessellation "post" ALU ops
   if (numGPRs) {
      // TODO: GX2FSGenPostAluOps
   }

   // Generate an EOP
   latte::ControlFlowInst eop;
   std::memset(&eop, 0, sizeof(eop));
   eop.word1 = eop.word1
      .BARRIER(1)
      .CF_INST(latte::SQ_CF_INST_RETURN);
   *(cfPtr++) = eop;

   // Set sq_pgm_resources_fs
   auto sq_pgm_resources_fs = fetchShader->regs.sq_pgm_resources_fs.value();
   sq_pgm_resources_fs = sq_pgm_resources_fs
      .NUM_GPRS(numGPRs);
   fetchShader->regs.sq_pgm_resources_fs = sq_pgm_resources_fs;

   GX2Invalidate(GX2InvalidateMode::CPU, fetchShader->data, fetchShader->size);
}

void
Library::registerFetchShadersSymbols()
{
   RegisterFunctionExport(GX2CalcFetchShaderSizeEx);
   RegisterFunctionExport(GX2InitFetchShaderEx);
}

} // namespace cafe::gx2
