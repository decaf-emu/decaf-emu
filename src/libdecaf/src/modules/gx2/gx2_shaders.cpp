#include <cassert>
#include <stdexcept>
#include "common/emuassert.h"
#include "gx2_debug.h"
#include "gx2_shaders.h"
#include "gpu/pm4_writer.h"

namespace gx2
{

uint32_t
GX2CalcGeometryShaderInputRingBufferSize(uint32_t ringItemSize)
{
   return ringItemSize * 16384;
}

uint32_t
GX2CalcGeometryShaderOutputRingBufferSize(uint32_t ringItemSize)
{
   return ringItemSize * 16384;
}

void
GX2SetFetchShader(GX2FetchShader *shader)
{
   auto sq_pgm_resources_fs = shader->regs.sq_pgm_resources_fs.value();

   uint32_t shaderRegData[] = {
      shader->data.getAddress() >> 8,
      shader->size >> 3,
      0x100000,
      0x100000,
      sq_pgm_resources_fs.value,
   };
   pm4::write(pm4::SetContextRegs { latte::Register::SQ_PGM_START_FS, gsl::as_span(shaderRegData) });

   uint32_t vgt_instance_step_rates[] = {
      shader->divisors[0],
      shader->divisors[1],
   };
   pm4::write(pm4::SetContextRegs { latte::Register::VGT_INSTANCE_STEP_RATE_0, gsl::as_span(vgt_instance_step_rates) });
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

   uint32_t shaderProgAddr = shader->data.getAddress();
   uint32_t shaderProgSize = shader->size;
   if (!shaderProgAddr) {
      shaderProgAddr = shader->gx2rData.buffer.getAddress();
      shaderProgSize = shader->gx2rData.elemCount * shader->gx2rData.elemSize;
   }

   emuassert(shaderProgAddr);
   emuassert(shaderProgSize);

   uint32_t shaderRegData[] = {
      shaderProgAddr >> 8,
      shaderProgSize >> 3,
      0x100000,
      0x100000,
      sq_pgm_resources_vs.value,
   };

   if (shader->mode != GX2ShaderMode::GeometryShader) {
      pm4::write(pm4::SetContextRegs { latte::Register::SQ_PGM_START_VS, gsl::as_span(shaderRegData) });
      pm4::write(pm4::SetContextReg { latte::Register::VGT_PRIMITIVEID_EN, vgt_primitiveid_en.value });
      pm4::write(pm4::SetContextReg { latte::Register::SPI_VS_OUT_CONFIG, spi_vs_out_config.value });
      pm4::write(pm4::SetContextReg { latte::Register::PA_CL_VS_OUT_CNTL, pa_cl_vs_out_cntl.value });

      if (shader->regs.num_spi_vs_out_id > 0) {
         pm4::write(pm4::SetContextRegs {
            latte::Register::SPI_VS_OUT_ID_0,
            gsl::as_span(&spi_vs_out_id[0].value, shader->regs.num_spi_vs_out_id)
         });
      }

      pm4::write(pm4::SetContextReg { latte::Register::SQ_PGM_CF_OFFSET_VS, 0 });

      if (shader->hasStreamOut) {
         pm4::write(pm4::SetContextReg { latte::Register::VGT_STRMOUT_VTX_STRIDE_0, shader->streamOutStride[0] >> 2 });
         pm4::write(pm4::SetContextReg { latte::Register::VGT_STRMOUT_VTX_STRIDE_1, shader->streamOutStride[1] >> 2 });
         pm4::write(pm4::SetContextReg { latte::Register::VGT_STRMOUT_VTX_STRIDE_2, shader->streamOutStride[2] >> 2 });
         pm4::write(pm4::SetContextReg { latte::Register::VGT_STRMOUT_VTX_STRIDE_3, shader->streamOutStride[3] >> 2 });
      }

      pm4::write(pm4::SetContextReg { latte::Register::VGT_STRMOUT_BUFFER_EN, vgt_strmout_buffer_en.value });
   } else {
      pm4::write(pm4::SetContextRegs { latte::Register::SQ_PGM_START_ES, gsl::as_span(shaderRegData) });
      pm4::write(pm4::SetContextReg { latte::Register::SQ_ESGS_RING_ITEMSIZE, ringItemsize });
   }

   pm4::write(pm4::SetContextReg { latte::Register::SQ_VTX_SEMANTIC_CLEAR, sq_vtx_semantic_clear.value });

   if (shader->regs.num_sq_vtx_semantic > 0) {
      pm4::write(pm4::SetContextRegs {
         latte::Register::SQ_VTX_SEMANTIC_0,
         gsl::as_span(&sq_vtx_semantic[0].value, shader->regs.num_sq_vtx_semantic)
      });
   }

   pm4::write(pm4::SetContextReg { latte::Register::VGT_VERTEX_REUSE_BLOCK_CNTL, vgt_vertex_reuse_block_cntl.value });
   pm4::write(pm4::SetContextReg { latte::Register::VGT_HOS_REUSE_DEPTH, vgt_hos_reuse_depth.value });

   for (auto i = 0u; i < shader->loopVarCount; ++i) {
      auto id = static_cast<latte::Register>(latte::Register::SQ_LOOP_CONST_VS_0 + shader->loopVars[i].offset);
      pm4::write(pm4::SetLoopConst { id, shader->loopVars[i].value });
   }

   GX2DebugDumpShader(shader);
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

   uint32_t shaderProgAddr = shader->data.getAddress();
   uint32_t shaderProgSize = shader->size;
   if (!shaderProgAddr) {
      shaderProgAddr = shader->gx2rData.buffer.getAddress();
      shaderProgSize = shader->gx2rData.elemCount * shader->gx2rData.elemSize;
   }

   emuassert(shaderProgAddr);
   emuassert(shaderProgSize);

   uint32_t shaderRegData[] = {
      shaderProgAddr >> 8,
      shaderProgSize >> 3,
      0x100000,
      0x100000,
      sq_pgm_resources_ps.value,
      sq_pgm_exports_ps.value,
   };
   pm4::write(pm4::SetContextRegs { latte::Register::SQ_PGM_START_PS, gsl::as_span(shaderRegData) });

   uint32_t spi_ps_in_control[] = {
      spi_ps_in_control_0.value,
      spi_ps_in_control_1.value,
   };
   pm4::write(pm4::SetContextRegs { latte::Register::SPI_PS_IN_CONTROL_0, gsl::as_span(spi_ps_in_control) });

   if (num_spi_ps_input_cntl > 0) {
      pm4::write(pm4::SetContextRegs {
         latte::Register::SPI_PS_INPUT_CNTL_0,
         gsl::as_span(&spi_ps_input_cntls[0].value, num_spi_ps_input_cntl),
      });
   }

   db_shader_control = db_shader_control
      .DUAL_EXPORT_ENABLE().set(1);

   pm4::write(pm4::SetContextReg { latte::Register::CB_SHADER_MASK, cb_shader_mask.value });
   pm4::write(pm4::SetContextReg { latte::Register::CB_SHADER_CONTROL, cb_shader_control.value });
   pm4::write(pm4::SetContextReg { latte::Register::DB_SHADER_CONTROL, db_shader_control.value });
   pm4::write(pm4::SetContextReg { latte::Register::SPI_INPUT_Z, spi_input_z.value });

   for (auto i = 0u; i < shader->loopVarCount; ++i) {
      auto id = static_cast<latte::Register>(latte::Register::SQ_LOOP_CONST_PS_0 + shader->loopVars[i].offset);
      pm4::write(pm4::SetLoopConst { id, shader->loopVars[i].value });
   }

   GX2DebugDumpShader(shader);
}

void
GX2SetGeometryShader(GX2GeometryShader *shader)
{
   // Setup geometry shader data
   auto sq_pgm_resources_gs = shader->regs.sq_pgm_resources_gs.value();
   auto sq_gs_vert_itemsize = shader->regs.sq_gs_vert_itemsize.value();
   auto vgt_gs_out_prim_type = shader->regs.vgt_gs_out_prim_type.value();
   auto vgt_gs_mode = shader->regs.vgt_gs_mode.value();
   auto vgt_strmout_buffer_en = shader->regs.vgt_strmout_buffer_en.value();

   uint32_t shaderProgAddr = shader->data.getAddress();
   uint32_t shaderProgSize = shader->size;
   if (!shaderProgAddr) {
      shaderProgAddr = shader->gx2rData.buffer.getAddress();
      shaderProgSize = shader->gx2rData.elemCount * shader->gx2rData.elemSize;
   }

   emuassert(shaderProgAddr);
   emuassert(shaderProgSize);

   uint32_t shaderRegData[] = {
      shaderProgAddr >> 8,
      shaderProgSize >> 3,
      0,
      0,
      sq_pgm_resources_gs.value,
   };
   pm4::write(pm4::SetContextRegs { latte::Register::SQ_PGM_START_GS, gsl::as_span(shaderRegData) });
   pm4::write(pm4::SetContextReg { latte::Register::VGT_GS_OUT_PRIM_TYPE, vgt_gs_out_prim_type.value });
   pm4::write(pm4::SetContextReg { latte::Register::VGT_GS_MODE, vgt_gs_mode.value });
   pm4::write(pm4::SetContextReg { latte::Register::SQ_GS_VERT_ITEMSIZE, sq_gs_vert_itemsize.value });

   if (shader->hasStreamOut) {
      pm4::write(pm4::SetContextReg { latte::Register::VGT_STRMOUT_VTX_STRIDE_0, shader->streamOutStride[0] >> 2 });
      pm4::write(pm4::SetContextReg { latte::Register::VGT_STRMOUT_VTX_STRIDE_1, shader->streamOutStride[1] >> 2 });
      pm4::write(pm4::SetContextReg { latte::Register::VGT_STRMOUT_VTX_STRIDE_2, shader->streamOutStride[2] >> 2 });
      pm4::write(pm4::SetContextReg { latte::Register::VGT_STRMOUT_VTX_STRIDE_3, shader->streamOutStride[3] >> 2 });
   }

   pm4::write(pm4::SetContextReg { latte::Register::VGT_STRMOUT_BUFFER_EN, vgt_strmout_buffer_en.value });

   // Setup vertex shader data
   auto sq_pgm_resources_vs = shader->regs.sq_pgm_resources_vs.value();
   auto num_spi_vs_out_id = shader->regs.num_spi_vs_out_id.value();
   auto spi_vs_out_id = shader->regs.spi_vs_out_id.value();
   auto spi_vs_out_config = shader->regs.spi_vs_out_config.value();
   auto pa_cl_vs_out_cntl = shader->regs.pa_cl_vs_out_cntl.value();

   uint32_t vertexShaderRegData[] = {
      shader->vertexShaderData.getAddress() >> 8,
      shader->vertexShaderSize >> 3,
      0,
      0,
      sq_pgm_resources_vs.value,
   };
   pm4::write(pm4::SetContextRegs { latte::Register::SQ_PGM_START_VS, gsl::as_span(vertexShaderRegData) });
   pm4::write(pm4::SetContextReg { latte::Register::PA_CL_VS_OUT_CNTL, pa_cl_vs_out_cntl.value });

   if (shader->regs.num_spi_vs_out_id > 0) {
      pm4::write(pm4::SetContextRegs {
         latte::Register::SPI_VS_OUT_ID_0,
         gsl::as_span(&spi_vs_out_id[0].value, shader->regs.num_spi_vs_out_id)
      });
   }

   pm4::write(pm4::SetContextReg { latte::Register::SPI_VS_OUT_CONFIG, spi_vs_out_config.value });
   pm4::write(pm4::SetContextReg { latte::Register::SQ_GSVS_RING_ITEMSIZE, shader->ringItemSize });

   for (auto i = 0u; i < shader->loopVarCount; ++i) {
      auto id = static_cast<latte::Register>(latte::Register::SQ_LOOP_CONST_GS_0 + shader->loopVars[i].offset);
      pm4::write(pm4::SetLoopConst { id, shader->loopVars[i].value });
   }
}

void
_GX2SetSampler(GX2Sampler *sampler, uint32_t id)
{
   pm4::write(pm4::SetSamplerAttrib {
      id * 3,
      sampler->regs.word0,
      sampler->regs.word1,
      sampler->regs.word2
   });
}

void
GX2SetPixelSampler(GX2Sampler *sampler, uint32_t id)
{
   assert(id < 0x12);
   _GX2SetSampler(sampler, 0 + id);
}

void
GX2SetVertexSampler(GX2Sampler *sampler, uint32_t id)
{
   assert(id < 0x12);
   _GX2SetSampler(sampler, 18 + id);
}

void
GX2SetGeometrySampler(GX2Sampler *sampler, uint32_t id)
{
   assert(id < 0x12);
   _GX2SetSampler(sampler, 36 + id);
}

void
GX2SetVertexUniformReg(uint32_t offset, uint32_t count, be_val<uint32_t> *data)
{
   auto loop = offset >> 16;
   if (loop) {
      auto id = static_cast<latte::Register>(latte::Register::SQ_LOOP_CONST_VS_0 + 4 * loop);
      pm4::write(pm4::SetLoopConst { id, data[0] });
   }

   auto alu = offset & 0x7fff;
   auto id = static_cast<latte::Register>(latte::Register::SQ_ALU_CONSTANT0_256 + 4 * alu);

   // Custom write packet so we can endian swap data
   pm4::PacketWriter writer { pm4::SetAluConsts::Opcode };
   writer.REG_OFFSET(id, latte::Register::AluConstRegisterBase);

   for (auto i = 0u; i < count; ++i) {
      writer(data[i].value());
   }
}

void
GX2SetPixelUniformReg(uint32_t offset, uint32_t count, be_val<uint32_t> *data)
{
   auto loop = offset >> 16;

   if (loop) {
      auto id = static_cast<latte::Register>(latte::Register::SQ_LOOP_CONST_PS_0 + 4 * loop);
      pm4::write(pm4::SetLoopConst { id, data[0] });
   }

   auto alu = offset & 0x7fff;
   auto id = static_cast<latte::Register>(latte::Register::SQ_ALU_CONSTANT0_0 + 4 * alu);

   // Custom write packet so we can endian swap data
   pm4::PacketWriter writer { pm4::SetAluConsts::Opcode };
   writer.REG_OFFSET(id, latte::Register::AluConstRegisterBase);

   for (auto i = 0u; i < count; ++i) {
      writer(data[i].value());
   }
}

void
GX2SetVertexUniformBlock(uint32_t location, uint32_t size, const void *data)
{
   pm4::SetVtxResource res;
   memset(&res, 0, sizeof(pm4::SetVtxResource));
   res.id = (location * 7) + latte::SQ_VS_BUF_RESOURCE_0;
   res.baseAddress = data;

   res.word1.SIZE = size - 1;

   res.word2 = res.word2
      .STRIDE().set(16)
      .DATA_FORMAT().set(latte::FMT_32_32_32_32)
      .FORMAT_COMP_ALL().set(latte::SQ_FORMAT_COMP_SIGNED);

   res.word3 = res.word3
      .MEM_REQUEST_SIZE().set(1);

   res.word6 = res.word6
      .TYPE().set(latte::SQ_TEX_VTX_VALID_BUFFER);

   pm4::write(res);

   auto addrId = static_cast<latte::Register>(latte::Register::SQ_ALU_CONST_CACHE_VS_0 + location * 4);
   auto addr256 = mem::untranslate(data) >> 8;
   pm4::write(pm4::SetContextReg { addrId, addr256 });

   auto sizeId = static_cast<latte::Register>(latte::Register::SQ_ALU_CONST_BUFFER_SIZE_VS_0 + location * 4);
   auto size256 = ((size + 255) >> 8) & 0x1FF;
   pm4::write(pm4::SetContextReg { sizeId, size256 });
}

void
GX2SetPixelUniformBlock(uint32_t location, uint32_t size, const void *data)
{
   pm4::SetVtxResource res;
   memset(&res, 0, sizeof(pm4::SetVtxResource));
   res.id = (location * 7) + latte::SQ_PS_BUF_RESOURCE_0;
   res.baseAddress = data;
   res.word1.SIZE = size - 1;

   res.word2 = res.word2
      .STRIDE().set(16)
      .DATA_FORMAT().set(latte::FMT_32_32_32_32)
      .FORMAT_COMP_ALL().set(latte::SQ_FORMAT_COMP_SIGNED);

   res.word3 = res.word3
      .MEM_REQUEST_SIZE().set(1);

   res.word6 = res.word6
      .TYPE().set(latte::SQ_TEX_VTX_VALID_BUFFER);

   pm4::write(res);

   auto addrId = static_cast<latte::Register>(latte::Register::SQ_ALU_CONST_CACHE_PS_0 + location * 4);
   auto addr256 = mem::untranslate(data) >> 8;
   pm4::write(pm4::SetContextReg { addrId, addr256 });

   auto sizeId = static_cast<latte::Register>(latte::Register::SQ_ALU_CONST_BUFFER_SIZE_PS_0 + location * 4);
   auto size256 = ((size + 255) >> 8) & 0x1FF;
   pm4::write(pm4::SetContextReg { sizeId, size256 });
}

void
GX2SetGeometryUniformBlock(uint32_t location, uint32_t size, const void *data)
{
   pm4::SetVtxResource res;
   memset(&res, 0, sizeof(pm4::SetVtxResource));
   res.id = (location * 7) + latte::SQ_GS_BUF_RESOURCE_0;
   res.baseAddress = data;
   res.word1.SIZE = size - 1;

   res.word2 = res.word2
      .STRIDE().set(16)
      .DATA_FORMAT().set(latte::FMT_32_32_32_32)
      .FORMAT_COMP_ALL().set(latte::SQ_FORMAT_COMP_SIGNED);

   res.word3 = res.word3
      .MEM_REQUEST_SIZE().set(1);

   res.word6 = res.word6
      .TYPE().set(latte::SQ_TEX_VTX_VALID_BUFFER);

   pm4::write(res);

   auto addrId = static_cast<latte::Register>(latte::Register::SQ_ALU_CONST_CACHE_GS_0 + location * 4);
   auto addr256 = mem::untranslate(data) >> 8;
   pm4::write(pm4::SetContextReg { addrId, addr256 });

   auto sizeId = static_cast<latte::Register>(latte::Register::SQ_ALU_CONST_BUFFER_SIZE_GS_0 + location * 4);
   auto size256 = ((size + 255) >> 8) & 0x1FF;
   pm4::write(pm4::SetContextReg { sizeId, size256 });
}

void
GX2SetShaderModeEx(GX2ShaderMode mode,
                   uint32_t numVsGpr, uint32_t numVsStackEntries,
                   uint32_t numGsGpr, uint32_t numGsStackEntries,
                   uint32_t numPsGpr, uint32_t numPsStackEntries)
{
   auto sq_config = latte::SQ_CONFIG::get(0);
   auto sq_gpr_resource_mgmt_1 = latte::SQ_GPR_RESOURCE_MGMT_1::get(0);
   auto sq_gpr_resource_mgmt_2 = latte::SQ_GPR_RESOURCE_MGMT_2::get(0);
   auto sq_thread_resource_mgmt = latte::SQ_THREAD_RESOURCE_MGMT::get(0);
   auto sq_stack_resource_mgmt_1 = latte::SQ_STACK_RESOURCE_MGMT_1::get(0);
   auto sq_stack_resource_mgmt_2 = latte::SQ_STACK_RESOURCE_MGMT_2::get(0);

   if (mode != GX2ShaderMode::GeometryShader) {
      auto vgt_gs_mode = latte::VGT_GS_MODE::get(0);

      if (mode == GX2ShaderMode::ComputeShader) {
         vgt_gs_mode = vgt_gs_mode
            .MODE().set(latte::GS_SCENARIO_G)
            .COMPUTE_MODE().set(1)
            .FAST_COMPUTE_MODE().set(1)
            .PARTIAL_THD_AT_EOI().set(1);
      } else {
         vgt_gs_mode = vgt_gs_mode
            .MODE().set(latte::GS_OFF);
      }

      pm4::write(pm4::SetContextReg { latte::Register::VGT_GS_MODE, vgt_gs_mode.value });
   }

   if (mode == GX2ShaderMode::ComputeShader) {
      sq_config = sq_config
         .ALU_INST_PREFER_VECTOR().set(1)
         .PS_PRIO().set(0)
         .VS_PRIO().set(1)
         .GS_PRIO().set(2)
         .ES_PRIO().set(3);
   } else {
      sq_config = sq_config
         .PS_PRIO().set(3)
         .VS_PRIO().set(2)
         .GS_PRIO().set(1)
         .ES_PRIO().set(0);
   }

   if (mode == GX2ShaderMode::UniformRegister) {
      sq_config = sq_config
         .DX9_CONSTS().set(1);
   }

   if (mode == GX2ShaderMode::GeometryShader) {
      sq_gpr_resource_mgmt_1 = sq_gpr_resource_mgmt_1
         .NUM_PS_GPRS().set(numPsGpr)
         .NUM_VS_GPRS().set(64)
         .NUM_CLAUSE_TEMP_GPRS().set(4);

      sq_gpr_resource_mgmt_2 = sq_gpr_resource_mgmt_2
         .NUM_ES_GPRS().set(numVsGpr)
         .NUM_GS_GPRS().set(numGsGpr);

      sq_stack_resource_mgmt_1 = sq_stack_resource_mgmt_1
         .NUM_PS_STACK_ENTRIES().set(numPsStackEntries);

      sq_stack_resource_mgmt_2
         .NUM_ES_STACK_ENTRIES().set(numVsStackEntries);

      sq_stack_resource_mgmt_2
         .NUM_GS_STACK_ENTRIES().set(numGsStackEntries);

      sq_thread_resource_mgmt = sq_thread_resource_mgmt
         .NUM_PS_THREADS().set(124)
         .NUM_VS_THREADS().set(32)
         .NUM_GS_THREADS().set(8)
         .NUM_ES_THREADS().set(28);
   } else if (mode == GX2ShaderMode::ComputeShader) {
      sq_gpr_resource_mgmt_1 = sq_gpr_resource_mgmt_1
         .NUM_CLAUSE_TEMP_GPRS().set(4);

      sq_gpr_resource_mgmt_2 = sq_gpr_resource_mgmt_2
         .NUM_ES_GPRS().set(248);

      sq_stack_resource_mgmt_2 = sq_stack_resource_mgmt_2
         .NUM_ES_STACK_ENTRIES().set(256);

      sq_thread_resource_mgmt = sq_thread_resource_mgmt
         .NUM_PS_THREADS().set(1)
         .NUM_VS_THREADS().set(1)
         .NUM_GS_THREADS().set(1)
         .NUM_ES_THREADS().set(189);
   } else {
      sq_gpr_resource_mgmt_1 = sq_gpr_resource_mgmt_1
         .NUM_PS_GPRS().set(numPsGpr)
         .NUM_VS_GPRS().set(numVsGpr);

      sq_stack_resource_mgmt_1 = sq_stack_resource_mgmt_1
         .NUM_PS_STACK_ENTRIES().set(numPsStackEntries)
         .NUM_VS_STACK_ENTRIES().set(numVsStackEntries);

      sq_thread_resource_mgmt = sq_thread_resource_mgmt
         .NUM_PS_THREADS().set(136)
         .NUM_VS_THREADS().set(48)
         .NUM_GS_THREADS().set(4)
         .NUM_ES_THREADS().set(4);
   }

   uint32_t regData[] = {
      sq_config.value,
      sq_gpr_resource_mgmt_1.value,
      sq_gpr_resource_mgmt_2.value,
      sq_thread_resource_mgmt.value,
      sq_stack_resource_mgmt_1.value,
      sq_stack_resource_mgmt_2.value,
   };
   pm4::write(pm4::SetConfigRegs { latte::Register::SQ_CONFIG, gsl::as_span(regData) });

   if (mode == GX2ShaderMode::ComputeShader) {
      uint32_t ringBaseData[] = { 0, 0xFFFFFF, 0, 0xFFFFFF };
      pm4::write(pm4::SetConfigRegs { latte::Register::SQ_ESGS_RING_BASE, gsl::as_span(ringBaseData) });

      uint32_t ringItemSizes[] = { 0, 1 };
      pm4::write(pm4::SetContextRegs { latte::Register::SQ_ESGS_RING_ITEMSIZE, gsl::as_span(ringItemSizes) });

      pm4::write(pm4::SetContextReg { latte::Register::VGT_STRMOUT_EN, 0 });
   }
}

void
GX2SetStreamOutEnable(BOOL enable)
{
   auto vgt_strmout_en = latte::VGT_STRMOUT_EN::get(0);

   vgt_strmout_en = vgt_strmout_en
      .STREAMOUT().set(!!enable);

   pm4::write(pm4::SetContextReg { latte::Register::VGT_STRMOUT_EN, vgt_strmout_en.value });
}

void
GX2SetGeometryShaderInputRingBuffer(void *buffer, uint32_t size)
{
   pm4::write(pm4::SetConfigReg { latte::Register::SQ_ESGS_RING_BASE, mem::untranslate(buffer) >> 8 });
   pm4::write(pm4::SetConfigReg { latte::Register::SQ_ESGS_RING_SIZE, size >> 8 });

   pm4::SetVtxResource res;
   memset(&res, 0, sizeof(pm4::SetVtxResource));
   res.id = latte::SQ_GS_GSIN_RESOURCE;
   res.baseAddress = buffer;
   res.word1.SIZE = size - 1;

   res.word2 = res.word2
      .STRIDE().set(4)
      .CLAMP_X().set(latte::SQ_VTX_CLAMP_NAN)
      .DATA_FORMAT().set(latte::FMT_32_32_32_32_FLOAT);

   res.word3 = res.word3
      .UNCACHED().set(1);

   res.word6 = res.word6
      .TYPE().set(latte::SQ_TEX_VTX_VALID_BUFFER);

   pm4::write(res);
}

void
GX2SetGeometryShaderOutputRingBuffer(void *buffer, uint32_t size)
{
   pm4::write(pm4::SetConfigReg { latte::Register::SQ_GSVS_RING_BASE, mem::untranslate(buffer) >> 8 });
   pm4::write(pm4::SetConfigReg { latte::Register::SQ_GSVS_RING_SIZE, size >> 8 });

   pm4::SetVtxResource res;
   memset(&res, 0, sizeof(pm4::SetVtxResource));
   res.id = latte::SQ_VS_GSOUT_RESOURCE;
   res.baseAddress = buffer;
   res.word1.SIZE = size - 1;

   res.word2 = res.word2
      .STRIDE().set(4)
      .CLAMP_X().set(latte::SQ_VTX_CLAMP_NAN)
      .DATA_FORMAT().set(latte::FMT_32_32_32_32_FLOAT);

   res.word3 = res.word3
      .UNCACHED().set(1);

   res.word6 = res.word6
      .TYPE().set(latte::SQ_TEX_VTX_VALID_BUFFER);

   pm4::write(res);
}

uint32_t
GX2GetPixelShaderGPRs(GX2PixelShader *shader)
{
   return shader->regs.sq_pgm_resources_ps.value().NUM_GPRS();
}

uint32_t
GX2GetPixelShaderStackEntries(GX2PixelShader *shader)
{
   return shader->regs.sq_pgm_resources_ps.value().STACK_SIZE();
}

uint32_t
GX2GetVertexShaderGPRs(GX2VertexShader *shader)
{
   return shader->regs.sq_pgm_resources_vs.value().NUM_GPRS();
}

uint32_t
GX2GetVertexShaderStackEntries(GX2VertexShader *shader)
{
   return shader->regs.sq_pgm_resources_vs.value().STACK_SIZE();
}

uint32_t
GX2GetGeometryShaderGPRs(GX2GeometryShader *shader)
{
   return shader->regs.sq_pgm_resources_gs.value().NUM_GPRS();
}

uint32_t
GX2GetGeometryShaderStackEntries(GX2GeometryShader *shader)
{
   return shader->regs.sq_pgm_resources_gs.value().STACK_SIZE();
}

} // namespace gx2
