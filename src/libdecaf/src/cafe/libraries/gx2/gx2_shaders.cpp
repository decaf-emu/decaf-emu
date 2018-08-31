#include "gx2.h"
#include "gx2_debug.h"
#include "gx2_internal_cbpool.h"
#include "gx2_fetchshader.h"
#include "gx2_shaders.h"

#include <common/decaf_assert.h>

using namespace latte::pm4;
using latte::Register;

namespace cafe::gx2
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
GX2SetFetchShader(virt_ptr<GX2FetchShader> shader)
{
   auto sq_pgm_resources_fs = shader->regs.sq_pgm_resources_fs.value();

   uint32_t shaderRegData[] = {
      virt_cast<virt_addr>(shader->data) >> 8,
      shader->size >> 3,
      0x100000,
      0x100000,
      sq_pgm_resources_fs.value,
   };
   internal::writePM4(SetContextRegs { Register::SQ_PGM_START_FS, gsl::make_span(shaderRegData) });

   uint32_t vgt_instance_step_rates[] = {
      shader->divisors[0],
      shader->divisors[1],
   };
   internal::writePM4(SetContextRegs { Register::VGT_INSTANCE_STEP_RATE_0, gsl::make_span(vgt_instance_step_rates) });

   internal::debugDumpShader(shader);
}

void
GX2SetVertexShader(virt_ptr<GX2VertexShader> shader)
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

   auto shaderProgAddr = virt_cast<virt_addr>(shader->data);
   auto shaderProgSize = shader->size;
   if (!shaderProgAddr) {
      shaderProgAddr = virt_cast<virt_addr>(shader->gx2rData.buffer);
      shaderProgSize = shader->gx2rData.elemCount * shader->gx2rData.elemSize;
   }

   decaf_check(shaderProgAddr);
   decaf_check(shaderProgSize);

   uint32_t shaderRegData[] = {
      shaderProgAddr >> 8,
      shaderProgSize >> 3,
      0x100000,
      0x100000,
      sq_pgm_resources_vs.value,
   };

   if (shader->mode != GX2ShaderMode::GeometryShader) {
      internal::writePM4(SetContextRegs { Register::SQ_PGM_START_VS, gsl::make_span(shaderRegData) });
      internal::writePM4(SetContextReg { Register::VGT_PRIMITIVEID_EN, vgt_primitiveid_en.value });
      internal::writePM4(SetContextReg { Register::SPI_VS_OUT_CONFIG, spi_vs_out_config.value });
      internal::writePM4(SetContextReg { Register::PA_CL_VS_OUT_CNTL, pa_cl_vs_out_cntl.value });

      if (shader->regs.num_spi_vs_out_id > 0) {
         internal::writePM4(SetContextRegs {
            Register::SPI_VS_OUT_ID_0,
            gsl::make_span(&spi_vs_out_id[0].value, shader->regs.num_spi_vs_out_id)
         });
      }

      internal::writePM4(SetContextReg { Register::SQ_PGM_CF_OFFSET_VS, 0 });

      if (shader->hasStreamOut) {
         internal::writePM4(SetContextReg { Register::VGT_STRMOUT_VTX_STRIDE_0, shader->streamOutStride[0] >> 2 });
         internal::writePM4(SetContextReg { Register::VGT_STRMOUT_VTX_STRIDE_1, shader->streamOutStride[1] >> 2 });
         internal::writePM4(SetContextReg { Register::VGT_STRMOUT_VTX_STRIDE_2, shader->streamOutStride[2] >> 2 });
         internal::writePM4(SetContextReg { Register::VGT_STRMOUT_VTX_STRIDE_3, shader->streamOutStride[3] >> 2 });
      }

      internal::writePM4(SetContextReg { Register::VGT_STRMOUT_BUFFER_EN, vgt_strmout_buffer_en.value });
   } else {
      internal::writePM4(SetContextRegs { Register::SQ_PGM_START_ES, gsl::make_span(shaderRegData) });
      internal::writePM4(SetContextReg { Register::SQ_ESGS_RING_ITEMSIZE, ringItemsize });
   }

   internal::writePM4(SetContextReg { Register::SQ_VTX_SEMANTIC_CLEAR, sq_vtx_semantic_clear.value });

   if (shader->regs.num_sq_vtx_semantic > 0) {
      internal::writePM4(SetContextRegs {
         Register::SQ_VTX_SEMANTIC_0,
         gsl::make_span(&sq_vtx_semantic[0].value, shader->regs.num_sq_vtx_semantic)
      });
   }

   internal::writePM4(SetContextReg { Register::VGT_VERTEX_REUSE_BLOCK_CNTL, vgt_vertex_reuse_block_cntl.value });
   internal::writePM4(SetContextReg { Register::VGT_HOS_REUSE_DEPTH, vgt_hos_reuse_depth.value });

   for (auto i = 0u; i < shader->loopVarCount; ++i) {
      auto id = static_cast<Register>(Register::SQ_LOOP_CONST_VS_0 + shader->loopVars[i].offset);
      internal::writePM4(SetLoopConst { id, shader->loopVars[i].value });
   }

   internal::debugDumpShader(shader);
}

