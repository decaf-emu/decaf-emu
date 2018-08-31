#include "gfd_comment_parser.h"
#include <libgpu/latte/latte_constants.h>

#include <cstring>
#include <fmt/format.h>
#include <libgfd/gfd.h>
#include <regex>
#include <vector>

using namespace cafe::gx2;

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
         throw gfd_header_parse_exception {
            fmt::format("Syntax error in comment {}", comment)
         };
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
      throw gfd_header_parse_exception {
         fmt::format("{} is an array of objects", kv.obj)
      };
   }
}

void
ensureArrayOfValues(const CommentKeyValue &kv)
{
   if (!kv.isArrayOfValues()) {
      throw gfd_header_parse_exception {
         fmt::format("{} is an array of values", kv.obj)
      };
   }
}

void
ensureObject(const CommentKeyValue &kv)
{
   if (!kv.isObject()) {
      throw gfd_header_parse_exception {
         fmt::format("{} is an object", kv.obj)
      };
   }
}

void
ensureValue(const CommentKeyValue &kv)
{
   if (!kv.isValue()) {
      throw gfd_header_parse_exception {
         fmt::format("{} is a value", kv.obj)
      };
   }
}

GX2ShaderVarType
parseShaderVarType(const std::string &v)
{
   auto value = v;
   std::transform(value.begin(), value.end(), value.begin(), ::toupper);

   if (value == "INT") {
      return GX2ShaderVarType::Int;
   } else if (value == "INT2") {
      return GX2ShaderVarType::Int2;
   } else if (value == "INT3") {
      return GX2ShaderVarType::Int3;
   } else if (value == "INT4") {
      return GX2ShaderVarType::Int4;
   } else if (value == "FLOAT") {
      return GX2ShaderVarType::Float;
   } else if (value == "FLOAT2") {
      return GX2ShaderVarType::Float2;
   } else if (value == "FLOAT3") {
      return GX2ShaderVarType::Float3;
   } else if (value == "FLOAT4") {
      return GX2ShaderVarType::Float4;
   } else if (value == "MATRIX4X4") {
      return GX2ShaderVarType::Matrix4x4;
   } else {
      throw gfd_header_parse_exception {
         fmt::format("Invalid GX2ShaderVarType {}", value)
      };
   }
}

GX2SamplerVarType
parseSamplerVarType(const std::string &v)
{
   auto value = v;
   std::transform(value.begin(), value.end(), value.begin(), ::toupper);

   if (value == "SAMPLER1D") {
      return GX2SamplerVarType::Sampler1D;
   } else if (value == "SAMPLER2D") {
      return GX2SamplerVarType::Sampler2D;
   } else if (value == "SAMPLER3D") {
      return GX2SamplerVarType::Sampler3D;
   } else if (value == "SAMPLERCUBE") {
      return GX2SamplerVarType::SamplerCube;
   } else {
      throw gfd_header_parse_exception {
         fmt::format("Invalid GX2SamplerVarType {}", value)
      };
   }
}

GX2ShaderMode
parseShaderMode(const std::string &v)
{
   auto value = v;
   std::transform(value.begin(), value.end(), value.begin(), ::toupper);

   if (value == "UNIFORMREGISTER") {
      return GX2ShaderMode::UniformRegister;
   } else if (value == "UNIFORMBLOCK") {
      return GX2ShaderMode::UniformBlock;
   } else if (value == "GEOMETRYSHADER") {
      return GX2ShaderMode::GeometryShader;
   } else if (value == "COMPUTERSHADER") {
      return GX2ShaderMode::ComputeShader;
   } else {
      throw gfd_header_parse_exception {
         fmt::format("Invalid GX2ShaderMode {}", value)
      };
   }
}

void
parseUniformBlocks(std::vector<gfd::GFDUniformBlock> &UniformBlocks,
                   uint32_t index,
                   const std::string &member,
                   const std::string &value)
{
   if (index >= latte::MaxUniformBlocks) {
      throw gfd_header_parse_exception {
         fmt::format("UNIFORM_BLOCKS[{}] invalid index, max: {}",
                     index, latte::MaxUniformBlocks)
      };
   }

   if (index >= UniformBlocks.size()) {
      UniformBlocks.resize(index + 1);
      UniformBlocks[index].offset = index + 1;
      UniformBlocks[index].size = 16;
   }

   if (member == "NAME") {
      UniformBlocks[index].name = value;
   } else if (member == "OFFSET") {
      UniformBlocks[index].offset = parseValueNumber(value);
   } else if (member == "SIZE") {
      UniformBlocks[index].size = parseValueNumber(value);
      if (UniformBlocks[index].size >= latte::MaxUniformBlockSize) {
         throw gfd_header_parse_exception {
            fmt::format("UNIFORM_BLOCKS[{}] invalid index, max: {}",
                        index, latte::MaxUniformBlocks)
         };
      }
   } else {
      throw gfd_header_parse_exception {
         fmt::format("UNIFORM_BLOCKS[{}] does not have member {}",
                     index, member)
      };
   }
}

