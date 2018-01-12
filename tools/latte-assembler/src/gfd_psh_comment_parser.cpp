#include "gfd_comment_parser.h"
#include <fmt/format.h>

static void
parseRegisterValue(latte::SQ_PGM_RESOURCES_PS &reg,
                   const std::string &member,
                   const std::string &value)
{
   if (member == "NUM_GPRS") {
      reg = reg
         .NUM_GPRS(parseValueNumber(value));
   } else if (member == "STACK_SIZE") {
      reg = reg
         .STACK_SIZE(parseValueNumber(value));
   } else if (member == "DX10_CLAMP") {
      reg = reg
         .DX10_CLAMP(parseValueBool(value));
   } else if (member == "PRIME_CACHE_PGM_EN") {
      reg = reg
         .PRIME_CACHE_PGM_EN(parseValueBool(value));
   } else if (member == "PRIME_CACHE_ON_DRAW") {
      reg = reg
         .PRIME_CACHE_ON_DRAW(parseValueBool(value));
   } else if (member == "FETCH_CACHE_LINES") {
      reg = reg
         .FETCH_CACHE_LINES(parseValueNumber(value));
   } else if (member == "UNCACHED_FIRST_INST") {
      reg = reg
         .UNCACHED_FIRST_INST(parseValueBool(value));
   } else if (member == "PRIME_CACHE_ENABLE") {
      reg = reg
         .PRIME_CACHE_ENABLE(parseValueBool(value));
   } else if (member == "PRIME_CACHE_ON_CONST") {
      reg = reg
         .PRIME_CACHE_ON_CONST(parseValueBool(value));
   } else if (member == "CLAMP_CONSTS") {
      reg = reg
         .CLAMP_CONSTS(parseValueBool(value));
   } else {
      throw gfd_header_parse_exception {
         fmt::format("SQ_PGM_RESOURCES_PS does not have member {}", member)
      };
   }
}

static void
parseRegisterValue(latte::SQ_PGM_EXPORTS_PS &reg,
                   const std::string &member,
                   const std::string &value)
{
   if (member == "EXPORT_MODE") {
      reg = reg
         .EXPORT_MODE(parseValueNumber(value));
   } else {
      throw gfd_header_parse_exception {
         fmt::format("SQ_PGM_EXPORTS_PS does not have member {}", member)
      };
   }
}

static latte::SPI_BARYC_CNTL
parseSpiBarycCntl(const std::string &v)
{
   auto value = v;
   std::transform(value.begin(), value.end(), value.begin(), ::toupper);

   if (value == "CENTROIDS_ONLY") {
      return latte::SPI_BARYC_CNTL::CENTROIDS_ONLY;
   } else if (value == "CENTERS_ONLY") {
      return latte::SPI_BARYC_CNTL::CENTERS_ONLY;
   } else if (value == "CENTROIDS_AND_CENTERS") {
      return latte::SPI_BARYC_CNTL::CENTROIDS_AND_CENTERS;
   } else {
      throw gfd_header_parse_exception {
         fmt::format("Invalid SPI_BARYC_CNTL {}", value)
      };
   }
}

static latte::DB_Z_ORDER
parseDbZOrder(const std::string &v)
{
   auto value = v;
   std::transform(value.begin(), value.end(), value.begin(), ::toupper);

   if (value == "LATE_Z") {
      return latte::DB_Z_ORDER::LATE_Z;
   } else if (value == "EARLY_Z_THEN_LATE_Z") {
      return latte::DB_Z_ORDER::EARLY_Z_THEN_LATE_Z;
   } else if (value == "RE_Z") {
      return latte::DB_Z_ORDER::RE_Z;
   } else if (value == "EARLY_Z_THEN_RE_Z") {
      return latte::DB_Z_ORDER::EARLY_Z_THEN_RE_Z;
   } else {
      throw gfd_header_parse_exception {
         fmt::format("Invalid DB_Z_ORDER {}", value)
      };
   }
}

