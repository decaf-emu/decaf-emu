#include "gx2_shaders.h"
#include "gpu/pm4_writer.h"

uint32_t
GX2CalcGeometryShaderInputRingBufferSize(uint32_t ringItemSize)
{
   return 0;
}

uint32_t
GX2CalcGeometryShaderOutputRingBufferSize(uint32_t ringItemSize)
{
   return 0;
}

uint32_t
GX2CalcFetchShaderSizeEx(uint32_t attribs,
                         GX2FetchShaderType::Value fetchShaderType,
                         GX2TessellationMode::Value tesellationMode)
{
   return 0;
}

void
GX2InitFetchShaderEx(GX2FetchShader *fetchShader,
                     void *buffer,
                     uint32_t count,
                     GX2AttribStream *attribs,
                     GX2FetchShaderType::Value type,
                     GX2TessellationMode::Value tessMode)
{
}

void
GX2SetFetchShader(GX2FetchShader *shader)
{
}

void
GX2SetVertexShader(GX2VertexShader *shader)
{
   // Some kind of shenanigans that involves using a hardcoded *shader


   uint32_t shaderRegData[] = {
      shader->data.getAddress(),
      shader->size >> 3,
      0x10,
      0x10,
      shader->regs.sq_pgm_resources_vs.value
   };
   pm4::write(pm4::SetContextRegs{ latte::Register::SQ_PGM_START_VS, shaderRegData });

   if (shader->mode != GX2ShaderMode::GeometryShader) {
      pm4::write(pm4::SetContextReg{ latte::Register::VGT_PRIMITIVEID_EN, shader->regs.vgt_primitiveid_en.value });
      pm4::write(pm4::SetContextReg{ latte::Register::SPI_VS_OUT_CONFIG, shader->regs.spi_vs_out_config.value });
      pm4::write(pm4::SetContextReg{ latte::Register::PA_CL_VS_OUT_CNTL, shader->regs.pa_cl_vs_out_cntl.value });
      if (shader->regs.num_spi_vs_out_id > 0) {
         pm4::write(pm4::SetContextRegs{
            latte::Register::SPI_VS_OUT_ID_0,
            { &shader->regs.spi_vs_out_id[0].value, shader->regs.num_spi_vs_out_id } });
      }
      pm4::write(pm4::SetContextReg{ latte::Register::SQ_PGM_CF_OFFSET_VS, 0 });
      if (shader->hasStreamOut) {
         //_GX2WriteStreamOutStride(&shader->streamOutVertexStride);
      }
      pm4::write(pm4::SetContextReg{ latte::Register::SQ_PGM_CF_OFFSET_VS, 0 });
      pm4::write(pm4::SetContextReg{ latte::Register::VGT_STRMOUT_BUFFER_EN, shader->regs.vgt_strmout_buffer_en.value });
   } else {
      pm4::write(pm4::SetContextReg{ latte::Register::SQ_ESGS_RING_ITEMSIZE, shader->ringItemsize });
   }

   pm4::write(pm4::SetContextReg{ latte::Register::SQ_VTX_SEMANTIC_CLEAR, shader->regs.sq_vtx_semantic_clear.value });

   if (shader->regs.num_sq_vtx_semantic > 0) {
      pm4::write(pm4::SetContextRegs{
         latte::Register::SQ_VTX_SEMANTIC_0,
         { &shader->regs.sq_vtx_semantic[0].value, shader->regs.num_sq_vtx_semantic } });
   }

   pm4::write(pm4::SetContextReg{ latte::Register::VGT_VERTEX_REUSE_BLOCK_CNTL, shader->regs.vgt_vertex_reuse_block_cntl.value });
   pm4::write(pm4::SetContextReg{ latte::Register::VGT_HOS_REUSE_DEPTH, shader->regs.vgt_hos_reuse_depth.value });

   if (shader->loopVarCount > 0) {
      //_GX2SetVertexLoopVar(shader->loopVars, shader->loopVars + (shader->loopVarCount << 3));
   }
}

void
GX2SetPixelShader(GX2PixelShader *shader)
{
}

void
GX2SetGeometryShader(GX2GeometryShader *shader)
{
}

void
GX2SetPixelSampler(GX2Sampler *sampler, uint32_t id)
{
}

void
GX2SetVertexUniformReg(uint32_t offset, uint32_t count, void *data)
{
}

void
GX2SetPixelUniformReg(uint32_t offset, uint32_t count, void *data)
{
}

void
GX2SetVertexUniformBlock(uint32_t location, uint32_t size, const void *data)
{
}

void
GX2SetPixelUniformBlock(uint32_t location, uint32_t size, const void *data)
{
}

void
GX2SetShaderModeEx(GX2ShaderMode::Value mode,
                   uint32_t unk1, uint32_t unk2, uint32_t unk3,
                   uint32_t unk4, uint32_t unk5, uint32_t unk6)
{
}

uint32_t
GX2GetPixelShaderGPRs(GX2PixelShader *shader)
{
   return 0;
}

uint32_t
GX2GetPixelShaderStackEntries(GX2PixelShader *shader)
{
   return 0;
}

uint32_t
GX2GetVertexShaderGPRs(GX2VertexShader *shader)
{
   return 0;
}

uint32_t
GX2GetVertexShaderStackEntries(GX2VertexShader *shader)
{
   return 0;
}