void
GX2SetPixelShader(virt_ptr<GX2PixelShader> shader)
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

   auto shaderProgAddr = virt_cast<virt_addr>(shader->data);
   auto shaderProgSize = shader->size;
   if (!shaderProgAddr) {
      shaderProgAddr = virt_cast<virt_addr>(shader->gx2rData.buffer);
      shaderProgSize = shader->gx2rData.elemCount * shader->gx2rData.elemSize;
   }

   decaf_check(shaderProgAddr);
   decaf_check(shaderProgSize);

   uint32_t shaderRegData[] = {
      shaderProgAddr >> 8,
      shaderProgSize >> 3,
      0x100000,
      0x100000,
      sq_pgm_resources_ps.value,
      sq_pgm_exports_ps.value,
   };
   internal::writePM4(SetContextRegs { Register::SQ_PGM_START_PS, gsl::make_span(shaderRegData) });

   uint32_t spi_ps_in_control[] = {
      spi_ps_in_control_0.value,
      spi_ps_in_control_1.value,
   };
   internal::writePM4(SetContextRegs { Register::SPI_PS_IN_CONTROL_0, gsl::make_span(spi_ps_in_control) });

   if (num_spi_ps_input_cntl > 0) {
      internal::writePM4(SetContextRegs {
         Register::SPI_PS_INPUT_CNTL_0,
         gsl::make_span(&spi_ps_input_cntls[0].value, num_spi_ps_input_cntl),
      });
   }

   db_shader_control = db_shader_control
      .DUAL_EXPORT_ENABLE(1);

   internal::writePM4(SetContextReg { Register::CB_SHADER_MASK, cb_shader_mask.value });
   internal::writePM4(SetContextReg { Register::CB_SHADER_CONTROL, cb_shader_control.value });
   internal::writePM4(SetContextReg { Register::DB_SHADER_CONTROL, db_shader_control.value });
   internal::writePM4(SetContextReg { Register::SPI_INPUT_Z, spi_input_z.value });

   for (auto i = 0u; i < shader->loopVarCount; ++i) {
      auto id = static_cast<Register>(Register::SQ_LOOP_CONST_PS_0 + shader->loopVars[i].offset);
      internal::writePM4(SetLoopConst { id, shader->loopVars[i].value });
   }

   internal::debugDumpShader(shader);
}

