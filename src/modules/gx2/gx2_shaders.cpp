#include <cassert>
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

void
GX2SetFetchShader(GX2FetchShader *shader)
{
   auto sq_pgm_resources_fs = shader->regs.sq_pgm_resources_fs.value();

   uint32_t shaderRegData[] = {
      shader->data.getAddress() / 256,
      shader->size >> 3,
      0x10,
      0x10,
      sq_pgm_resources_fs.value,
   };
   pm4::write(pm4::SetContextRegs { latte::Register::SQ_PGM_START_FS, shaderRegData });

   uint32_t vgt_instance_step_rates[] = {
      shader->divisors[0],
      shader->divisors[1],
   };
   pm4::write(pm4::SetContextRegs { latte::Register::VGT_INSTANCE_STEP_RATE_0, vgt_instance_step_rates });
}

void
GX2SetVertexShader(GX2VertexShader *shader)
{
   auto ringItemsize = shader->ringItemsize.value();
   auto pa_cl_vs_out_cntl = shader->regs.pa_cl_vs_out_cntl.value();

   auto spi_vs_out_config = shader->regs.spi_vs_out_config.value();
   auto num_spi_vs_out_id = shader->regs.num_spi_vs_out_id.value();
   auto spi_vs_out_id = shader->regs.spi_vs_out_id.value();

   auto sq_pgm_resources_vs = shader->regs.sq_pgm_resources_vs.value();
   auto sq_vtx_semantic_clear = shader->regs.sq_vtx_semantic_clear.value();
   auto num_sq_vtx_semantic = shader->regs.num_sq_vtx_semantic.value();
   auto sq_vtx_semantic = shader->regs.sq_vtx_semantic.value();

   auto vgt_hos_reuse_depth = shader->regs.vgt_hos_reuse_depth.value();
   auto vgt_primitiveid_en = shader->regs.vgt_primitiveid_en.value();
   auto vgt_strmout_buffer_en = shader->regs.vgt_strmout_buffer_en.value();
   auto vgt_vertex_reuse_block_cntl = shader->regs.vgt_vertex_reuse_block_cntl.value();

   // Some kind of shenanigans that involves using a hardcoded *shader

   uint32_t shaderRegData[] = {
      shader->data.getAddress() / 256,
      shader->size >> 3,
      0x10,
      0x10,
      sq_pgm_resources_vs.value,
   };
   pm4::write(pm4::SetContextRegs{ latte::Register::SQ_PGM_START_VS, shaderRegData });

   if (shader->mode != GX2ShaderMode::GeometryShader) {
      pm4::write(pm4::SetContextReg{ latte::Register::VGT_PRIMITIVEID_EN, vgt_primitiveid_en.value });
      pm4::write(pm4::SetContextReg{ latte::Register::SPI_VS_OUT_CONFIG, spi_vs_out_config.value });
      pm4::write(pm4::SetContextReg{ latte::Register::PA_CL_VS_OUT_CNTL, pa_cl_vs_out_cntl.value });

      if (shader->regs.num_spi_vs_out_id > 0) {
         pm4::write(pm4::SetContextRegs { latte::Register::SPI_VS_OUT_ID_0, { &spi_vs_out_id[0].value, num_spi_vs_out_id } });
      }

      pm4::write(pm4::SetContextReg{ latte::Register::SQ_PGM_CF_OFFSET_VS, 0 });

      if (shader->hasStreamOut) {
         //_GX2WriteStreamOutStride(&shader->streamOutVertexStride);
      }

      pm4::write(pm4::SetContextReg { latte::Register::SQ_PGM_CF_OFFSET_VS, 0 });
      pm4::write(pm4::SetContextReg { latte::Register::VGT_STRMOUT_BUFFER_EN, vgt_strmout_buffer_en.value });
   } else {
      pm4::write(pm4::SetContextReg { latte::Register::SQ_ESGS_RING_ITEMSIZE, ringItemsize });
   }

   pm4::write(pm4::SetContextReg { latte::Register::SQ_VTX_SEMANTIC_CLEAR, sq_vtx_semantic_clear.value });

   if (shader->regs.num_sq_vtx_semantic > 0) {
      pm4::write(pm4::SetContextRegs { latte::Register::SQ_VTX_SEMANTIC_0, { &sq_vtx_semantic[0].value, num_sq_vtx_semantic } });
   }

   pm4::write(pm4::SetContextReg{ latte::Register::VGT_VERTEX_REUSE_BLOCK_CNTL, vgt_vertex_reuse_block_cntl.value });
   pm4::write(pm4::SetContextReg{ latte::Register::VGT_HOS_REUSE_DEPTH, vgt_hos_reuse_depth.value });

   if (shader->loopVarCount > 0) {
      //_GX2SetVertexLoopVar(shader->loopVars, shader->loopVars + (shader->loopVarCount << 3));
   }
}

