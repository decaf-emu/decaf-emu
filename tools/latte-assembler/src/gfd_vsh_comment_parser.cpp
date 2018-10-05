#include "gfd_comment_parser.h"
#include <fmt/format.h>
#include <libgpu/latte/latte_constants.h>

static void
parseRegisterValue(latte::SQ_PGM_RESOURCES_VS &reg,
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
   } else {
      throw gfd_header_parse_exception {
         fmt::format("SQ_PGM_RESOURCES_VS does not have member {}", member)
      };
   }
}

static void
parseRegisterValue(latte::VGT_PRIMITIVEID_EN &reg,
                   const std::string &member,
                   const std::string &value)
{
   if (member == "PRIMITIVEID_EN") {
      reg = reg
         .PRIMITIVEID_EN(parseValueBool(value));
   } else {
      throw gfd_header_parse_exception {
         fmt::format("VGT_PRIMITIVEID_EN does not have member {}", member)
      };
   }
}

static void
parseRegisterValue(latte::SPI_VS_OUT_CONFIG &reg,
                   const std::string &member,
                   const std::string &value)
{
   if (member == "VS_PER_COMPONENT") {
      reg = reg
         .VS_PER_COMPONENT(parseValueBool(value));
   } else if (member == "VS_EXPORT_COUNT") {
      reg = reg
         .VS_EXPORT_COUNT(parseValueNumber(value));
   } else if (member == "VS_EXPORTS_FOG") {
      reg = reg
         .VS_EXPORTS_FOG(parseValueBool(value));
   } else if (member == "VS_OUT_FOG_VEC_ADDR") {
      reg = reg
         .VS_OUT_FOG_VEC_ADDR(parseValueNumber(value));
   } else {
      throw gfd_header_parse_exception {
         fmt::format("SPI_VS_OUT_CONFIG does not have member {}", member)
      };
   }
}

static void
parseRegisterValue(std::array<latte::SPI_VS_OUT_ID_N, 10> &spi_vs_out_id,
                   uint32_t index,
                   const std::string &member,
                   const std::string &value)
{
   if (index >= spi_vs_out_id.size()) {
      throw gfd_header_parse_exception {
         fmt::format("SQ_VTX_SEMANTIC[{}] invalid index, max: {}",
                     index, spi_vs_out_id.size())
      };
   }

   if (member == "SEMANTIC_0") {
      spi_vs_out_id[index] = spi_vs_out_id[index]
         .SEMANTIC_0(parseValueNumber(value));
   } else if (member == "SEMANTIC_1") {
      spi_vs_out_id[index] = spi_vs_out_id[index]
         .SEMANTIC_1(parseValueNumber(value));
   } else if (member == "SEMANTIC_2") {
      spi_vs_out_id[index] = spi_vs_out_id[index]
         .SEMANTIC_2(parseValueNumber(value));
   } else if (member == "SEMANTIC_3") {
      spi_vs_out_id[index] = spi_vs_out_id[index]
         .SEMANTIC_3(parseValueNumber(value));
   } else {
      throw gfd_header_parse_exception {
         fmt::format("SPI_VS_OUT_ID[{}] does not have member {}",
                     index, member)
      };
   }
}