void
GX2SetGeometryShader(virt_ptr<GX2GeometryShader> shader)
{
   // Setup geometry shader data
   auto sq_pgm_resources_gs = shader->regs.sq_pgm_resources_gs.value();
   auto sq_gs_vert_itemsize = shader->regs.sq_gs_vert_itemsize.value();
   auto vgt_gs_out_prim_type = shader->regs.vgt_gs_out_prim_type.value();
   auto vgt_gs_mode = shader->regs.vgt_gs_mode.value();
   auto vgt_strmout_buffer_en = shader->regs.vgt_strmout_buffer_en.value();

   auto shaderProgAddr = virt_cast<virt_addr>(shader->data);
   auto shaderProgSize = shader->size;
   if (!shaderProgAddr) {
      shaderProgAddr = virt_cast<virt_addr>(shader->gx2rData.buffer);
      shaderProgSize = shader->gx2rData.elemCount * shader->gx2rData.elemSize;
   }

   decaf_check(shaderProgAddr);
   decaf_check(shaderProgSize);

   uint32_t shaderRegData[] = {
      shaderProgAddr >> 8,
      shaderProgSize >> 3,
      0,
      0,
      sq_pgm_resources_gs.value,
   };
   internal::writePM4(SetContextRegs { Register::SQ_PGM_START_GS, gsl::make_span(shaderRegData) });
   internal::writePM4(SetContextReg { Register::VGT_GS_OUT_PRIM_TYPE, vgt_gs_out_prim_type.value });
   internal::writePM4(SetContextReg { Register::VGT_GS_MODE, vgt_gs_mode.value });
   internal::writePM4(SetContextReg { Register::SQ_GS_VERT_ITEMSIZE, sq_gs_vert_itemsize.value });

   if (shader->hasStreamOut) {
      internal::writePM4(SetContextReg { Register::VGT_STRMOUT_VTX_STRIDE_0, shader->streamOutStride[0] >> 2 });
      internal::writePM4(SetContextReg { Register::VGT_STRMOUT_VTX_STRIDE_1, shader->streamOutStride[1] >> 2 });
      internal::writePM4(SetContextReg { Register::VGT_STRMOUT_VTX_STRIDE_2, shader->streamOutStride[2] >> 2 });
      internal::writePM4(SetContextReg { Register::VGT_STRMOUT_VTX_STRIDE_3, shader->streamOutStride[3] >> 2 });
   }

   internal::writePM4(SetContextReg { Register::VGT_STRMOUT_BUFFER_EN, vgt_strmout_buffer_en.value });

   // Setup vertex shader data
   auto sq_pgm_resources_vs = shader->regs.sq_pgm_resources_vs.value();
   auto num_spi_vs_out_id = shader->regs.num_spi_vs_out_id.value();
   auto spi_vs_out_id = shader->regs.spi_vs_out_id.value();
   auto spi_vs_out_config = shader->regs.spi_vs_out_config.value();
   auto pa_cl_vs_out_cntl = shader->regs.pa_cl_vs_out_cntl.value();

   auto vertexShaderProgAddr = virt_cast<virt_addr>(shader->vertexShaderData);
   auto vertexShaderProgSize = shader->vertexShaderSize;
   if (!vertexShaderProgAddr) {
      vertexShaderProgAddr = virt_cast<virt_addr>(shader->gx2rVertexShaderData.buffer);
      vertexShaderProgSize = shader->gx2rVertexShaderData.elemCount * shader->gx2rVertexShaderData.elemSize;
   }

   decaf_check(vertexShaderProgAddr);
   decaf_check(vertexShaderProgSize);

   uint32_t vertexShaderRegData[] = {
      vertexShaderProgAddr >> 8,
      vertexShaderProgSize >> 3,
      0,
      0,
      sq_pgm_resources_vs.value,
   };
   internal::writePM4(SetContextRegs { Register::SQ_PGM_START_VS, gsl::make_span(vertexShaderRegData) });
   internal::writePM4(SetContextReg { Register::PA_CL_VS_OUT_CNTL, pa_cl_vs_out_cntl.value });

   if (shader->regs.num_spi_vs_out_id > 0) {
      internal::writePM4(SetContextRegs {
         Register::SPI_VS_OUT_ID_0,
         gsl::make_span(&spi_vs_out_id[0].value, shader->regs.num_spi_vs_out_id)
      });
   }

   internal::writePM4(SetContextReg { Register::SPI_VS_OUT_CONFIG, spi_vs_out_config.value });
   internal::writePM4(SetContextReg { Register::SQ_GSVS_RING_ITEMSIZE, shader->ringItemSize });

   for (auto i = 0u; i < shader->loopVarCount; ++i) {
      auto id = static_cast<Register>(Register::SQ_LOOP_CONST_GS_0 + shader->loopVars[i].offset);
      internal::writePM4(SetLoopConst { id, shader->loopVars[i].value });
   }
}

void
_GX2SetSampler(virt_ptr<GX2Sampler> sampler,
               uint32_t id)
{
   internal::writePM4(SetSamplerAttrib {
      id * 3,
      sampler->regs.word0,
      sampler->regs.word1,
      sampler->regs.word2
   });
}