void
GX2SetPixelShader(GX2PixelShader *shader)
{
   auto cb_shader_control = shader->regs.cb_shader_control.value();
   auto cb_shader_mask = shader->regs.cb_shader_mask.value();
   auto db_shader_control = shader->regs.db_shader_control.value();

   auto spi_input_z = shader->regs.spi_input_z.value();
   auto spi_ps_in_control_0 = shader->regs.spi_ps_in_control_0.value();
   auto spi_ps_in_control_1 = shader->regs.spi_ps_in_control_1.value();
   auto spi_ps_input_cntls = shader->regs.spi_ps_input_cntls.value();
   auto num_spi_ps_input_cntl = shader->regs.num_spi_ps_input_cntl.value();

   auto sq_pgm_resources_ps = shader->regs.sq_pgm_resources_ps.value();
   auto sq_pgm_exports_ps = shader->regs.sq_pgm_exports_ps.value();

   uint32_t shaderRegData[] = {
      shader->data.getAddress() / 256,
      shader->size >> 3,
      0x10,
      0x10,
      sq_pgm_resources_ps.value,
      sq_pgm_exports_ps.value,
   };
   pm4::write(pm4::SetContextRegs { latte::Register::SQ_PGM_START_PS, shaderRegData });

   uint32_t spi_ps_in_control[] = {
      spi_ps_in_control_0.value,
      spi_ps_in_control_1.value,
   };
   pm4::write(pm4::SetContextRegs { latte::Register::SPI_PS_IN_CONTROL_0, spi_ps_in_control });

   if (num_spi_ps_input_cntl > 0) {
      pm4::write(pm4::SetContextRegs {
         latte::Register::SPI_PS_INPUT_CNTL_0,
         { &spi_ps_input_cntls[0].value, num_spi_ps_input_cntl }
      });
   }

   db_shader_control.DUAL_EXPORT_ENABLE = 1;
   pm4::write(pm4::SetContextReg { latte::Register::CB_SHADER_MASK, cb_shader_mask.value });
   pm4::write(pm4::SetContextReg { latte::Register::CB_SHADER_CONTROL, cb_shader_control.value });
   pm4::write(pm4::SetContextReg { latte::Register::DB_SHADER_CONTROL, db_shader_control.value });
   pm4::write(pm4::SetContextReg { latte::Register::SPI_INPUT_Z, spi_input_z.value });

   if (shader->loopVarCount > 0) {
      //_GX2SetPixelLoopVar(shader->loopVars, shader->loopVars + (shader->loopVarCount << 3));
   }
}

void
GX2SetGeometryShader(GX2GeometryShader *shader)
{
}

void
_GX2SetSampler(GX2Sampler *sampler, uint32_t id)
{
   pm4::write(pm4::SetSamplerAttrib {
      id,
      sampler->regs.word0,
      sampler->regs.word1,
      sampler->regs.word2
   });
}

void
GX2SetVertexSampler(GX2Sampler *sampler, uint32_t id)
{
   assert(id < 0x12);
   _GX2SetSampler(sampler, 0x12 + id);
}

void
GX2SetPixelSampler(GX2Sampler *sampler, uint32_t id)
{
   assert(id < 0x12);
   _GX2SetSampler(sampler, 0x00 + id);
}

void
GX2SetGeometrySampler(GX2Sampler *sampler, uint32_t id)
{
   assert(id < 0x12);
   _GX2SetSampler(sampler, 0x24 + id);
}

void
GX2SetVertexUniformReg(uint32_t offset, uint32_t count, uint32_t *data)
{
}

