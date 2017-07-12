#include "gx2_internal_gfd.h"

namespace gx2
{

namespace internal
{

void
gfdToGX2Surface(const gfd::GFDSurface &src,
                GX2Surface *dst)
{
   dst->dim = src.dim;
   dst->width = src.width;
   dst->height = src.height;
   dst->depth = src.depth;
   dst->mipLevels = src.mipLevels;
   dst->format = src.format;
   dst->aa = src.aa;
   dst->use = src.use;
   dst->imageSize = src.image.size();
   dst->mipmapSize = src.mipmap.size();
   dst->tileMode = src.tileMode;
   dst->swizzle = src.swizzle;
   dst->alignment = src.alignment;
   dst->pitch = src.pitch;

   for (auto i = 0u; i < src.mipLevelOffset.size(); ++i) {
      dst->mipLevelOffset[i] = src.mipLevelOffset[i];
   }
}

void
gx2ToGFDSurface(const GX2Surface *src,
                gfd::GFDSurface &dst)
{
   dst.dim = src->dim;
   dst.width = src->width;
   dst.height = src->height;
   dst.depth = src->depth;
   dst.mipLevels = src->mipLevels;
   dst.format = src->format;
   dst.aa = src->aa;
   dst.use = src->use;
   dst.image.resize(src->imageSize);
   std::memcpy(dst.image.data(), src->image, src->imageSize);
   dst.mipmap.resize(src->mipmapSize);
   std::memcpy(dst.mipmap.data(), src->mipmaps, src->mipmapSize);
   dst.tileMode = src->tileMode;
   dst.swizzle = src->swizzle;
   dst.alignment = src->alignment;
   dst.pitch = src->pitch;

   for (auto i = 0u; i < src->mipLevelOffset.size(); ++i) {
      dst.mipLevelOffset[i] = src->mipLevelOffset[i];
   }
}

void
gfdToGX2Texture(const gfd::GFDTexture &src,
                GX2Texture *dst)
{
   gfdToGX2Surface(src.surface, &dst->surface);

   dst->viewFirstMip = src.viewFirstMip;
   dst->viewNumMips = src.viewNumMips;
   dst->viewFirstSlice = src.viewFirstSlice;
   dst->viewNumSlices = src.viewNumSlices;
   dst->compMap = src.compMap;

   dst->regs.word0 = src.regs.word0;
   dst->regs.word1 = src.regs.word1;
   dst->regs.word4 = src.regs.word4;
   dst->regs.word5 = src.regs.word5;
   dst->regs.word6 = src.regs.word6;
}

void
gx2ToGFDTexture(const GX2Texture *src,
                gfd::GFDTexture &dst)
{
   gx2ToGFDSurface(&src->surface, dst.surface);

   dst.viewFirstMip = src->viewFirstMip;
   dst.viewNumMips = src->viewNumMips;
   dst.viewFirstSlice = src->viewFirstSlice;
   dst.viewNumSlices = src->viewNumSlices;
   dst.compMap = src->compMap;

   dst.regs.word0 = src->regs.word0;
   dst.regs.word1 = src->regs.word1;
   dst.regs.word4 = src->regs.word4;
   dst.regs.word5 = src->regs.word5;
   dst.regs.word6 = src->regs.word6;
}

void
gx2ToGFDVertexShader(const GX2VertexShader *src,
                     gfd::GFDVertexShader &dst)
{
   dst.regs.sq_pgm_resources_vs = src->regs.sq_pgm_resources_vs;
   dst.regs.vgt_primitiveid_en = src->regs.vgt_primitiveid_en;
   dst.regs.spi_vs_out_config = src->regs.spi_vs_out_config;
   dst.regs.num_spi_vs_out_id = src->regs.num_spi_vs_out_id;

   for (auto i = 0u; i < src->regs.spi_vs_out_id.size(); ++i) {
      dst.regs.spi_vs_out_id[i] = src->regs.spi_vs_out_id[i];
   }

   dst.regs.pa_cl_vs_out_cntl = src->regs.pa_cl_vs_out_cntl;
   dst.regs.sq_vtx_semantic_clear = src->regs.sq_vtx_semantic_clear;
   dst.regs.num_sq_vtx_semantic = src->regs.num_sq_vtx_semantic;

   for (auto i = 0u; i < src->regs.sq_vtx_semantic.size(); ++i) {
      dst.regs.sq_vtx_semantic[i] = src->regs.sq_vtx_semantic[i];
   }

   dst.regs.vgt_strmout_buffer_en = src->regs.vgt_strmout_buffer_en;
   dst.regs.vgt_vertex_reuse_block_cntl = src->regs.vgt_vertex_reuse_block_cntl;
   dst.regs.vgt_hos_reuse_depth = src->regs.vgt_hos_reuse_depth;
   dst.mode = src->mode;

   dst.uniformBlocks.resize(src->uniformBlockCount);
   for (auto i = 0u; i < src->uniformBlockCount; ++i) {
      dst.uniformBlocks[i].name = src->uniformBlocks[i].name;
      dst.uniformBlocks[i].offset = src->uniformBlocks[i].offset;
      dst.uniformBlocks[i].size = src->uniformBlocks[i].size;
   }

   dst.uniformVars.resize(src->uniformVarCount);
   for (auto i = 0u; i < src->uniformBlockCount; ++i) {
      dst.uniformVars[i].name = src->uniformVars[i].name;
      dst.uniformVars[i].type = src->uniformVars[i].type;
      dst.uniformVars[i].count = src->uniformVars[i].count;
      dst.uniformVars[i].offset = src->uniformVars[i].offset;
      dst.uniformVars[i].block = src->uniformVars[i].block;
   }

   dst.initialValues.resize(src->initialValueCount);
   for (auto i = 0u; i < src->initialValueCount; ++i) {
      for (auto j = 0u; j < dst.initialValues[i].value.size(); ++j) {
         dst.initialValues[i].value[j] = src->initialValues[i].value[j];
      }

      dst.initialValues[i].offset = src->initialValues[i].offset;
   }

   dst.loopVars.resize(src->loopVarCount);
   for (auto i = 0u; i < src->loopVarCount; ++i) {
      dst.loopVars[i].offset = src->loopVars[i].offset;
      dst.loopVars[i].value = src->loopVars[i].value;
   }

   dst.samplerVars.resize(src->samplerVarCount);
   for (auto i = 0u; i < src->samplerVarCount; ++i) {
      dst.samplerVars[i].name = src->samplerVars[i].name;
      dst.samplerVars[i].type = src->samplerVars[i].type;
      dst.samplerVars[i].location = src->samplerVars[i].location;
   }

   dst.attribVars.resize(src->attribVarCount);
   for (auto i = 0u; i < src->attribVarCount; ++i) {
      dst.attribVars[i].name = src->attribVars[i].name;
      dst.attribVars[i].type = src->attribVars[i].type;
      dst.attribVars[i].count = src->attribVars[i].count;
      dst.attribVars[i].location = src->attribVars[i].location;
   }

   dst.ringItemSize = src->ringItemsize;
   dst.hasStreamOut = src->hasStreamOut;

   for (auto i = 0u; i < dst.streamOutStride.size(); ++i) {
      dst.streamOutStride[i] = src->streamOutStride[i];
   }

   dst.gx2rData.elemCount = src->gx2rData.elemCount;
   dst.gx2rData.elemSize = src->gx2rData.elemSize;
   dst.gx2rData.flags = src->gx2rData.flags;

   if (src->gx2rData.buffer) {
      auto size = src->gx2rData.elemCount * src->gx2rData.elemSize;
      dst.gx2rData.buffer.resize(size);
      std::memcpy(dst.gx2rData.buffer.data(), src->gx2rData.buffer, size);
   }
}

void
gx2ToGFDPixelShader(const GX2PixelShader *src,
                    gfd::GFDPixelShader &dst)
{
   dst.regs.sq_pgm_resources_ps = src->regs.sq_pgm_resources_ps;
   dst.regs.sq_pgm_exports_ps = src->regs.sq_pgm_exports_ps;
   dst.regs.spi_ps_in_control_0 = src->regs.spi_ps_in_control_0;
   dst.regs.spi_ps_in_control_1 = src->regs.spi_ps_in_control_1;
   dst.regs.num_spi_ps_input_cntl = src->regs.num_spi_ps_input_cntl;

   for (auto i = 0u; i < src->regs.spi_ps_input_cntls.size(); ++i) {
      dst.regs.spi_ps_input_cntls[i] = src->regs.spi_ps_input_cntls[i];
   }

   dst.regs.cb_shader_mask = src->regs.cb_shader_mask;
   dst.regs.cb_shader_control = src->regs.cb_shader_control;
   dst.regs.db_shader_control = src->regs.db_shader_control;
   dst.regs.spi_input_z = src->regs.spi_input_z;

   dst.data.resize(src->size);
   std::memcpy(dst.data.data(), src->data, src->size);
   dst.mode = src->mode;

   dst.uniformBlocks.resize(src->uniformBlockCount);
   for (auto i = 0u; i < src->uniformBlockCount; ++i) {
      dst.uniformBlocks[i].name = src->uniformBlocks[i].name;
      dst.uniformBlocks[i].offset = src->uniformBlocks[i].offset;
      dst.uniformBlocks[i].size = src->uniformBlocks[i].size;
   }

   dst.uniformVars.resize(src->uniformVarCount);
   for (auto i = 0u; i < src->uniformBlockCount; ++i) {
      dst.uniformVars[i].name = src->uniformVars[i].name;
      dst.uniformVars[i].type = src->uniformVars[i].type;
      dst.uniformVars[i].count = src->uniformVars[i].count;
      dst.uniformVars[i].offset = src->uniformVars[i].offset;
      dst.uniformVars[i].block = src->uniformVars[i].block;
   }

   dst.initialValues.resize(src->initialValueCount);
   for (auto i = 0u; i < src->initialValueCount; ++i) {
      for (auto j = 0u; j < dst.initialValues[i].value.size(); ++j) {
         dst.initialValues[i].value[j] = src->initialValues[i].value[j];
      }

      dst.initialValues[i].offset = src->initialValues[i].offset;
   }

   dst.loopVars.resize(src->loopVarCount);
   for (auto i = 0u; i < src->loopVarCount; ++i) {
      dst.loopVars[i].offset = src->loopVars[i].offset;
      dst.loopVars[i].value = src->loopVars[i].value;
   }

   dst.samplerVars.resize(src->samplerVarCount);
   for (auto i = 0u; i < src->samplerVarCount; ++i) {
      dst.samplerVars[i].name = src->samplerVars[i].name;
      dst.samplerVars[i].type = src->samplerVars[i].type;
      dst.samplerVars[i].location = src->samplerVars[i].location;
   }

   dst.gx2rData.elemCount = src->gx2rData.elemCount;
   dst.gx2rData.elemSize = src->gx2rData.elemSize;
   dst.gx2rData.flags = src->gx2rData.flags;

   if (src->gx2rData.buffer) {
      auto size = src->gx2rData.elemCount * src->gx2rData.elemSize;
      dst.gx2rData.buffer.resize(size);
      std::memcpy(dst.gx2rData.buffer.data(), src->gx2rData.buffer, size);
   }
}

void
gx2ToGFDGeometryShader(const GX2GeometryShader *src,
                       gfd::GFDGeometryShader &dst)
{
   dst.regs.sq_pgm_resources_gs = src->regs.sq_pgm_resources_gs;
   dst.regs.vgt_gs_out_prim_type = src->regs.vgt_gs_out_prim_type;
   dst.regs.vgt_gs_mode = src->regs.vgt_gs_mode;
   dst.regs.pa_cl_vs_out_cntl = src->regs.pa_cl_vs_out_cntl;
   dst.regs.sq_pgm_resources_vs = src->regs.sq_pgm_resources_vs;
   dst.regs.sq_gs_vert_itemsize = src->regs.sq_gs_vert_itemsize;
   dst.regs.spi_vs_out_config = src->regs.spi_vs_out_config;
   dst.regs.num_spi_vs_out_id = src->regs.num_spi_vs_out_id;

   for (auto i = 0u; i < src->regs.spi_vs_out_id.size(); ++i) {
      dst.regs.spi_vs_out_id[i] = src->regs.spi_vs_out_id[i];
   }

   dst.regs.vgt_strmout_buffer_en = src->regs.vgt_strmout_buffer_en;

   dst.data.resize(src->size);
   std::memcpy(dst.data.data(), src->data, src->size);

   dst.vertexShaderData.resize(src->vertexShaderSize);
   std::memcpy(dst.vertexShaderData.data(), src->vertexShaderData, src->vertexShaderSize);

   dst.mode = src->mode;

   dst.uniformBlocks.resize(src->uniformBlockCount);
   for (auto i = 0u; i < src->uniformBlockCount; ++i) {
      dst.uniformBlocks[i].name = src->uniformBlocks[i].name;
      dst.uniformBlocks[i].offset = src->uniformBlocks[i].offset;
      dst.uniformBlocks[i].size = src->uniformBlocks[i].size;
   }

   dst.uniformVars.resize(src->uniformVarCount);
   for (auto i = 0u; i < src->uniformBlockCount; ++i) {
      dst.uniformVars[i].name = src->uniformVars[i].name;
      dst.uniformVars[i].type = src->uniformVars[i].type;
      dst.uniformVars[i].count = src->uniformVars[i].count;
      dst.uniformVars[i].offset = src->uniformVars[i].offset;
      dst.uniformVars[i].block = src->uniformVars[i].block;
   }

   dst.initialValues.resize(src->initialValueCount);
   for (auto i = 0u; i < src->initialValueCount; ++i) {
      for (auto j = 0u; j < dst.initialValues[i].value.size(); ++j) {
         dst.initialValues[i].value[j] = src->initialValues[i].value[j];
      }

      dst.initialValues[i].offset = src->initialValues[i].offset;
   }

   dst.loopVars.resize(src->loopVarCount);
   for (auto i = 0u; i < src->loopVarCount; ++i) {
      dst.loopVars[i].offset = src->loopVars[i].offset;
      dst.loopVars[i].value = src->loopVars[i].value;
   }

   dst.samplerVars.resize(src->samplerVarCount);
   for (auto i = 0u; i < src->samplerVarCount; ++i) {
      dst.samplerVars[i].name = src->samplerVars[i].name;
      dst.samplerVars[i].type = src->samplerVars[i].type;
      dst.samplerVars[i].location = src->samplerVars[i].location;
   }

   dst.ringItemSize = src->ringItemSize;
   dst.hasStreamOut = src->hasStreamOut;

   for (auto i = 0u; i < dst.streamOutStride.size(); ++i) {
      dst.streamOutStride[i] = src->streamOutStride[i];
   }

   dst.gx2rData.elemCount = src->gx2rData.elemCount;
   dst.gx2rData.elemSize = src->gx2rData.elemSize;
   dst.gx2rData.flags = src->gx2rData.flags;

   if (src->gx2rData.buffer) {
      auto size = src->gx2rData.elemCount * src->gx2rData.elemSize;
      dst.gx2rData.buffer.resize(size);
      std::memcpy(dst.gx2rData.buffer.data(), src->gx2rData.buffer, size);
   }

   dst.gx2rVertexShaderData.elemCount = src->gx2rVertexShaderData.elemCount;
   dst.gx2rVertexShaderData.elemSize = src->gx2rVertexShaderData.elemSize;
   dst.gx2rVertexShaderData.flags = src->gx2rVertexShaderData.flags;

   if (src->gx2rVertexShaderData.buffer) {
      auto size = src->gx2rVertexShaderData.elemCount * src->gx2rVertexShaderData.elemSize;
      dst.gx2rVertexShaderData.buffer.resize(size);
      std::memcpy(dst.gx2rVertexShaderData.buffer.data(), src->gx2rVertexShaderData.buffer, size);
   }
}

} // namespace internal

} // namespace gx2