void
GX2SetPixelSampler(virt_ptr<GX2Sampler> sampler,
                   uint32_t id)
{
   decaf_check(id < 0x12);
   _GX2SetSampler(sampler, 0 + id);
}

void
GX2SetVertexSampler(virt_ptr<GX2Sampler> sampler,
                    uint32_t id)
{
   decaf_check(id < 0x12);
   _GX2SetSampler(sampler, 18 + id);
}

void
GX2SetGeometrySampler(virt_ptr<GX2Sampler> sampler,
                      uint32_t id)
{
   decaf_check(id < 0x12);
   _GX2SetSampler(sampler, 36 + id);
}

void
GX2SetVertexUniformReg(uint32_t offset,
                       uint32_t count,
                       virt_ptr<uint32_t> data)
{
   auto loop = offset >> 16;
   if (loop) {
      auto id = static_cast<Register>(Register::SQ_LOOP_CONST_VS_0 + 4 * loop);
      internal::writePM4(SetLoopConst { id, data[0] });
   }

   auto alu = offset & 0x7fff;
   auto id = static_cast<Register>(Register::SQ_ALU_CONSTANT0_256 + 4 * alu);

   // Custom write packet so we can endian swap data
   auto totalSize = 2 + count;
   auto buffer = internal::getCommandBuffer(totalSize);

   auto writer = latte::pm4::PacketWriter {
      buffer->buffer,
      buffer->curSize,
      SetAluConsts::Opcode,
      totalSize
   };

   writer.REG_OFFSET(id, Register::AluConstRegisterBase);

   for (auto i = 0u; i < count; ++i) {
      writer(data[i].value());
   }
}

void
GX2SetPixelUniformReg(uint32_t offset,
                      uint32_t count,
                      virt_ptr<uint32_t> data)
{
   auto loop = offset >> 16;

   if (loop) {
      auto id = static_cast<Register>(Register::SQ_LOOP_CONST_PS_0 + 4 * loop);
      internal::writePM4(SetLoopConst { id, data[0] });
   }

   auto alu = offset & 0x7fff;
   auto id = static_cast<Register>(Register::SQ_ALU_CONSTANT0_0 + 4 * alu);

   // Custom write packet so we can endian swap data
   auto totalSize = 2 + count;
   auto buffer = internal::getCommandBuffer(totalSize);

   auto writer = latte::pm4::PacketWriter {
      buffer->buffer,
      buffer->curSize,
      SetAluConsts::Opcode,
      totalSize
   };

   writer.REG_OFFSET(id, Register::AluConstRegisterBase);

   for (auto i = 0u; i < count; ++i) {
      writer(data[i].value());
   }
}

void
GX2SetVertexUniformBlock(uint32_t location,
                         uint32_t size,
                         virt_ptr<const void> data)
{
   decaf_check(!(virt_cast<virt_addr>(data) & 0xFF));

   SetVtxResource res;
   memset(&res, 0, sizeof(SetVtxResource));
   res.id = (latte::SQ_RES_OFFSET::VS_BUF_RESOURCE_0 + location) * 7;
   res.baseAddress = data.getRawPointer();

   res.word1 = res.word1
      .SIZE(size - 1);

   res.word2 = res.word2
      .STRIDE(16)
      .DATA_FORMAT(latte::SQ_DATA_FORMAT::FMT_32_32_32_32)
      .FORMAT_COMP_ALL(latte::SQ_FORMAT_COMP::SIGNED);

   res.word3 = res.word3
      .MEM_REQUEST_SIZE(1);

   res.word6 = res.word6
      .TYPE(latte::SQ_TEX_VTX_TYPE::VALID_BUFFER);

   internal::writePM4(res);

   auto addrId = static_cast<Register>(Register::SQ_ALU_CONST_CACHE_VS_0 + location * 4);
   auto addr256 = virt_cast<virt_addr>(data) >> 8;
   internal::writePM4(SetContextReg { addrId, addr256 });

   auto sizeId = static_cast<Register>(Register::SQ_ALU_CONST_BUFFER_SIZE_VS_0 + location * 4);
   auto size256 = ((size + 255) >> 8) & 0x1FF;
   internal::writePM4(SetContextReg { sizeId, size256 });
}

