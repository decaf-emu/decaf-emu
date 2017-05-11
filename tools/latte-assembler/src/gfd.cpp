#include "gfd_comment_parser.h"
#include <cstring>
#include <vector>
#include <libgfd/gfd.h>
#include <regex>

/*
Matches:
; $Something = true
; $attribVars[1].type = "Float4"
; $VGT_HOS_REUSE_DEPTH.REUSE_DEPTH = 16
; $SQ_VTX_SEMANTIC_CLEAR.CLEAR = 0xFFFFFFFC
*/
static std::regex
sCommentKeyValueRegex
{
   ";[[:space:]]*\\$([_[:alnum:]]+)(?:\\[([[:digit:]]+)\\])?(?:\\.([_[:alnum:]]+))?[[:space:]]*=[[:space:]]*(\"[^\"]+\"|[0-9]+|0x[0-9a-fA-F]+|true|false|TRUE|FALSE)"
};

static std::regex
sCommentKeyValueStartRegex
{
   ";[[:space:]]*\\$"
};

bool
parseComment(const std::string &comment,
             CommentKeyValue &out)
{
   std::smatch match;
   if (!std::regex_match(comment, match, sCommentKeyValueRegex)) {
      if (std::regex_match(comment, match, sCommentKeyValueStartRegex)) {
         throw gfd_header_parse_exception { fmt::format("Syntax error in comment {}", comment) };
      }

      return false;
   }

   out.obj = match[1];
   out.index = match[2];
   out.member = match[3];
   out.value = match[4];

   if (out.value.size() >= 2 && out.value[0] == '"') {
      // Erase quotes from string value
      out.value.erase(out.value.begin());
      out.value.erase(out.value.end() - 1);
   }

   return true;
}

void
ensureArrayOfObjects(const CommentKeyValue &kv)
{
   if (!kv.isArrayOfObjects()) {
      throw gfd_header_parse_exception { fmt::format("{} is an array of objects", kv.obj) };
   }
}

void
ensureArrayOfValues(const CommentKeyValue &kv)
{
   if (!kv.isArrayOfValues()) {
      throw gfd_header_parse_exception { fmt::format("{} is an array of values", kv.obj) };
   }
}

void
ensureObject(const CommentKeyValue &kv)
{
   if (!kv.isObject()) {
      throw gfd_header_parse_exception { fmt::format("{} is an object", kv.obj) };
   }
}

void
ensureValue(const CommentKeyValue &kv)
{
   if (!kv.isValue()) {
      throw gfd_header_parse_exception { fmt::format("{} is a value", kv.obj) };
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
      throw gfd_header_parse_exception { fmt::format("Invalid GX2ShaderVarType {}", value) };
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
      throw gfd_header_parse_exception { fmt::format("Invalid GX2ShaderMode {}", value) };
   }
}

bool
parseValueBool(const std::string &v)
{
   auto value = v;
   std::transform(value.begin(), value.end(), value.begin(), ::toupper);

   if (value == "TRUE") {
      return true;
   } else if (value == "FALSE") {
      return true;
   } else {
      throw gfd_header_parse_exception { fmt::format("Expected boolean value, found {}", value) };
   }
}

uint32_t
parseValueNumber(const std::string &v)
{
   return static_cast<uint32_t>(std::stoul(v, 0, 0));
}

static std::vector<uint8_t>
getShaderBinary(Shader &shader)
{
   std::vector<uint8_t> binary;
   static_assert(sizeof(shader.cfInsts[0]) == 8);
   auto cfStart = 0ull;
   auto cfSize = shader.cfInsts.size() * sizeof(shader.cfInsts[0]);
   auto cfEnd = cfStart + cfSize;

   auto aluStart = shader.aluClauseBaseAddress * 8;
   auto aluSize = shader.aluClauseData.size() * sizeof(shader.aluClauseData[0]);
   auto aluEnd = aluStart + aluSize;

   auto texStart = shader.texClauseBaseAddress * 8;
   auto texSize = shader.texClauseData.size() * sizeof(shader.texClauseData[0]);
   auto texEnd = texStart + texSize;

   binary.resize(std::max({ cfEnd, aluEnd, texEnd }), 0);

   std::memcpy(binary.data() + cfStart, shader.cfInsts.data(), cfSize);
   std::memcpy(binary.data() + aluStart, shader.aluClauseData.data(), aluSize);
   std::memcpy(binary.data() + texStart, shader.texClauseData.data(), texSize);
   return std::move(binary);
}

static uint32_t
countNumGpr(Shader &shader)
{
   auto highestRead = uint32_t { 0 };
   auto highestWritten = uint32_t { 0 };

   for (auto i = 0u; i < shader.gprRead.size(); ++i) {
      if (!shader.gprRead[i]) {
         continue;
      }

      if (i >= (latte::SQ_ALU_SRC::REGISTER_TEMP_FIRST - latte::SQ_ALU_SRC::REGISTER_FIRST)) {
         // Ignore temporary registers
         continue;
      }

      highestRead = std::max<uint32_t>(highestRead, i);
   }

   for (auto i = 0u; i < shader.gprWritten.size(); ++i) {
      if (!shader.gprWritten[i]) {
         continue;
      }

      if (i >= (latte::SQ_ALU_SRC::REGISTER_TEMP_FIRST - latte::SQ_ALU_SRC::REGISTER_FIRST)) {
         // Ignore temporary registers
         continue;
      }

      highestWritten = std::max<uint32_t>(highestWritten, i);
   }

   return std::max(highestRead, highestWritten) + 1;
}