void
parseUniformVars(std::vector<gfd::GFDUniformVar> &uniformVars,
                 uint32_t index,
                 const std::string &member,
                 const std::string &value)
{
   if (index >= latte::MaxUniformRegisters) {
      throw gfd_header_parse_exception {
         fmt::format("UNIFORM_VARS[{}] invalid index, max: {}",
                     index, latte::MaxUniformRegisters)
      };
   }

   if (index >= uniformVars.size()) {
      uniformVars.resize(index + 1);
      uniformVars[index].type = GX2ShaderVarType::Float4;
      uniformVars[index].count = 1;
   }

   if (member == "NAME") {
      uniformVars[index].name = value;
   } else if (member == "BLOCK") {
      uniformVars[index].block = parseValueNumber(value);
   } else if (member == "COUNT") {
      uniformVars[index].count = parseValueNumber(value);
   } else if (member == "OFFSET") {
      uniformVars[index].offset = parseValueNumber(value);
   } else if (member == "TYPE") {
      uniformVars[index].type = parseShaderVarType(value);
   } else {
      throw gfd_header_parse_exception {
         fmt::format("UNIFORM_VARS[{}] does not have member {}", index, member)
      };
   }
}

void
parseInitialValues(std::vector<gfd::GFDUniformInitialValue> &initialValues,
                   uint32_t index,
                   const std::string &member,
                   const std::string &value)
{
   if (index >= latte::MaxUniformRegisters) {
      throw gfd_header_parse_exception {
         fmt::format("INITIAL_VALUES[{}] invalid index, max: {}",
                     index, latte::MaxUniformRegisters)
      };
   }

   if (index >= initialValues.size()) {
      initialValues.resize(index + 1);
   }

   if (member == "OFFSET") {
      initialValues[index].offset = parseValueNumber(value);
   } else if (member == "VALUE[0]") {
      initialValues[index].value[0] = parseValueFloat(value);
   } else if (member == "VALUE[1]") {
      initialValues[index].value[1] = parseValueFloat(value);
   } else if (member == "VALUE[2]") {
      initialValues[index].value[2] = parseValueFloat(value);
   } else if (member == "VALUE[3]") {
      initialValues[index].value[3] = parseValueFloat(value);
   } else {
      throw gfd_header_parse_exception {
         fmt::format("INITIAL_VALUES[{}] does not have member {}",
                     index, member)
      };
   }
}

void
parseLoopVars(std::vector<gfd::GFDLoopVar> &loopVars,
              uint32_t index,
              const std::string &member,
              const std::string &value)
{
   if (index >= loopVars.size()) {
      loopVars.resize(index + 1);
      loopVars[index].offset = index;
   }

   if (member == "OFFSET") {
      loopVars[index].offset = parseValueNumber(value);
   } else if (member == "VALUE") {
      loopVars[index].value = parseValueNumber(value);
   } else {
      throw gfd_header_parse_exception {
         fmt::format("LOOP_VARS[{}] does not have member {}", index, member)
      };
   }
}

void
parseSamplerVars(std::vector<gfd::GFDSamplerVar> &samplerVars,
                 uint32_t index,
                 const std::string &member,
                 const std::string &value)
{
   if (index >= latte::MaxSamplers) {
      throw gfd_header_parse_exception {
         fmt::format("SAMPLER_VARS[{}] invalid index, max: {}",
                     index, latte::MaxSamplers)
      };
   }

   if (index >= samplerVars.size()) {
      samplerVars.resize(index + 1);
      samplerVars[index].type = GX2SamplerVarType::Sampler2D;
      samplerVars[index].location = index;
   }

   if (member == "NAME") {
      samplerVars[index].name = value;
   } else if (member == "LOCATION") {
      samplerVars[index].location = parseValueNumber(value);
   } else if (member == "TYPE") {
      samplerVars[index].type = parseSamplerVarType(value);
   } else {
      throw gfd_header_parse_exception {
         fmt::format("SAMPLER_VARS[{}] does not have member {}",
                     index, member)
      };
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
      return false;
   } else {
      throw gfd_header_parse_exception {
         fmt::format("Expected boolean value, found {}", value)
      };
   }
}

uint32_t
parseValueNumber(const std::string &v)
{
   return static_cast<uint32_t>(std::stoul(v, 0, 0));
}

float
parseValueFloat(const std::string &v)
{
   return static_cast<float>(std::stof(v));
}