void
GX2SetPixelUniformBlock(uint32_t location,
                        uint32_t size,
                        virt_ptr<const void> data)
{
   decaf_check(!(virt_cast<virt_addr>(data) & 0xFF));

   SetVtxResource res;
   memset(&res, 0, sizeof(SetVtxResource));
   res.id = (latte::SQ_RES_OFFSET::PS_BUF_RESOURCE_0 + location) * 7;
   res.baseAddress = data.getRawPointer();

   res.word1 = res.word1
      .SIZE(size - 1);

   res.word2 = res.word2
      .STRIDE(16)
      .DATA_FORMAT(latte::SQ_DATA_FORMAT::FMT_32_32_32_32)
      .FORMAT_COMP_ALL(latte::SQ_FORMAT_COMP::SIGNED);

   res.word3 = res.word3
      .MEM_REQUEST_SIZE(1);

   res.word6 = res.word6
      .TYPE(latte::SQ_TEX_VTX_TYPE::VALID_BUFFER);

   internal::writePM4(res);

   auto addrId = static_cast<Register>(Register::SQ_ALU_CONST_CACHE_PS_0 + location * 4);
   auto addr256 = virt_cast<virt_addr>(data) >> 8;
   internal::writePM4(SetContextReg { addrId, addr256 });

   auto sizeId = static_cast<Register>(Register::SQ_ALU_CONST_BUFFER_SIZE_PS_0 + location * 4);
   auto size256 = ((size + 255) >> 8) & 0x1FF;
   internal::writePM4(SetContextReg { sizeId, size256 });
}

