#include "gx2_shaders.h"
#include "gx2_enum.h"
#include "gx2_format.h"
#include "utils/align.h"
#include "gpu/latte_instructions.h"
#include "gpu/latte_opcodes.h"

struct IndexMapEntry
{
   uint32_t gpr;
   latte::SQ_CHAN chan;
};

static uint32_t
GX2FetchInstsPerAttrib(GX2FetchShaderType::Value type);

static uint32_t
GX2FSCalcNumFetchInsts(uint32_t attribs, GX2FetchShaderType::Value type);

static uint32_t
GX2FSCalcNumTessALUInsts(GX2FetchShaderType::Value type, GX2TessellationMode::Value mode);

static uint32_t
GX2FSCalcNumAluInsts(GX2FetchShaderType::Value type, GX2TessellationMode::Value mode);

static uint32_t
GX2FSCalcNumFetchCFInsts(uint32_t fetches);

static uint32_t
GX2FSCalcNumCFInsts(uint32_t fetches, GX2FetchShaderType::Value type);

static const IndexMapEntry *
GX2FSGetIndexGprMap(GX2FetchShaderType::Value type,
                    GX2TessellationMode::Value mode);

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
GX2FetchInstsPerAttrib(GX2FetchShaderType::Value type)
{
   switch (type) {
   case GX2FetchShaderType::NoTesselation:
      return 1;
   case GX2FetchShaderType::LineTesselation:
      return 2;
   case GX2FetchShaderType::TriangleTesselation:
      return 3;
   case GX2FetchShaderType::QuadTesselation:
      return 4;
   default:
      throw std::logic_error("Unexpected fetch shader type.");
   }
}

uint32_t
GX2FSCalcNumFetchInsts(uint32_t attribs, GX2FetchShaderType::Value type)
{
   switch (type) {
   case GX2FetchShaderType::NoTesselation:
      return attribs;
   case GX2FetchShaderType::LineTesselation:
   case GX2FetchShaderType::TriangleTesselation:
   case GX2FetchShaderType::QuadTesselation:
      return GX2FetchInstsPerAttrib(type) * (attribs - 2);
   default:
      throw std::logic_error("Unexpected fetch shader type.");
   }
}

uint32_t
GX2FSCalcNumTessALUInsts(GX2FetchShaderType::Value type, GX2TessellationMode::Value mode)
{
   if (mode == GX2TessellationMode::Adaptive) {
      switch (type) {
      case GX2FetchShaderType::LineTesselation:
         return 11;
      case GX2FetchShaderType::TriangleTesselation:
         return 57;
      case GX2FetchShaderType::QuadTesselation:
         return 43;
      }
   }

   return 4;
}

uint32_t
GX2FSCalcNumAluInsts(GX2FetchShaderType::Value type, GX2TessellationMode::Value mode)
{
   if (type == GX2FetchShaderType::NoTesselation) {
      return 0;
   } else {
      return GX2FSCalcNumTessALUInsts(type, mode) + 4;
   }
}

uint32_t
GX2FSCalcNumFetchCFInsts(uint32_t fetches)
{
   return (fetches + (latte::BytesPerVTX - 1)) / latte::BytesPerVTX;
}

uint32_t
GX2FSCalcNumCFInsts(uint32_t fetches, GX2FetchShaderType::Value type)
{
   auto count = GX2FSCalcNumFetchCFInsts(fetches);

   if (type != GX2FetchShaderType::NoTesselation) {
      count += 2;
   }

   return count + 1;
}

const IndexMapEntry *
GX2FSGetIndexGprMap(GX2FetchShaderType::Value type,
                    GX2TessellationMode::Value mode)
{
   switch (type) {
   case GX2FetchShaderType::LineTesselation:
      if (mode == GX2TessellationMode::Adaptive) {
         return IndexMapLineTessAdaptive;
      } else {
         return IndexMapLineTess;
      }
   case GX2FetchShaderType::TriangleTesselation:
      if (mode == GX2TessellationMode::Adaptive) {
         return IndexMapTriTessAdaptive;
      } else {
         return IndexMapTriTess;
      }
   case GX2FetchShaderType::QuadTesselation:
      if (mode == GX2TessellationMode::Adaptive) {
         return IndexMapQuadTessAdaptive;
      } else {
         return IndexMapQuadTess;
      }
   case GX2FetchShaderType::NoTesselation:
   default:
      return IndexMapNoTess;
   }
}