static void
parseRegisterValue(latte::SPI_PS_IN_CONTROL_0 &reg,
                   const std::string &member,
                   const std::string &value)
{
   if (member == "NUM_INTERP") {
      reg = reg
         .NUM_INTERP(parseValueNumber(value));
   } else if (member == "POSITION_ENA") {
      reg = reg
         .POSITION_ENA(parseValueBool(value));
   } else if (member == "POSITION_CENTROID") {
      reg = reg
         .POSITION_CENTROID(parseValueBool(value));
   } else if (member == "POSITION_ADDR") {
      reg = reg
         .POSITION_ADDR(parseValueNumber(value));
   } else if (member == "PARAM_GEN") {
      reg = reg
         .PARAM_GEN(parseValueNumber(value));
   } else if (member == "PARAM_GEN_ADDR") {
      reg = reg
         .PARAM_GEN_ADDR(parseValueNumber(value));
   } else if (member == "BARYC_SAMPLE_CNTL") {
      reg = reg
         .BARYC_SAMPLE_CNTL(parseSpiBarycCntl(value));
   } else if (member == "PERSP_GRADIENT_ENA") {
      reg = reg
         .PERSP_GRADIENT_ENA(parseValueBool(value));
   } else if (member == "LINEAR_GRADIENT_ENA") {
      reg = reg
         .LINEAR_GRADIENT_ENA(parseValueBool(value));
   } else if (member == "POSITION_SAMPLE") {
      reg = reg
         .POSITION_SAMPLE(parseValueBool(value));
   } else if (member == "BARYC_AT_SAMPLE_ENA") {
      reg = reg
         .BARYC_AT_SAMPLE_ENA(parseValueBool(value));
   } else {
      throw gfd_header_parse_exception {
         fmt::format("SPI_PS_IN_CONTROL_0 does not have member {}", member)
      };
   }
}

static void
parseRegisterValue(latte::SPI_PS_IN_CONTROL_1 &reg,
                   const std::string &member,
                   const std::string &value)
{
   if (member == "GEN_INDEX_PIX") {
      reg = reg
         .GEN_INDEX_PIX(parseValueBool(value));
   } else if (member == "GEN_INDEX_PIX_ADDR") {
      reg = reg
         .GEN_INDEX_PIX_ADDR(parseValueNumber(value));
   } else if (member == "FRONT_FACE_ENA") {
      reg = reg
         .FRONT_FACE_ENA(parseValueBool(value));
   } else if (member == "FRONT_FACE_CHAN") {
      reg = reg
         .FRONT_FACE_CHAN(parseValueNumber(value));
   } else if (member == "FRONT_FACE_ALL_BITS") {
      reg = reg
         .FRONT_FACE_ALL_BITS(parseValueBool(value));
   } else if (member == "FRONT_FACE_ADDR") {
      reg = reg
         .FRONT_FACE_ADDR(parseValueNumber(value));
   } else if (member == "FOG_ADDR") {
      reg = reg
         .FOG_ADDR(parseValueNumber(value));
   } else if (member == "FIXED_PT_POSITION_ENA") {
      reg = reg
         .FIXED_PT_POSITION_ENA(parseValueBool(value));
   } else if (member == "FIXED_PT_POSITION_ADDR") {
      reg = reg
         .FIXED_PT_POSITION_ADDR(parseValueNumber(value));
   } else if (member == "POSITION_ULC") {
      reg = reg
         .POSITION_ULC(parseValueBool(value));
   } else {
      throw gfd_header_parse_exception {
         fmt::format("SPI_PS_IN_CONTROL_1 does not have member {}", member)
      };
   }
}

static void
parseRegisterValue(std::array<latte::SPI_PS_INPUT_CNTL_N, 32> &spi_ps_input_cntls,
                   uint32_t index,
                   const std::string &member,
                   const std::string &value)
{
   if (index >= spi_ps_input_cntls.size()) {
      throw gfd_header_parse_exception {
         fmt::format("SPI_PS_INPUT_CNTL[{}] invalid index, max: {}",
                     index, spi_ps_input_cntls.size())
      };
   }

   if (member == "SEMANTIC") {
      spi_ps_input_cntls[index] = spi_ps_input_cntls[index]
         .SEMANTIC(parseValueNumber(value));
   } else if (member == "DEFAULT_VAL") {
      spi_ps_input_cntls[index] = spi_ps_input_cntls[index]
         .DEFAULT_VAL(parseValueNumber(value));
   } else if (member == "FLAT_SHADE") {
      spi_ps_input_cntls[index] = spi_ps_input_cntls[index]
         .FLAT_SHADE(parseValueBool(value));
   } else if (member == "SEL_CENTROID") {
      spi_ps_input_cntls[index] = spi_ps_input_cntls[index]
         .SEL_CENTROID(parseValueBool(value));
   } else if (member == "SEL_LINEAR") {
      spi_ps_input_cntls[index] = spi_ps_input_cntls[index]
         .SEL_LINEAR(parseValueBool(value));
   } else if (member == "CYL_WRAP") {
      spi_ps_input_cntls[index] = spi_ps_input_cntls[index]
         .CYL_WRAP(parseValueNumber(value));
   } else if (member == "PT_SPRITE_TEX") {
      spi_ps_input_cntls[index] = spi_ps_input_cntls[index]
         .PT_SPRITE_TEX(parseValueBool(value));
   } else if (member == "SEL_SAMPLE") {
      spi_ps_input_cntls[index] = spi_ps_input_cntls[index]
         .SEL_SAMPLE(parseValueBool(value));
   } else {
      throw gfd_header_parse_exception {
         fmt::format("SPI_PS_INPUT_CNTL[{}] does not have member {}",
                     index, member)
      };
   }
}

