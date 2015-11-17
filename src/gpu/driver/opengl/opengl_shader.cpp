#include "opengl_driver.h"
#include "gpu/latte_registers.h"
#include "gpu/latte_instructions.h"
#include "gpu/latte_opcodes.h"
#include "gpu/latte.h"
#include "gpu/glsl/glsl_generator.h"
#include "utils/log.h"
#include <spdlog/spdlog.h>

namespace gpu
{

namespace opengl
{

bool Driver::parseFetchShader(FetchShader &shader, void *buffer, size_t size)
{
   auto program = reinterpret_cast<latte::ControlFlowInst *>(buffer);

   for (auto i = 0; i < size / latte::WordsPerCF; i++) {
      auto &cf = program[i];

      switch (cf.word1.CF_INST) {
      case latte::SQ_CF_INST_VTX:
      case latte::SQ_CF_INST_VTX_TC:
      {
         auto vfPtr = reinterpret_cast<latte::VertexFetchInst *>(program + cf.word0.addr);
         auto count = (cf.word1.COUNT_3 << 3) | cf.word1.COUNT;

         for (auto j = 0u; j < count; ++j) {
            auto &vf = vfPtr[j];

            if (vf.word0.VTX_INST != latte::SQ_VTX_INST_SEMANTIC) {
               gLog->error("Unexpected fetch shader VTX_INST {}", vf.word0.VTX_INST);
               continue;
            }

            // Parse new attrib
            shader.attribs.emplace_back();
            auto &attrib = shader.attribs.back();
            attrib.bytesPerElement = vf.word0.MEGA_FETCH_COUNT + 1;
            attrib.format = vf.word1.DATA_FORMAT;
            attrib.buffer = vf.word0.BUFFER_ID;
            attrib.location = vf.gpr.DST_GPR;
            attrib.offset = vf.word2.OFFSET;
            attrib.formatComp = vf.word1.FORMAT_COMP_ALL;
            attrib.numFormat = vf.word1.NUM_FORMAT_ALL;
            attrib.dstSel[0] = vf.word1.DST_SEL_X;
            attrib.dstSel[1] = vf.word1.DST_SEL_Y;
            attrib.dstSel[2] = vf.word1.DST_SEL_Z;
            attrib.dstSel[3] = vf.word1.DST_SEL_W;
         }
         break;
      }
      case latte::SQ_CF_INST_RETURN:
      case latte::SQ_CF_INST_END_PROGRAM:
         return true;
      default:
         gLog->error("Unexpected fetch shader instruction {}", cf.word1.CF_INST);
      }

      if (cf.word1.END_OF_PROGRAM) {
         return true;
      }
   }

   return false;
}

static size_t
getDataFormatChannels(latte::SQ_DATA_FORMAT format)
{
   switch (format) {
   case latte::FMT_8:
      return 1;
   case latte::FMT_4_4:
      return 2;
   case latte::FMT_3_3_2:
      return 3;
   case latte::FMT_16:
      return 1;
   case latte::FMT_16_FLOAT:
      return 1;
   case latte::FMT_8_8:
      return 2;
   case latte::FMT_5_6_5:
      return 3;
   case latte::FMT_6_5_5:
      return 3;
   case latte::FMT_1_5_5_5:
      return 4;
   case latte::FMT_4_4_4_4:
      return 4;
   case latte::FMT_5_5_5_1:
      return 4;
   case latte::FMT_32:
      return 1;
   case latte::FMT_32_FLOAT:
      return 1;
   case latte::FMT_16_16:
      return 2;
   case latte::FMT_16_16_FLOAT:
      return 2;
   case latte::FMT_8_24:
      return 2;
   case latte::FMT_8_24_FLOAT:
      return 2;
   case latte::FMT_24_8:
      return 2;
   case latte::FMT_24_8_FLOAT:
      return 2;
   case latte::FMT_10_11_11:
      return 3;
   case latte::FMT_10_11_11_FLOAT:
      return 3;
   case latte::FMT_11_11_10:
      return 3;
   case latte::FMT_11_11_10_FLOAT:
      return 3;
   case latte::FMT_2_10_10_10:
      return 4;
   case latte::FMT_8_8_8_8:
      return 4;
   case latte::FMT_10_10_10_2:
      return 4;
   case latte::FMT_X24_8_32_FLOAT:
      return 3;
   case latte::FMT_32_32:
      return 2;
   case latte::FMT_32_32_FLOAT:
      return 2;
   case latte::FMT_16_16_16_16:
      return 4;
   case latte::FMT_16_16_16_16_FLOAT:
      return 4;
   case latte::FMT_32_32_32_32:
      return 4;
   case latte::FMT_32_32_32_32_FLOAT:
      return 4;
   case latte::FMT_1:
      return 1;
   case latte::FMT_GB_GR:
      return 2;
   case latte::FMT_BG_RG:
      return 2;
   case latte::FMT_32_AS_8:
      return 1;
   case latte::FMT_32_AS_8_8:
      return 1;
   case latte::FMT_5_9_9_9_SHAREDEXP:
      return 4;
   case latte::FMT_8_8_8:
      return 3;
   case latte::FMT_16_16_16:
      return 3;
   case latte::FMT_16_16_16_FLOAT:
      return 3;
   case latte::FMT_32_32_32:
      return 3;
   case latte::FMT_32_32_32_FLOAT:
      return 3;
   default:
      return 4;
   }
}

static const char *
getGLSLDataFormat(latte::SQ_DATA_FORMAT format, latte::SQ_NUM_FORMAT num, latte::SQ_FORMAT_COMP comp)
{
   auto channels = getDataFormatChannels(format);

   if (num == latte::SQ_NUM_FORMAT_INT) {
      if (comp == latte::SQ_FORMAT_COMP_SIGNED) {
         switch (channels) {
         case 1:
            return "int";
         case 2:
            return "ivec2";
         case 3:
            return "ivec3";
         case 4:
            return "ivec4";
         }
      } else {
         switch (channels) {
         case 1:
            return "uint";
         case 2:
            return "uvec2";
         case 3:
            return "uvec3";
         case 4:
            return "uvec4";
         }
      }
   } else {
      switch (channels) {
      case 1:
         return "float";
      case 2:
         return "vec2";
      case 3:
         return "vec3";
      case 4:
         return "vec4";
      }
   }

   gLog->error("Unsupported GLSL data format, format={}, num={}, comp={}", format, num, comp);
   return "vec4";
}

bool Driver::compileVertexShader(VertexShader &vertex, FetchShader &fetch, uint8_t *buffer, size_t size)
{
   auto sq_config = getRegister<latte::SQ_CONFIG>(latte::Register::SQ_CONFIG);
   auto spi_vs_out_config = getRegister<latte::SPI_VS_OUT_CONFIG>(latte::Register::SPI_VS_OUT_CONFIG);
   fmt::MemoryWriter out;
   latte::Shader shader;
   std::string body;
   FetchShader::Attrib *semanticAttribs[32];
   memset(semanticAttribs, 0, sizeof(FetchShader::Attrib *) * 32);

   if (!latte::decode(shader, latte::Shader::Vertex, { buffer, size })) {
      gLog->error("Failed to decode vertex shader");
      return false;
   }

   if (!glsl::generateBody(shader, body)) {
      gLog->error("Failed to generate GLSL for vertex shader");
      return false;
   }

   // Uniforms
   if (sq_config.DX9_CONSTS) {
      // Uniform registers
      out << "uniform vec4 VC[256];\n";
   } else {
      // Uniform blocks
      // TODO: Change max size of VUB
      out << "uniform VertexUB {\n"
          << "   vec4 values[];\n"
          << "} VUB[4]\n";
   }

   out << '\n';

   // Samplers
   for (auto id : shader.samplersUsed) {
      out << "uniform sampler2D sampler_" << id << ";\n";
   }

   out << '\n';

   // Vertex Shader Inputs
   for (auto &attrib : fetch.attribs) {
      semanticAttribs[attrib.location] = &attrib;

      out << "in "
          << getGLSLDataFormat(attrib.format, attrib.numFormat, attrib.formatComp)
          << " fs_out_" << attrib.location << ";\n";
   }

   out << '\n';

   // Vertex Shader Exports
   for (auto i = 0u; i <= spi_vs_out_config.VS_EXPORT_COUNT; i += 4) {
      auto spi_vs_out_id = getRegister<latte::SPI_VS_OUT_ID_N>(latte::Register::SPI_VS_OUT_ID_0 + i * 4);

      if ((i + 0) <= spi_vs_out_config.VS_EXPORT_COUNT) {
         out << "out vec4 vs_out_" << spi_vs_out_id.SEMANTIC_0 << ";\n";
      }

      if ((i + 1) <= spi_vs_out_config.VS_EXPORT_COUNT) {
         out << "out vec4 vs_out_" << spi_vs_out_id.SEMANTIC_1 << ";\n";
      }

      if ((i + 2) <= spi_vs_out_config.VS_EXPORT_COUNT) {
         out << "out vec4 vs_out_" << spi_vs_out_id.SEMANTIC_2 << ";\n";
      }

      if ((i + 3) <= spi_vs_out_config.VS_EXPORT_COUNT) {
         out << "out vec4 vs_out_" << spi_vs_out_id.SEMANTIC_3 << ";\n";
      }
   }

   out << '\n';

   // Program code
   out << "void main()\n"
       << "{\n";

   // Registers
   for (auto id : shader.gprsUsed) {
      out << "vec4 R" << id << ";\n";
   }

   // Previous Scalar
   if (shader.psUsed.size()) {
      out << "float PS;\n"
          << "float PSo;\n";
   }

   // Previous Vector
   if (shader.pvUsed.size()) {
      out << "float4 PV;\n"
          << "float4 PVo;\n";
   }

   // Address Register (TODO: Only print if used)
   out << "int4 AR;\n";

   // Loop Index (TODO: Only print if used)
   out << "int AL;\n";

   // Exports
   for (auto exp : shader.exports) {
      switch (exp->type) {
      case latte::exp::Type::Position:
         out << "vec4 exp_position_" << exp->dstReg << ";\n";
         break;
      case latte::exp::Type::Parameter:
         out << "vec4 exp_param_" << exp->dstReg << ";\n";
         break;
      case latte::exp::Type::Pixel:
         out << "vec4 exp_pixel_" << exp->dstReg << ";\n";
         break;
      }
   }

   // Initialise registers
   // TODO: Check which order of VertexID and InstanceID for r0.x, r0.y
   out << "R0 = vec4(intBitsToFloat(gl_VertexID), intBitsToFloat(gl_InstanceID), 0.0, 0.0);\n";

   // Assign fetch shader output to our GPR
   for (auto i = 0u; i < 32; ++i) {
      auto sq_vtx_semantic = getRegister<latte::SQ_VTX_SEMANTIC_N>(latte::Register::SQ_VTX_SEMANTIC_0 + i * 4);
      auto id = sq_vtx_semantic.SEMANTIC_ID;

      if (sq_vtx_semantic.SEMANTIC_ID == 0xff) {
         continue;
      }

      auto attrib = semanticAttribs[id];

      if (!attrib) {
         throw std::logic_error("Invalid semantic mapping");
      }

      out << "R" << (i + 1) << " = ";

      auto channels = getDataFormatChannels(attrib->format);

      switch (channels) {
      case 1:
         out << "vec4(fs_out_" << id << ", 0.0, 0.0, 0.0);\n";
         break;
      case 2:
         out << "vec4(fs_out_" << id << ", 0.0, 0.0);\n";
         break;
      case 3:
         out << "vec4(fs_out_" << id << ", 0.0);\n";
         break;
      case 4:
         out << "fs_out_" << id << ";\n";
         break;
      }
   }

   out << '\n' << body << '\n';

   for (auto exp : shader.exports) {
      switch (exp->type) {
      case latte::exp::Type::Position:
         out << "gl_Position = exp_position_" << exp->dstReg << ";\n";
         break;
      case latte::exp::Type::Parameter:
         out << "vs_out_" << exp->dstReg << " = exp_param_" << exp->dstReg << ";\n";
         break;
      case latte::exp::Type::Pixel:
         out << "ps_out_" << exp->dstReg << " = exp_pixel_" << exp->dstReg << ";\n";
         break;
      }
   }

   out << "}\n";
   gLog->debug("Shader: \n{}", out.c_str());
   return true;
}


bool Driver::compilePixelShader(PixelShader &pixel, uint8_t *buffer, size_t size)
{
   return false;
   /*
   latte::SPI_PS_IN_CONTROL_0 spi_ps_in_control_0;
   latte::SPI_PS_IN_CONTROL_1 spi_ps_in_control_1;
   latte::CB_SHADER_MASK cb_shader_mask;
   fmt::MemoryWriter out;

   for (auto i = 0u; i < spi_ps_in_control_0.NUM_INTERP; ++i) {
      latte::SPI_PS_INPUT_CNTL_0 input;

      if (input.FLAT_SHADE) {
         out << "flat ";
      }

      out << "in vec4 vs_out_" << input.SEMANTIC << ";\n";
   }

   auto maskBits = cb_shader_mask.value;
   for (auto i = 0; i < 8; ++i) {
      auto mask = maskBits & 0xf;

      if (mask) {
         out << "out vec4 ps_out_" << i << ";\n";
      }
      maskBits >>= 4;
   }


   out << "void main()\n"
      << "{\n";

   for (auto i = 0u; i < spi_ps_in_control_0.NUM_INTERP; ++i) {
      latte::SPI_PS_INPUT_CNTL_0 input;
      out << "R" << i << " = vs_out_" << input.SEMANTIC << ";\n";
   }

   out << "}\n";*/
}

} // namespace opengl

} // namespace gpu
