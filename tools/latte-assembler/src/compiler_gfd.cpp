#include "shader_compiler.h"
#include <cstring>
#include <vector>
#include <libgfd/gfd.h>

void
ensureArrayOfObjects(const CommentKeyValue &kv)
{
   if (!kv.isArrayOfObjects()) {
      throw parse_exception(fmt::format("{} is an array of objects", kv.obj));
   }
}

void
ensureArrayOfValues(const CommentKeyValue &kv)
{
   if (!kv.isArrayOfValues()) {
      throw parse_exception(fmt::format("{} is an array of values", kv.obj));
   }
}

void
ensureObject(const CommentKeyValue &kv)
{
   if (!kv.isObject()) {
      throw parse_exception(fmt::format("{} is an object", kv.obj));
   }
}

void
ensureValue(const CommentKeyValue &kv)
{
   if (!kv.isValue()) {
      throw parse_exception(fmt::format("{} is a value", kv.obj));
   }
}

gx2::GX2ShaderVarType
parseShaderVarType(const std::string &v)
{
   auto value = v;
   std::transform(value.begin(), value.end(), value.begin(), ::toupper);

   if (value == "INT") {
      return gx2::GX2ShaderVarType::Int;
   } else if (value == "INT2") {
      return gx2::GX2ShaderVarType::Int2;
   } else if (value == "INT3") {
      return gx2::GX2ShaderVarType::Int3;
   } else if (value == "INT4") {
      return gx2::GX2ShaderVarType::Int4;
   } else if (value == "FLOAT") {
      return gx2::GX2ShaderVarType::Float;
   } else if (value == "FLOAT2") {
      return gx2::GX2ShaderVarType::Float2;
   } else if (value == "FLOAT3") {
      return gx2::GX2ShaderVarType::Float3;
   } else if (value == "FLOAT4") {
      return gx2::GX2ShaderVarType::Float4;
   } else if (value == "MATRIX4X4") {
      return gx2::GX2ShaderVarType::Matrix4x4;
   } else {
      throw parse_exception { fmt::format("Invalid GX2ShaderVarType {}", value) };
   }
}

gx2::GX2ShaderMode
parseShaderMode(const std::string &v)
{
   auto value = v;
   std::transform(value.begin(), value.end(), value.begin(), ::toupper);

   if (value == "UNIFORMREGISTER") {
      return gx2::GX2ShaderMode::UniformRegister;
   } else if (value == "UNIFORMBLOCK") {
      return gx2::GX2ShaderMode::UniformBlock;
   } else if (value == "GEOMETRYSHADER") {
      return gx2::GX2ShaderMode::GeometryShader;
   } else if (value == "COMPUTERSHADER") {
      return gx2::GX2ShaderMode::ComputeShader;
   } else {
      throw parse_exception { fmt::format("Invalid GX2ShaderMode {}", value) };
   }
}

static std::vector<uint8_t>
getShaderBinary(Shader &shader)
{
   std::vector<uint8_t> binary;
   binary.resize((shader.aluClauseBaseAddress * 8) + (shader.aluClauseData.size() * 4), 0);

   static_assert(sizeof(shader.cfInsts[0]) == 8);
   std::memcpy(binary.data(), shader.cfInsts.data(), shader.cfInsts.size() * 8);

   static_assert(sizeof(shader.aluClauseData[0]) == 4);
   std::memcpy(binary.data() + (shader.aluClauseBaseAddress * 8), shader.aluClauseData.data(), shader.aluClauseData.size() * 4);

   return std::move(binary);
}

bool
gfdAddVertexShader(gfd::GFDFile &file,
                   Shader &shader)
{
   auto out = gfd::GFDVertexShader {};
   // Initialise some default values
   out.mode = gx2::GX2ShaderMode::UniformRegister;
   out.ringItemSize = 0;
   out.hasStreamOut = false;
   out.streamOutStride.fill(0);
   out.gx2rData.elemCount = 0;
   out.gx2rData.elemSize = 0;
   out.gx2rData.flags = static_cast<gx2::GX2RResourceFlags>(0);

   std::memset(&out.regs, 0, sizeof(out.regs));
   out.regs.spi_vs_out_id.fill(latte::SPI_VS_OUT_ID_N::get(0xFFFFFFFF));

   out.regs.vgt_hos_reuse_depth = out.regs.vgt_hos_reuse_depth
      .REUSE_DEPTH(16);

   out.regs.vgt_vertex_reuse_block_cntl = out.regs.vgt_vertex_reuse_block_cntl
      .VTX_REUSE_DEPTH(14);

   // Create binary
   out.data = getShaderBinary(shader);

   // Parse shader comments
   parseShaderComments(out, shader.comments);
   file.vertexShaders.push_back(out);
   return true;
}

bool
gfdAddPixelShader(gfd::GFDFile &file,
                  Shader &shader)
{
   auto out = gfd::GFDPixelShader {};
   // Initialise some default values
   out.mode = gx2::GX2ShaderMode::UniformRegister;
   out.gx2rData.elemCount = 0;
   out.gx2rData.elemSize = 0;
   out.gx2rData.flags = static_cast<gx2::GX2RResourceFlags>(0);

   std::memset(&out.regs, 0, sizeof(out.regs));
   out.regs.cb_shader_mask = out.regs.cb_shader_mask
      .OUTPUT0_ENABLE(0b1111);

   out.regs.cb_shader_control = out.regs.cb_shader_control
      .RT0_ENABLE(true);

   out.regs.db_shader_control = out.regs.db_shader_control
      .Z_ORDER(latte::DB_Z_ORDER::EARLY_Z_THEN_LATE_Z);

   out.regs.spi_ps_in_control_0 = out.regs.spi_ps_in_control_0
      .BARYC_SAMPLE_CNTL(latte::SPI_BARYC_CNTL::CENTERS_ONLY)
      .PERSP_GRADIENT_ENA(true);

   out.regs.sq_pgm_exports_ps = out.regs.sq_pgm_exports_ps
      .EXPORT_MODE(2);

   // Create binary
   out.data = getShaderBinary(shader);

   // Parse shader comments
   parseShaderComments(out, shader.comments);
   file.pixelShaders.push_back(out);
   return true;
}