void
GX2SetGeometryUniformBlock(uint32_t location,
                           uint32_t size,
                           virt_ptr<const void> data)
{
   decaf_check(!(virt_cast<virt_addr>(data) & 0xFF));

   SetVtxResource res;
   res.id = (latte::SQ_RES_OFFSET::GS_BUF_RESOURCE_0 + location) * 7;
   memset(&res, 0, sizeof(SetVtxResource));
   res.baseAddress = data.getRawPointer();

   res.word1 = res.word1
      .SIZE(size - 1);

   res.word2 = res.word2
      .STRIDE(16)
      .DATA_FORMAT(latte::SQ_DATA_FORMAT::FMT_32_32_32_32)
      .FORMAT_COMP_ALL(latte::SQ_FORMAT_COMP::SIGNED);

   res.word3 = res.word3
      .MEM_REQUEST_SIZE(1);

   res.word6 = res.word6
      .TYPE(latte::SQ_TEX_VTX_TYPE::VALID_BUFFER);

   internal::writePM4(res);

   auto addrId = static_cast<Register>(Register::SQ_ALU_CONST_CACHE_GS_0 + location * 4);
   auto addr256 = virt_cast<virt_addr>(data) >> 8;
   internal::writePM4(SetContextReg { addrId, addr256 });

   auto sizeId = static_cast<Register>(Register::SQ_ALU_CONST_BUFFER_SIZE_GS_0 + location * 4);
   auto size256 = ((size + 255) >> 8) & 0x1FF;
   internal::writePM4(SetContextReg { sizeId, size256 });
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
            .MODE(latte::VGT_GS_ENABLE_MODE::SCENARIO_G)
            .COMPUTE_MODE(1)
            .FAST_COMPUTE_MODE(1)
            .PARTIAL_THD_AT_EOI(1);
      } else {
         vgt_gs_mode = vgt_gs_mode
            .MODE(latte::VGT_GS_ENABLE_MODE::OFF);
      }

      internal::writePM4(SetContextReg { Register::VGT_GS_MODE, vgt_gs_mode.value });
   }

   if (mode == GX2ShaderMode::ComputeShader) {
      sq_config = sq_config
         .ALU_INST_PREFER_VECTOR(1)
         .PS_PRIO(0)
         .VS_PRIO(1)
         .GS_PRIO(2)
         .ES_PRIO(3);
   } else {
      sq_config = sq_config
         .ALU_INST_PREFER_VECTOR(1)
         .PS_PRIO(3)
         .VS_PRIO(2)
         .GS_PRIO(1)
         .ES_PRIO(0);
   }

   if (mode == GX2ShaderMode::UniformRegister) {
      sq_config = sq_config
         .DX9_CONSTS(1);
   }

   if (mode == GX2ShaderMode::GeometryShader) {
      sq_gpr_resource_mgmt_1 = sq_gpr_resource_mgmt_1
         .NUM_PS_GPRS(numPsGpr)
         .NUM_VS_GPRS(64)
         .NUM_CLAUSE_TEMP_GPRS(4);

      sq_gpr_resource_mgmt_2 = sq_gpr_resource_mgmt_2
         .NUM_ES_GPRS(numVsGpr)
         .NUM_GS_GPRS(numGsGpr);

      sq_stack_resource_mgmt_1 = sq_stack_resource_mgmt_1
         .NUM_PS_STACK_ENTRIES(numPsStackEntries);

      sq_stack_resource_mgmt_2
         .NUM_ES_STACK_ENTRIES(numVsStackEntries);

      sq_stack_resource_mgmt_2
         .NUM_GS_STACK_ENTRIES(numGsStackEntries);

      sq_thread_resource_mgmt = sq_thread_resource_mgmt
         .NUM_PS_THREADS(124)
         .NUM_VS_THREADS(32)
         .NUM_GS_THREADS(8)
         .NUM_ES_THREADS(28);
   } else if (mode == GX2ShaderMode::ComputeShader) {
      sq_gpr_resource_mgmt_1 = sq_gpr_resource_mgmt_1
         .NUM_CLAUSE_TEMP_GPRS(4);

      sq_gpr_resource_mgmt_2 = sq_gpr_resource_mgmt_2
         .NUM_ES_GPRS(248);

      sq_stack_resource_mgmt_2 = sq_stack_resource_mgmt_2
         .NUM_ES_STACK_ENTRIES(256);

      sq_thread_resource_mgmt = sq_thread_resource_mgmt
         .NUM_PS_THREADS(1)
         .NUM_VS_THREADS(1)
         .NUM_GS_THREADS(1)
         .NUM_ES_THREADS(189);
   } else {
      sq_gpr_resource_mgmt_1 = sq_gpr_resource_mgmt_1
         .NUM_PS_GPRS(numPsGpr)
         .NUM_VS_GPRS(numVsGpr);

      sq_stack_resource_mgmt_1 = sq_stack_resource_mgmt_1
         .NUM_PS_STACK_ENTRIES(numPsStackEntries)
         .NUM_VS_STACK_ENTRIES(numVsStackEntries);

      sq_thread_resource_mgmt = sq_thread_resource_mgmt
         .NUM_PS_THREADS(136)
         .NUM_VS_THREADS(48)
         .NUM_GS_THREADS(4)
         .NUM_ES_THREADS(4);
   }

   uint32_t regData[] = {
      sq_config.value,
      sq_gpr_resource_mgmt_1.value,
      sq_gpr_resource_mgmt_2.value,
      sq_thread_resource_mgmt.value,
      sq_stack_resource_mgmt_1.value,
      sq_stack_resource_mgmt_2.value,
   };
   internal::writePM4(SetConfigRegs { Register::SQ_CONFIG, gsl::make_span(regData) });

   if (mode == GX2ShaderMode::ComputeShader) {
      uint32_t ringBaseData[] = { 0, 0xFFFFFF, 0, 0xFFFFFF };
      internal::writePM4(SetConfigRegs { Register::SQ_ESGS_RING_BASE, gsl::make_span(ringBaseData) });

      uint32_t ringItemSizes[] = { 0, 1 };
      internal::writePM4(SetContextRegs { Register::SQ_ESGS_RING_ITEMSIZE, gsl::make_span(ringItemSizes) });

      internal::writePM4(SetContextReg { Register::VGT_STRMOUT_EN, 0 });
   }
}