void
GX2SetPixelUniformReg(uint32_t offset, uint32_t count, uint32_t *data)
{
   auto loop = offset >> 16;

   if (loop) {
      auto id = static_cast<latte::Register::Value>(latte::Register::SQ_LOOP_CONST_0 + loop);
      pm4::write(pm4::SetLoopConst { id, data[0] });
   }

   auto alu = offset & 0x7fff;
   auto id = static_cast<latte::Register::Value>(latte::Register::SQ_ALU_CONSTANT0_0 + alu);
   pm4::write(pm4::SetAluConsts { id, { data, count } });
}

void
GX2SetVertexUniformBlock(uint32_t location, uint32_t size, const void *data)
{
   pm4::SetResourceAttrib attrib;
   memset(&attrib, 0, sizeof(pm4::SetResourceAttrib));
   attrib.id = (location * 7) + 0x7E0;
   attrib.baseAddress = data;
   attrib.size = size - 1;
   // GX2 actually sets a bunch of useless word2,word3 stuff
   attrib.word6.TYPE = latte::SQ_TEX_VTX_VALID_BUFFER;
   pm4::write(attrib);

   uint32_t addr256 = memory_untranslate(data) >> 8;

   auto addrId = static_cast<latte::Register::Value>(latte::Register::SQ_ALU_CONST_CACHE_VS_0 + location * 4);
   pm4::write(pm4::SetContextReg{ addrId, addr256 });

   auto sizeId = static_cast<latte::Register::Value>(latte::Register::SQ_ALU_CONST_BUFFER_SIZE_VS_0 + location * 4);
   pm4::write(pm4::SetContextReg{ sizeId, size - 1 });
}

void
GX2SetPixelUniformBlock(uint32_t location, uint32_t size, const void *data)
{
   pm4::SetResourceAttrib attrib;
   memset(&attrib, 0, sizeof(pm4::SetResourceAttrib));
   attrib.id = (location * 7) + 0x380;
   attrib.baseAddress = data;
   attrib.size = size - 1;
   // GX2 actually sets a bunch of useless word2,word3 stuff
   attrib.word6.TYPE = latte::SQ_TEX_VTX_VALID_BUFFER;
   pm4::write(attrib);

   const uint32_t addr256 = memory_untranslate(data) >> 8;

   auto addrId = static_cast<latte::Register::Value>(latte::Register::SQ_ALU_CONST_CACHE_PS_0 + location * 4);
   pm4::write(pm4::SetContextReg{ addrId, addr256 });

   auto sizeId = static_cast<latte::Register::Value>(latte::Register::SQ_ALU_CONST_BUFFER_SIZE_PS_0 + location * 4);
   pm4::write(pm4::SetContextReg{ sizeId, size - 1 });
}

void
GX2SetShaderModeEx(GX2ShaderMode::Value mode,
                   uint32_t numVsGpr, uint32_t numVsStackEntries, 
                   uint32_t numGsGpr, uint32_t numGsStackEntries,
                   uint32_t numPsGpr, uint32_t numPsStackEntries)
{
   auto sq_config = latte::SQ_CONFIG{ 0 };

   if (mode == GX2ShaderMode::UniformRegister) {
      sq_config.DX9_CONSTS = 1;
   } else if (mode == GX2ShaderMode::UniformBlock) {
      sq_config.DX9_CONSTS = 0;
   } else {
      gsl::fail_fast("Unexpected shader mode");
   }

   pm4::write(pm4::SetConfigReg{ latte::Register::SQ_CONFIG, sq_config.value });

   // Normally would do lots of SET_CONFIG_REG here,
   //   but not needed for our drivers.
}

uint32_t
GX2GetPixelShaderGPRs(GX2PixelShader *shader)
{
   return shader->regs.sq_pgm_resources_ps.value().NUM_GPRS;
}

uint32_t
GX2GetPixelShaderStackEntries(GX2PixelShader *shader)
{
   return shader->regs.sq_pgm_resources_ps.value().STACK_SIZE;
}

uint32_t
GX2GetVertexShaderGPRs(GX2VertexShader *shader)
{
   return shader->regs.sq_pgm_resources_vs.value().NUM_GPRS;
}

uint32_t
GX2GetVertexShaderStackEntries(GX2VertexShader *shader)
{
   return shader->regs.sq_pgm_resources_vs.value().STACK_SIZE;
}