static void
parseRegisterValue(latte::CB_SHADER_MASK &reg,
                   const std::string &member,
                   const std::string &value)
{
   if (member == "OUTPUT0_ENABLE") {
      reg = reg
         .OUTPUT0_ENABLE(parseValueBool(value));
   } else if (member == "OUTPUT1_ENABLE") {
      reg = reg
         .OUTPUT1_ENABLE(parseValueBool(value));
   } else if (member == "OUTPUT2_ENABLE") {
      reg = reg
         .OUTPUT2_ENABLE(parseValueBool(value));
   } else if (member == "OUTPUT3_ENABLE") {
      reg = reg
         .OUTPUT3_ENABLE(parseValueBool(value));
   } else if (member == "OUTPUT4_ENABLE") {
      reg = reg
         .OUTPUT4_ENABLE(parseValueBool(value));
   } else if (member == "OUTPUT5_ENABLE") {
      reg = reg
         .OUTPUT5_ENABLE(parseValueBool(value));
   } else if (member == "OUTPUT6_ENABLE") {
      reg = reg
         .OUTPUT6_ENABLE(parseValueBool(value));
   } else if (member == "OUTPUT7_ENABLE") {
      reg = reg
         .OUTPUT7_ENABLE(parseValueBool(value));
   } else {
      throw gfd_header_parse_exception {
         fmt::format("CB_SHADER_MASK does not have member {}", member)
      };
   }
}

static void
parseRegisterValue(latte::CB_SHADER_CONTROL &reg,
                   const std::string &member,
                   const std::string &value)
{
   if (member == "RT0_ENABLE") {
      reg = reg
         .RT0_ENABLE(parseValueBool(value));
   } else if (member == "RT1_ENABLE") {
      reg = reg
         .RT1_ENABLE(parseValueBool(value));
   } else if (member == "RT2_ENABLE") {
      reg = reg
         .RT2_ENABLE(parseValueBool(value));
   } else if (member == "RT3_ENABLE") {
      reg = reg
         .RT3_ENABLE(parseValueBool(value));
   } else if (member == "RT4_ENABLE") {
      reg = reg
         .RT4_ENABLE(parseValueBool(value));
   } else if (member == "RT5_ENABLE") {
      reg = reg
         .RT5_ENABLE(parseValueBool(value));
   } else if (member == "RT6_ENABLE") {
      reg = reg
         .RT6_ENABLE(parseValueBool(value));
   } else if (member == "RT7_ENABLE") {
      reg = reg
         .RT7_ENABLE(parseValueBool(value));
   } else {
      throw gfd_header_parse_exception {
         fmt::format("CB_SHADER_CONTROL does not have member {}", member)
      };
   }
}

static void
parseRegisterValue(latte::DB_SHADER_CONTROL &reg,
                   const std::string &member,
                   const std::string &value)
{
   if (member == "Z_EXPORT_ENABLE") {
      reg = reg
         .Z_EXPORT_ENABLE(parseValueBool(value));
   } else if (member == "STENCIL_REF_EXPORT_ENABLE") {
      reg = reg
         .STENCIL_REF_EXPORT_ENABLE(parseValueBool(value));
   } else if (member == "Z_ORDER") {
      reg = reg
         .Z_ORDER(parseDbZOrder(value));
   } else if (member == "KILL_ENABLE") {
      reg = reg
         .KILL_ENABLE(parseValueBool(value));
   } else if (member == "COVERAGE_TO_MASK_ENABLE") {
      reg = reg
         .COVERAGE_TO_MASK_ENABLE(parseValueBool(value));
   } else if (member == "MASK_EXPORT_ENABLE") {
      reg = reg
         .MASK_EXPORT_ENABLE(parseValueBool(value));
   } else if (member == "DUAL_EXPORT_ENABLE") {
      reg = reg
         .DUAL_EXPORT_ENABLE(parseValueBool(value));
   } else if (member == "EXEC_ON_HIER_FAIL") {
      reg = reg
         .EXEC_ON_HIER_FAIL(parseValueBool(value));
   } else if (member == "EXEC_ON_NOOP") {
      reg = reg
         .EXEC_ON_NOOP(parseValueBool(value));
   } else if (member == "ALPHA_TO_MASK_DISABLE") {
      reg = reg
         .ALPHA_TO_MASK_DISABLE(parseValueBool(value));
   } else {
      throw gfd_header_parse_exception {
         fmt::format("DB_SHADER_CONTROL does not have member {}", member)
      };
   }
}