bool
gfdAddVertexShader(gfd::GFDFile &file,
                   Shader &shader)
{
   auto out = gfd::GFDVertexShader {};
   auto numGpr = countNumGpr(shader);

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

   out.regs.sq_pgm_resources_vs = out.regs.sq_pgm_resources_vs
      .NUM_GPRS(numGpr)
      .STACK_SIZE(1);

   out.regs.vgt_hos_reuse_depth = out.regs.vgt_hos_reuse_depth
      .REUSE_DEPTH(16);

   out.regs.vgt_vertex_reuse_block_cntl = out.regs.vgt_vertex_reuse_block_cntl
      .VTX_REUSE_DEPTH(14);

   // Create binary
   out.data = getShaderBinary(shader);

   // Parse shader comments
   parseShaderComments(out, shader.comments);

   // NUM_GPRS should be the number of GPRs used in the shader
   if (out.regs.sq_pgm_resources_vs.NUM_GPRS() != numGpr) {
      throw gfd_header_parse_exception { fmt::format("Invalid SQ_PGM_RESOURCES_VS.NUM_GPRS {}, expected {}",
                                                     out.regs.sq_pgm_resources_vs.NUM_GPRS(),
                                                     numGpr) };
   }

   // NUM_SQ_VTX_SEMANTIC should reflect the size of ATTRIB_VARS array
   if (out.regs.num_sq_vtx_semantic == 0) {
      out.regs.num_sq_vtx_semantic = static_cast<uint32_t>(out.attribVars.size());

      for (auto i = 0u; i < out.regs.num_sq_vtx_semantic; ++i) {
         out.regs.sq_vtx_semantic[i] = out.regs.sq_vtx_semantic[i]
            .SEMANTIC_ID(out.attribVars[i].location);
      }
   } else if (out.regs.num_sq_vtx_semantic != out.attribVars.size()) {
      throw gfd_header_parse_exception { fmt::format("Invalid NUM_SQ_VTX_SEMANTIC {}, expected {}",
                                                     out.regs.num_sq_vtx_semantic,
                                                     numGpr) };
   }

   // SQ_VTX_SEMANTIC_CLEAR.CLEAR should reflect the value of NUM_SQ_VTX_SEMANTIC
   auto semanticClear = static_cast<uint32_t>(~((1 << out.regs.num_sq_vtx_semantic) - 1));
   if (out.regs.sq_vtx_semantic_clear.CLEAR() == 0) {
      out.regs.sq_vtx_semantic_clear = out.regs.sq_vtx_semantic_clear
         .CLEAR(semanticClear);
   } else if (out.regs.sq_vtx_semantic_clear.CLEAR() != semanticClear) {
      throw gfd_header_parse_exception { fmt::format("Invalid SQ_VTX_SEMANTIC_CLEAR {}, expected {}",
                                                     out.regs.num_sq_vtx_semantic,
                                                     numGpr) };
   }

   file.vertexShaders.push_back(out);
   return true;
}

bool
gfdAddPixelShader(gfd::GFDFile &file,
                  Shader &shader)
{
   auto out = gfd::GFDPixelShader {};
   auto numGpr = countNumGpr(shader);

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

   out.regs.sq_pgm_resources_ps = out.regs.sq_pgm_resources_ps
      .NUM_GPRS(numGpr)
      .STACK_SIZE(1);

   // Create binary
   out.data = getShaderBinary(shader);

   // Parse shader comments
   parseShaderComments(out, shader.comments);

   // NUM_GPRS should be the number of GPRs used in the shader
   if (out.regs.sq_pgm_resources_ps.NUM_GPRS() != numGpr) {
      throw gfd_header_parse_exception { fmt::format("Invalid SQ_PGM_RESOURCES_PS.NUM_GPRS {}, expected {}",
                                                     out.regs.sq_pgm_resources_ps.NUM_GPRS(),
                                                     numGpr) };
   }

   if (out.regs.spi_ps_in_control_0.NUM_INTERP() == 0) {
      out.regs.spi_ps_in_control_0 = out.regs.spi_ps_in_control_0
         .NUM_INTERP(out.regs.num_spi_ps_input_cntl);
   } else if (out.regs.spi_ps_in_control_0.NUM_INTERP() != out.regs.num_spi_ps_input_cntl) {
      throw gfd_header_parse_exception { fmt::format("Expected SPI_PS_IN_CONTROL_0.NUM_INTERP {} to equal NUM_SPI_PS_INPUT_CNTL {}",
                                                     out.regs.spi_ps_in_control_0.NUM_INTERP(),
                                                     out.regs.num_spi_ps_input_cntl) };
   }

   file.pixelShaders.push_back(out);
   return true;
}