static void
parseRegisterValue(latte::PA_CL_VS_OUT_CNTL &reg,
                   const std::string &member,
                   const std::string &value)
{
   if (member == "CLIP_DIST_ENA_0") {
      reg = reg
         .CLIP_DIST_ENA_0(parseValueBool(value));
   } else if (member == "CLIP_DIST_ENA_1") {
      reg = reg
         .CLIP_DIST_ENA_1(parseValueBool(value));
   } else if (member == "CLIP_DIST_ENA_2") {
      reg = reg
         .CLIP_DIST_ENA_2(parseValueBool(value));
   } else if (member == "CLIP_DIST_ENA_3") {
      reg = reg
         .CLIP_DIST_ENA_3(parseValueBool(value));
   } else if (member == "CLIP_DIST_ENA_4") {
      reg = reg
         .CLIP_DIST_ENA_4(parseValueBool(value));
   } else if (member == "CLIP_DIST_ENA_5") {
      reg = reg
         .CLIP_DIST_ENA_5(parseValueBool(value));
   } else if (member == "CLIP_DIST_ENA_6") {
      reg = reg
         .CLIP_DIST_ENA_6(parseValueBool(value));
   } else if (member == "CLIP_DIST_ENA_7") {
      reg = reg
         .CLIP_DIST_ENA_7(parseValueBool(value));
   } else if (member == "CULL_DIST_ENA_0") {
      reg = reg
         .CULL_DIST_ENA_0(parseValueBool(value));
   } else if (member == "CULL_DIST_ENA_1") {
      reg = reg
         .CULL_DIST_ENA_1(parseValueBool(value));
   } else if (member == "CULL_DIST_ENA_2") {
      reg = reg
         .CULL_DIST_ENA_2(parseValueBool(value));
   } else if (member == "CULL_DIST_ENA_3") {
      reg = reg
         .CULL_DIST_ENA_3(parseValueBool(value));
   } else if (member == "CULL_DIST_ENA_4") {
      reg = reg
         .CULL_DIST_ENA_4(parseValueBool(value));
   } else if (member == "CULL_DIST_ENA_5") {
      reg = reg
         .CULL_DIST_ENA_5(parseValueBool(value));
   } else if (member == "CULL_DIST_ENA_6") {
      reg = reg
         .CULL_DIST_ENA_6(parseValueBool(value));
   } else if (member == "CULL_DIST_ENA_7") {
      reg = reg
         .CULL_DIST_ENA_7(parseValueBool(value));
   } else if (member == "USE_VTX_POINT_SIZE") {
      reg = reg
         .USE_VTX_POINT_SIZE(parseValueBool(value));
   } else if (member == "USE_VTX_EDGE_FLAG") {
      reg = reg
         .USE_VTX_EDGE_FLAG(parseValueBool(value));
   } else if (member == "USE_VTX_RENDER_TARGET_INDX") {
      reg = reg
         .USE_VTX_RENDER_TARGET_INDX(parseValueBool(value));
   } else if (member == "USE_VTX_VIEWPORT_INDX") {
      reg = reg
         .USE_VTX_VIEWPORT_INDX(parseValueBool(value));
   } else if (member == "USE_VTX_KILL_FLAG") {
      reg = reg
         .USE_VTX_KILL_FLAG(parseValueBool(value));
   } else if (member == "VS_OUT_MISC_VEC_ENA") {
      reg = reg
         .VS_OUT_MISC_VEC_ENA(parseValueBool(value));
   } else if (member == "VS_OUT_CCDIST0_VEC_ENA") {
      reg = reg
         .VS_OUT_CCDIST0_VEC_ENA(parseValueBool(value));
   } else if (member == "VS_OUT_CCDIST1_VEC_ENA") {
      reg = reg
         .VS_OUT_CCDIST1_VEC_ENA(parseValueBool(value));
   } else if (member == "VS_OUT_MISC_SIDE_BUS_ENA") {
      reg = reg
         .VS_OUT_MISC_SIDE_BUS_ENA(parseValueBool(value));
   } else if (member == "USE_VTX_GS_CUT_FLAG") {
      reg = reg
         .USE_VTX_GS_CUT_FLAG(parseValueBool(value));
   } else {
      throw gfd_header_parse_exception {
         fmt::format("SPI_VS_OUT_CONFIG does not have member {}", member)
      };
   }
}