void
GX2SetStreamOutBuffer(uint32_t index,
                      virt_ptr<GX2OutputStream> stream)
{
   decaf_check(index <= 3);
   decaf_check(virt_cast<virt_addr>(stream->buffer) % 256 == 0);
   decaf_check(stream->stride % 4 == 0);
   decaf_check(stream->size % 4 == 0);

   auto addr = virt_cast<virt_addr>(stream->buffer);
   auto size = stream->size;

   if (!addr) {
      addr = virt_cast<virt_addr>(stream->gx2rData.buffer);
      size = stream->gx2rData.elemCount * stream->gx2rData.elemSize;
   }

   internal::writePM4(SetContextReg { static_cast<Register>(Register::VGT_STRMOUT_BUFFER_SIZE_0 + 16 * index), size >> 2 });
   internal::writePM4(SetContextReg { static_cast<Register>(Register::VGT_STRMOUT_BUFFER_BASE_0 + 16 * index), addr >> 8 });
   internal::writePM4(StreamOutBaseUpdate { index, addr >> 8 });
}

void
GX2SetStreamOutEnable(BOOL enable)
{
   auto vgt_strmout_en = latte::VGT_STRMOUT_EN::get(0);

   vgt_strmout_en = vgt_strmout_en
      .STREAMOUT(!!enable);

   internal::writePM4(SetContextReg { Register::VGT_STRMOUT_EN, vgt_strmout_en.value });
}

void
GX2SetStreamOutContext(uint32_t index,
                       virt_ptr<GX2OutputStream> stream,
                       GX2StreamOutContextMode mode)
{
   decaf_check(index <= 3);
   auto srcLo = 0u;

   auto control = SBU_CONTROL::get(0)
      .STORE_BUFFER_FILLED_SIZE(false)
      .SELECT_BUFFER(index);

   switch (mode) {
   case GX2StreamOutContextMode::Append:
      control = control
         .OFFSET_SOURCE(STRMOUT_OFFSET_FROM_MEM);
      srcLo = static_cast<uint32_t>(virt_cast<virt_addr>(stream->context));
      break;
   case GX2StreamOutContextMode::FromStart:
      control = control
         .OFFSET_SOURCE(STRMOUT_OFFSET_FROM_PACKET);
      srcLo = 0u;
      break;
   case GX2StreamOutContextMode::FromOffset:
      control = control
         .OFFSET_SOURCE(STRMOUT_OFFSET_FROM_PACKET);
      srcLo = static_cast<uint32_t>(virt_cast<virt_addr>(stream));
      break;
   }

   internal::writePM4(StreamOutBufferUpdate { control, 0, 0, srcLo, 0 });
}

void
GX2SaveStreamOutContext(uint32_t index,
                        virt_ptr<GX2OutputStream> stream)
{
   decaf_check(index <= 3);
   auto dstLo = static_cast<uint32_t>(virt_cast<virt_addr>(stream->context));

   auto control = SBU_CONTROL::get(0)
      .STORE_BUFFER_FILLED_SIZE(true)
      .OFFSET_SOURCE(STRMOUT_OFFSET_NONE)
      .SELECT_BUFFER(index);

   internal::writePM4(StreamOutBufferUpdate { control, dstLo, 0, 0, 0 });
}

void
GX2SetGeometryShaderInputRingBuffer(virt_ptr<void> buffer,
                                    uint32_t size)
{
   internal::writePM4(SetConfigReg { Register::SQ_ESGS_RING_BASE, virt_cast<virt_addr>(buffer) >> 8 });
   internal::writePM4(SetConfigReg { Register::SQ_ESGS_RING_SIZE, size >> 8 });

   SetVtxResource res;
   memset(&res, 0, sizeof(SetVtxResource));
   res.id = latte::SQ_RES_OFFSET::GS_GSIN_RESOURCE * 7;
   res.baseAddress = buffer.getRawPointer();

   res.word1 = res.word1
      .SIZE(size - 1);

   res.word2 = res.word2
      .STRIDE(4)
      .CLAMP_X(latte::SQ_VTX_CLAMP::TO_NAN)
      .DATA_FORMAT(latte::SQ_DATA_FORMAT::FMT_32_32_32_32_FLOAT);

   res.word3 = res.word3
      .UNCACHED(1);

   res.word6 = res.word6
      .TYPE(latte::SQ_TEX_VTX_TYPE::VALID_BUFFER);

   internal::writePM4(res);
}