uint32_t
GX2CalcFetchShaderSizeEx(uint32_t attribs,
                         GX2FetchShaderType::Value type,
                         GX2TessellationMode::Value mode)
{
   auto fetch = GX2FSCalcNumFetchInsts(attribs, type);
   auto aluBytes = latte::BytesPerALU * GX2FSCalcNumAluInsts(type, mode);
   auto cfBytes = latte::BytesPerCF * GX2FSCalcNumCFInsts(fetch, type);

   return latte::BytesPerVTX * fetch + align_up(cfBytes + aluBytes, 16);
}

void
GX2InitFetchShaderEx(GX2FetchShader *fetchShader,
                     void *buffer,
                     uint32_t attribCount,
                     GX2AttribStream *attribs,
                     GX2FetchShaderType::Value type,
                     GX2TessellationMode::Value tessMode)
{
   auto indexMap = GX2FSGetIndexGprMap(type, tessMode);
   auto someTessVar1 = 128u;
   auto someTessVar2 = 128u;
   auto vfetches = reinterpret_cast<latte::VertexFetchInst *>(buffer);

   fetchShader->type = type;
   fetchShader->attribCount = attribCount;
   fetchShader->data = buffer;
   fetchShader->size = GX2CalcFetchShaderSizeEx(attribCount, type, tessMode);

   for (auto i = 0u; i < attribCount; ++i) {
      latte::VertexFetchInst vfetch = { 0, 0, 0, 0 };
      auto &attrib = attribs[i];

      if (attrib.buffer == 16) {
         // TODO: Figure out what this is for
         if (attrib.offset) {
            if (attrib.offset == 1) {
               someTessVar1 = attrib.location;
            }
         } else {
            someTessVar2 = attrib.location;
         }
      } else {
         vfetch.word0.VTX_INST = latte::SQ_VTX_INST_SEMANTIC;
         vfetch.word0.BUFFER_ID = attrib.buffer - 96; // TODO: Why -96?? Is this correct??

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

         vfetch.word0.MEGA_FETCH_COUNT = GX2GetAttribFormatBytes(attrib.format) - 1;

         vfetch.word1.DATA_FORMAT = GX2GetAttribFormatDataFormat(attrib.format);
         vfetch.word1.DST_SEL_W = static_cast<latte::SQ_SEL>(attrib.mask & 0x7);
         vfetch.word1.DST_SEL_Z = static_cast<latte::SQ_SEL>((attrib.mask >> 8) & 0x7);
         vfetch.word1.DST_SEL_Y = static_cast<latte::SQ_SEL>((attrib.mask >> 16) & 0x7);
         vfetch.word1.DST_SEL_X = static_cast<latte::SQ_SEL>((attrib.mask >> 24) & 0x7);

         if (attribs[i].format & GX2AttribFormatFlags::SCALED) {
            vfetch.word1.NUM_FORMAT_ALL = latte::SQ_NUM_FORMAT_SCALED;
         } else if (attribs[i].format & GX2AttribFormatFlags::INTEGER) {
            vfetch.word1.NUM_FORMAT_ALL = latte::SQ_NUM_FORMAT_INT;
         } else if (attribs[i].format) {
            vfetch.word1.NUM_FORMAT_ALL = latte::SQ_NUM_FORMAT_NORM;
         }

         vfetch.gpr.DST_GPR = attrib.location;

         if (attribs[i].format & GX2AttribFormatFlags::SIGNED) {
            vfetch.word1.FORMAT_COMP_ALL = latte::SQ_FORMAT_COMP_SIGNED;
         }

         if (attribs[i].endianSwap == latte::SQ_ENDIAN_AUTO) {
            vfetch.word2.ENDIAN_SWAP = static_cast<latte::SQ_ENDIAN>(GX2GetAttribFormatSwapMode(attribs[i].format));
         } else {
            vfetch.word2.ENDIAN_SWAP = static_cast<latte::SQ_ENDIAN>(attribs[i].endianSwap & 3);
         }

         vfetch.word2.OFFSET = attribs[i].offset;
         vfetch.word2.MEGA_FETCH = 1;

         // Append to program
         *(vfetches++) = vfetch;

         if (type != GX2FetchShaderType::NoTesselation && attrib.type != GX2AttribIndexType::PerInstance) {
            auto perAttrib = GX2FetchInstsPerAttrib(type);

            for (auto j = 1u; j < perAttrib; ++j) {
               latte::VertexFetchInst vfetch2 = vfetch;

               // Update src/dst
               vfetch2.word0.SRC_GPR = indexMap[j].gpr;
               vfetch2.word0.SRC_SEL_X = static_cast<latte::SQ_SEL>(indexMap[j].chan);
               vfetch2.gpr.DST_GPR = j + attrib.location;

               // Append to program
               *(vfetches++) = vfetch;
            }
         }
      }
   }

   // TODO: Tesellation stuff

   // TODO: Generate a VTX CF per 16 VFETCH

   // TODO: Append a EOP
}