static void
parseRegisterValue(std::array<latte::SQ_VTX_SEMANTIC_N, 32> &sq_vtx_semantic,
                   uint32_t index,
                   const std::string &member,
                   const std::string &value)
{
   if (index >= sq_vtx_semantic.size()) {
      throw gfd_header_parse_exception {
         fmt::format("SQ_VTX_SEMANTIC[{}] invalid index, max: {}",
                     index, sq_vtx_semantic.size())
      };
   }

   if (member == "SEMANTIC_ID") {
      sq_vtx_semantic[index] = sq_vtx_semantic[index]
         .SEMANTIC_ID(parseValueNumber(value));
   } else {
      throw gfd_header_parse_exception {
         fmt::format("SQ_VTX_SEMANTIC[{}] does not have member {}",
                     index, member)
      };
   }
}

static void
parseRegisterValue(latte::VGT_STRMOUT_BUFFER_EN &reg,
                   const std::string &member,
                   const std::string &value)
{
   if (member == "BUFFER_0_EN") {
      reg = reg
         .BUFFER_0_EN(parseValueBool(value));
   } else if(member == "BUFFER_1_EN") {
      reg = reg
         .BUFFER_1_EN(parseValueBool(value));
   } else if (member == "BUFFER_2_EN") {
      reg = reg
         .BUFFER_2_EN(parseValueBool(value));
   } else if (member == "BUFFER_3_EN") {
      reg = reg
         .BUFFER_3_EN(parseValueBool(value));
   } else {
      throw gfd_header_parse_exception {
         fmt::format("VGT_STRMOUT_BUFFER_EN does not have member {}", member)
      };
   }
}

static void
parseRegisterValue(latte::VGT_VERTEX_REUSE_BLOCK_CNTL &reg,
                   const std::string &member,
                   const std::string &value)
{
   if (member == "VTX_REUSE_DEPTH") {
      reg = reg
         .VTX_REUSE_DEPTH(parseValueNumber(value));
   } else {
      throw gfd_header_parse_exception {
         fmt::format("VGT_VERTEX_REUSE_BLOCK_CNTL does not have member {}",
                     member)
      };
   }
}

static void
parseRegisterValue(latte::VGT_HOS_REUSE_DEPTH &reg,
                   const std::string &member,
                   const std::string &value)
{
   if (member == "REUSE_DEPTH") {
      reg = reg
         .REUSE_DEPTH(parseValueNumber(value));
   } else {
      throw gfd_header_parse_exception {
         fmt::format("VGT_HOS_REUSE_DEPTH does not have member {}", member)
      };
   }
}

static void
parseAttribVars(std::vector<gfd::GFDAttribVar> &attribVars,
                uint32_t index,
                const std::string &member,
                const std::string &value)
{
   if (index >= latte::MaxAttribBuffers) {
      throw gfd_header_parse_exception {
         fmt::format("ATTRIB_VARS[{}] invalid index, max: {}",
                     index, latte::MaxAttribBuffers)
      };
   }

   if (index >= attribVars.size()) {
      attribVars.resize(index + 1);
      attribVars[index].type = cafe::gx2::GX2ShaderVarType::Float4;
      attribVars[index].count = 0;
      attribVars[index].location = index;
   }

   if (member == "NAME") {
      attribVars[index].name = value;
   } else if (member == "TYPE") {
      attribVars[index].type = parseShaderVarType(value);
   } else if (member == "COUNT") {
      attribVars[index].count = parseValueNumber(value);
   } else if (member == "LOCATION") {
      attribVars[index].location = parseValueNumber(value);
   } else {
      throw gfd_header_parse_exception {
         fmt::format("ATTRIB_VARS[{}] does not have member {}", index, member)
      };
   }
}