static std::vector<uint8_t>
getShaderBinary(Shader &shader)
{
   std::vector<uint8_t> binary;
   auto cfStart = size_t { 0 };
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
   out.ringItemSize = 0;
   out.hasStreamOut = false;
   out.streamOutStride.fill(0);
   out.gx2rData.elemCount = 0;
   out.gx2rData.elemSize = 0;
   out.gx2rData.flags = static_cast<GX2RResourceFlags>(0);

   if (shader.uniformBlocksUsed) {
      out.mode = GX2ShaderMode::UniformBlock;
   } else {
      out.mode = GX2ShaderMode::UniformRegister;
   }

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
      throw gfd_header_parse_exception {
         fmt::format("Invalid SQ_PGM_RESOURCES_VS.NUM_GPRS {}, expected {}",
                     out.regs.sq_pgm_resources_vs.NUM_GPRS(), numGpr)
      };
   }

   // NUM_SQ_VTX_SEMANTIC should reflect the size of ATTRIB_VARS array
   if (out.regs.num_sq_vtx_semantic == 0) {
      out.regs.num_sq_vtx_semantic = static_cast<uint32_t>(out.attribVars.size());

      for (auto i = 0u; i < out.regs.num_sq_vtx_semantic; ++i) {
         out.regs.sq_vtx_semantic[i] = out.regs.sq_vtx_semantic[i]
            .SEMANTIC_ID(out.attribVars[i].location);
      }
   } else if (out.regs.num_sq_vtx_semantic != out.attribVars.size()) {
      throw gfd_header_parse_exception {
         fmt::format("Invalid NUM_SQ_VTX_SEMANTIC {}, expected {}",
                     out.regs.num_sq_vtx_semantic, out.attribVars.size())
      };
   }

   for (auto i = out.regs.num_sq_vtx_semantic; i < out.regs.sq_vtx_semantic.size(); ++i) {
      out.regs.sq_vtx_semantic[i] = out.regs.sq_vtx_semantic[i]
         .SEMANTIC_ID(0xFF);
   }

   // SQ_VTX_SEMANTIC_CLEAR.CLEAR should reflect the value of NUM_SQ_VTX_SEMANTIC
   auto semanticClear = static_cast<uint32_t>(~((1 << out.regs.num_sq_vtx_semantic) - 1));
   if (out.regs.sq_vtx_semantic_clear.CLEAR() == 0) {
      out.regs.sq_vtx_semantic_clear = out.regs.sq_vtx_semantic_clear
         .CLEAR(semanticClear);
   } else if (out.regs.sq_vtx_semantic_clear.CLEAR() != semanticClear) {
      throw gfd_header_parse_exception {
         fmt::format("Invalid SQ_VTX_SEMANTIC_CLEAR {:#x}, expected {:#x}",
                     out.regs.sq_vtx_semantic_clear.CLEAR(), semanticClear)
      };
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
   out.gx2rData.elemCount = 0;
   out.gx2rData.elemSize = 0;
   out.gx2rData.flags = static_cast<GX2RResourceFlags>(0);

   if (shader.uniformBlocksUsed) {
      out.mode = GX2ShaderMode::UniformBlock;
   } else {
      out.mode = GX2ShaderMode::UniformRegister;
   }

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
      .STACK_SIZE(out.loopVars.size() * 2);

   // Create binary
   out.data = getShaderBinary(shader);

   // Parse shader comments
   parseShaderComments(out, shader.comments);

   // NUM_GPRS should be the number of GPRs used in the shader
   if (out.regs.sq_pgm_resources_ps.NUM_GPRS() != numGpr) {
      throw gfd_header_parse_exception {
         fmt::format("Invalid SQ_PGM_RESOURCES_PS.NUM_GPRS {}, expected {}",
                     out.regs.sq_pgm_resources_ps.NUM_GPRS(), numGpr)
      };
   }

   if (out.regs.spi_ps_in_control_0.NUM_INTERP() == 0) {
      out.regs.spi_ps_in_control_0 = out.regs.spi_ps_in_control_0
         .NUM_INTERP(out.regs.num_spi_ps_input_cntl);
   } else if (out.regs.spi_ps_in_control_0.NUM_INTERP() != out.regs.num_spi_ps_input_cntl) {
      throw gfd_header_parse_exception {
         fmt::format("Expected SPI_PS_IN_CONTROL_0.NUM_INTERP {} to equal NUM_SPI_PS_INPUT_CNTL {}",
                     out.regs.spi_ps_in_control_0.NUM_INTERP(),
                     out.regs.num_spi_ps_input_cntl)
      };
   }

   file.pixelShaders.push_back(out);
   return true;
}