void
GX2SetGeometryShaderOutputRingBuffer(virt_ptr<void> buffer,
                                     uint32_t size)
{
   internal::writePM4(SetConfigReg { Register::SQ_GSVS_RING_BASE, virt_cast<virt_addr>(buffer) >> 8 });
   internal::writePM4(SetConfigReg { Register::SQ_GSVS_RING_SIZE, size >> 8 });

   SetVtxResource res;
   memset(&res, 0, sizeof(SetVtxResource));
   res.id = latte::SQ_RES_OFFSET::VS_GSOUT_RESOURCE * 7;
   res.baseAddress = buffer.getRawPointer();

   res.word1 = res.word1
      .SIZE(size - 1);

   res.word2 = res.word2
      .STRIDE(4)
      .CLAMP_X(latte::SQ_VTX_CLAMP::TO_NAN)
      .DATA_FORMAT(latte::SQ_DATA_FORMAT::FMT_32_32_32_32_FLOAT);

   res.word3 = res.word3
      .UNCACHED(1);

   res.word6 = res.word6
      .TYPE(latte::SQ_TEX_VTX_TYPE::VALID_BUFFER);

   internal::writePM4(res);
}

uint32_t
GX2GetPixelShaderGPRs(virt_ptr<GX2PixelShader> shader)
{
   return shader->regs.sq_pgm_resources_ps.value().NUM_GPRS();
}

uint32_t
GX2GetPixelShaderStackEntries(virt_ptr<GX2PixelShader> shader)
{
   return shader->regs.sq_pgm_resources_ps.value().STACK_SIZE();
}

uint32_t
GX2GetVertexShaderGPRs(virt_ptr<GX2VertexShader> shader)
{
   return shader->regs.sq_pgm_resources_vs.value().NUM_GPRS();
}

uint32_t
GX2GetVertexShaderStackEntries(virt_ptr<GX2VertexShader> shader)
{
   return shader->regs.sq_pgm_resources_vs.value().STACK_SIZE();
}

uint32_t
GX2GetGeometryShaderGPRs(virt_ptr<GX2GeometryShader> shader)
{
   return shader->regs.sq_pgm_resources_gs.value().NUM_GPRS();
}

uint32_t
GX2GetGeometryShaderStackEntries(virt_ptr<GX2GeometryShader> shader)
{
   return shader->regs.sq_pgm_resources_gs.value().STACK_SIZE();
}

void
Library::registerShadersSymbols()
{
   RegisterFunctionExport(GX2CalcGeometryShaderInputRingBufferSize);
   RegisterFunctionExport(GX2CalcGeometryShaderOutputRingBufferSize);
   RegisterFunctionExport(GX2SetFetchShader);
   RegisterFunctionExport(GX2SetVertexShader);
   RegisterFunctionExport(GX2SetPixelShader);
   RegisterFunctionExport(GX2SetGeometryShader);
   RegisterFunctionExport(GX2SetVertexSampler);
   RegisterFunctionExport(GX2SetPixelSampler);
   RegisterFunctionExport(GX2SetGeometrySampler);
   RegisterFunctionExport(GX2SetVertexUniformReg);
   RegisterFunctionExport(GX2SetPixelUniformReg);
   RegisterFunctionExport(GX2SetVertexUniformBlock);
   RegisterFunctionExport(GX2SetPixelUniformBlock);
   RegisterFunctionExport(GX2SetGeometryUniformBlock);
   RegisterFunctionExport(GX2SetShaderModeEx);
   RegisterFunctionExport(GX2SetStreamOutBuffer);
   RegisterFunctionExport(GX2SetStreamOutEnable);
   RegisterFunctionExport(GX2SetStreamOutContext);
   RegisterFunctionExport(GX2SaveStreamOutContext);
   RegisterFunctionExport(GX2SetGeometryShaderInputRingBuffer);
   RegisterFunctionExport(GX2SetGeometryShaderOutputRingBuffer);
   RegisterFunctionExport(GX2GetPixelShaderGPRs);
   RegisterFunctionExport(GX2GetPixelShaderStackEntries);
   RegisterFunctionExport(GX2GetVertexShaderGPRs);
   RegisterFunctionExport(GX2GetVertexShaderStackEntries);
   RegisterFunctionExport(GX2GetGeometryShaderGPRs);
   RegisterFunctionExport(GX2GetGeometryShaderStackEntries);
}

} // namespace cafe::gx2