bool
parseShaderComments(gfd::GFDVertexShader &shader,
                    std::vector<std::string> &comments)
{
   for (auto &comment : comments) {
      CommentKeyValue kv;

      if (!parseComment(comment, kv)) {
         continue;
      }

      std::transform(kv.obj.begin(), kv.obj.end(), kv.obj.begin(), ::toupper);
      std::transform(kv.member.begin(), kv.member.end(), kv.member.begin(), ::toupper);

      if (kv.obj == "SQ_PGM_RESOURCES_VS") {
         ensureObject(kv);
         parseRegisterValue(shader.regs.sq_pgm_resources_vs, kv.member, kv.value);
      } else if (kv.obj == "VGT_PRIMITIVEID_EN") {
         ensureObject(kv);
         parseRegisterValue(shader.regs.vgt_primitiveid_en, kv.member, kv.value);
      } else if (kv.obj == "SPI_VS_OUT_CONFIG") {
         ensureObject(kv);
         parseRegisterValue(shader.regs.spi_vs_out_config, kv.member, kv.value);
      } else if (kv.obj == "NUM_SPI_VS_OUT_ID") {
         ensureValue(kv);
         shader.regs.num_spi_vs_out_id = parseValueNumber(kv.value);
      } else if (kv.obj == "SPI_VS_OUT_ID") {
         ensureArrayOfObjects(kv);
         parseRegisterValue(shader.regs.spi_vs_out_id, std::stoul(kv.index), kv.member, kv.value);
      } else if (kv.obj == "PA_CL_VS_OUT_CNTL") {
         ensureObject(kv);
         parseRegisterValue(shader.regs.pa_cl_vs_out_cntl, kv.member, kv.value);
      } else if (kv.obj == "SQ_VTX_SEMANTIC_CLEAR") {
         ensureValue(kv);
         shader.regs.sq_vtx_semantic_clear = shader.regs.sq_vtx_semantic_clear
            .CLEAR(parseValueNumber(kv.value));
      } else if (kv.obj == "NUM_SQ_VTX_SEMANTIC") {
         ensureValue(kv);
         shader.regs.num_sq_vtx_semantic = parseValueNumber(kv.value);
      } else if (kv.obj == "SQ_VTX_SEMANTIC") {
         ensureArrayOfObjects(kv);
         parseRegisterValue(shader.regs.sq_vtx_semantic, std::stoul(kv.index), kv.member, kv.value);
      } else if (kv.obj == "VGT_STRMOUT_BUFFER_EN") {
         ensureObject(kv);
         parseRegisterValue(shader.regs.vgt_strmout_buffer_en, kv.member, kv.value);
      } else if (kv.obj == "VGT_VERTEX_REUSE_BLOCK_CNTL") {
         ensureObject(kv);
         parseRegisterValue(shader.regs.vgt_vertex_reuse_block_cntl, kv.member, kv.value);
      } else if (kv.obj == "VGT_HOS_REUSE_DEPTH") {
         ensureObject(kv);
         parseRegisterValue(shader.regs.vgt_hos_reuse_depth, kv.member, kv.value);
      } else if (kv.obj == "ATTRIB_VARS") {
         ensureArrayOfObjects(kv);
         parseAttribVars(shader.attribVars, std::stoul(kv.index), kv.member, kv.value);
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
      } else if (kv.obj == "RING_ITEM_SIZE") {
         ensureValue(kv);
         shader.ringItemSize = parseValueNumber(kv.value);
      } else if (kv.obj == "HAS_STREAM_OUT") {
         ensureValue(kv);
         shader.hasStreamOut = parseValueBool(kv.value);
      } else if (kv.obj == "STREAM_OUT_STRIDE") {
         ensureArrayOfValues(kv);
         auto index = std::stoul(kv.index);

         if (index >= shader.streamOutStride.size()) {
            throw gfd_header_parse_exception {
               fmt::format("STREAM_OUT_STRIDE[{}] invalid index, max: {}",
                           index, shader.streamOutStride.size())
            };
         }

         shader.streamOutStride[index] = parseValueNumber(kv.value);
      } else {
         throw gfd_header_parse_exception {
            fmt::format("Unknown key {}", kv.obj)
         };
      }

      /*
      TODO:
      GFDRBuffer gx2rData;
      */
   }

   return true;
}