static void
parseRegisterValue(latte::SPI_INPUT_Z &reg,
                   const std::string &member,
                   const std::string &value)
{
   if (member == "PROVIDE_Z_TO_SPI") {
      reg = reg
         .PROVIDE_Z_TO_SPI(parseValueBool(value));
   } else {
      throw gfd_header_parse_exception {
         fmt::format("SPI_INPUT_Z does not have member {}", member)
      };
   }
}

bool
parseShaderComments(gfd::GFDPixelShader &shader,
                    std::vector<std::string> &comments)
{
   for (auto &comment : comments) {
      CommentKeyValue kv;

      if (!parseComment(comment, kv)) {
         continue;
      }

      std::transform(kv.obj.begin(), kv.obj.end(), kv.obj.begin(), ::toupper);
      std::transform(kv.member.begin(), kv.member.end(), kv.member.begin(), ::toupper);

      if (kv.obj == "SQ_PGM_RESOURCES_PS") {
         ensureObject(kv);
         parseRegisterValue(shader.regs.sq_pgm_resources_ps, kv.member, kv.value);
      } else if (kv.obj == "SQ_PGM_EXPORTS_PS") {
         ensureObject(kv);
         parseRegisterValue(shader.regs.sq_pgm_exports_ps, kv.member, kv.value);
      } else if (kv.obj == "SPI_PS_IN_CONTROL_0") {
         ensureObject(kv);
         parseRegisterValue(shader.regs.spi_ps_in_control_0, kv.member, kv.value);
      } else if (kv.obj == "SPI_PS_IN_CONTROL_1") {
         ensureValue(kv);
         parseRegisterValue(shader.regs.spi_ps_in_control_1, kv.member, kv.value);
      } else if (kv.obj == "NUM_SPI_PS_INPUT_CNTL") {
         ensureValue(kv);
         shader.regs.num_spi_ps_input_cntl = parseValueNumber(kv.value);
      } else if (kv.obj == "SPI_PS_INPUT_CNTL") {
         ensureArrayOfObjects(kv);
         parseRegisterValue(shader.regs.spi_ps_input_cntls, std::stoul(kv.index), kv.member, kv.value);
      } else if (kv.obj == "CB_SHADER_MASK") {
         ensureObject(kv);
         parseRegisterValue(shader.regs.cb_shader_mask, kv.member, kv.value);
      } else if (kv.obj == "CB_SHADER_CONTROL") {
         ensureObject(kv);
         parseRegisterValue(shader.regs.cb_shader_control, kv.member, kv.value);
      } else if (kv.obj == "DB_SHADER_CONTROL") {
         ensureObject(kv);
         parseRegisterValue(shader.regs.db_shader_control, kv.member, kv.value);
      } else if (kv.obj == "SPI_INPUT_Z") {
         ensureArrayOfObjects(kv);
         parseRegisterValue(shader.regs.spi_input_z, kv.member, kv.value);
      } else if (kv.obj == "UNIFORM_BLOCKS") {
         ensureArrayOfObjects(kv);
         parseUniformBlocks(shader.uniformBlocks, std::stoul(kv.index), kv.member, kv.value);
      } else if (kv.obj == "UNIFORM_VARS") {
         ensureArrayOfObjects(kv);
         parseUniformVars(shader.uniformVars, std::stoul(kv.index), kv.member, kv.value);
      } else if (kv.obj == "INITIAL_VALUES") {
         ensureArrayOfObjects(kv);
         parseInitialValues(shader.initialValues, std::stoul(kv.index), kv.member, kv.value);
      } else if (kv.obj == "LOOP_VARS") {
         ensureArrayOfObjects(kv);
         parseLoopVars(shader.loopVars, std::stoul(kv.index), kv.member, kv.value);
      } else if (kv.obj == "SAMPLER_VARS") {
         ensureArrayOfObjects(kv);
         parseSamplerVars(shader.samplerVars, std::stoul(kv.index), kv.member, kv.value);
      } else if (kv.obj == "MODE") {
         ensureValue(kv);
         shader.mode = parseShaderMode(kv.value);
      } else {
         throw gfd_header_parse_exception { fmt::format("Unknown key {}", kv.obj) };
      }

      /*
      TODO:
      GFDRBuffer gx2rData;
      */
   }

   return true;
}
