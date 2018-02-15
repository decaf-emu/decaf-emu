#include "gfd.h"
#include <common/align.h>
#include <fstream>

namespace gfd
{

struct DataPatch
{
   size_t offset;
   size_t target;
};

struct TextPatch
{
   size_t offset;
   size_t target;
   const char *text;
};

using MemoryFile = std::vector<uint8_t>;

template<typename Type>
inline void
writeAt(MemoryFile &fh,
        size_t pos,
        Type value)
{
   *reinterpret_cast<Type *>(fh.data() + pos) = byte_swap(value);
}

template<typename Type>
inline void
write(MemoryFile &fh,
      Type value)
{
   auto pos = fh.size();
   fh.resize(pos + sizeof(Type));
   *reinterpret_cast<Type *>(fh.data() + pos) = byte_swap(value);
}

inline void
writeNullTerminatedString(MemoryFile &fh,
                          const char *str)
{
   auto pos = fh.size();
   auto len = strlen(str) + 1;
   fh.resize(align_up(pos + len, 4));
   std::memcpy(fh.data() + pos, str, len);
}

inline void
writeBinary(MemoryFile &fh,
            const std::vector<uint8_t> &data)
{
   auto pos = fh.size();
   auto len = data.size();
   fh.resize(pos + len);
   std::memcpy(fh.data() + pos, data.data(), len);
}

static bool
writeGX2RBuffer(MemoryFile &fh,
                const GFDRBuffer &buffer)
{
   write<uint32_t>(fh, static_cast<uint32_t>(buffer.flags));
   write<uint32_t>(fh, buffer.elemSize);
   write<uint32_t>(fh, buffer.elemCount);
   write<uint32_t>(fh, 0); // buffer.buffer
   return true;
}

static bool
writeUniformBlocksData(std::vector<uint8_t> &fh,
                       DataPatch &patch,
                       std::vector<DataPatch> &dataPatches,
                       std::vector<TextPatch> &textPatches,
                       const std::vector<GFDUniformBlock> &uniformBlocks)
{
   if (uniformBlocks.size() > 0) {
      patch.target = fh.size();
      dataPatches.push_back(patch);

      for (auto &block : uniformBlocks) {
         textPatches.push_back(TextPatch { fh.size(), 0, block.name.c_str() });
         write<uint32_t>(fh, 0); // block.name
         write<uint32_t>(fh, block.offset);
         write<uint32_t>(fh, block.size);
      }
   }

   return true;
}

static bool
writeUniformVarsData(std::vector<uint8_t> &fh,
                     DataPatch &patch,
                     std::vector<DataPatch> &dataPatches,
                     std::vector<TextPatch> &textPatches,
                     const std::vector<GFDUniformVar> &uniformVars)
{
   if (uniformVars.size() > 0) {
      patch.target = fh.size();
      dataPatches.push_back(patch);

      for (auto &var : uniformVars) {
         textPatches.push_back(TextPatch { fh.size(), 0, var.name.c_str() });
         write<uint32_t>(fh, 0); // var.name
         write<uint32_t>(fh, static_cast<uint32_t>(var.type));
         write<uint32_t>(fh, var.count);
         write<uint32_t>(fh, var.offset);
         write<int32_t>(fh, var.block);
      }
   }

   return true;
}

static bool
writeInitialValuesData(std::vector<uint8_t> &fh,
                       DataPatch &patch,
                       std::vector<DataPatch> &dataPatches,
                       std::vector<TextPatch> &textPatches,
                       const std::vector<GFDUniformInitialValue> &initialValues)
{
   if (initialValues.size() > 0) {
      patch.target = fh.size();
      dataPatches.push_back(patch);

      for (auto &var : initialValues) {
         for (auto &value : var.value) {
            write<float>(fh, value);
         }

         write<uint32_t>(fh, var.offset);
      }
   }

   return true;
}

static bool
writeLoopVarsData(std::vector<uint8_t> &fh,
                  DataPatch &patch,
                  std::vector<DataPatch> &dataPatches,
                  std::vector<TextPatch> &textPatches,
                  const std::vector<GFDLoopVar> &loopVars)
{
   if (loopVars.size() > 0) {
      patch.target = fh.size();
      dataPatches.push_back(patch);

      for (auto &var : loopVars) {
         write<uint32_t>(fh, var.offset);
         write<uint32_t>(fh, var.value);
      }
   }

   return true;
}

static bool
writeSamplerVarsData(std::vector<uint8_t> &fh,
                     DataPatch &patch,
                     std::vector<DataPatch> &dataPatches,
                     std::vector<TextPatch> &textPatches,
                     const std::vector<GFDSamplerVar> &samplerVars)
{
   if (samplerVars.size() > 0) {
      patch.target = fh.size();
      dataPatches.push_back(patch);

      for (auto &var : samplerVars) {
         textPatches.push_back(TextPatch { fh.size(), 0, var.name.c_str() });
         write<uint32_t>(fh, 0); // var.name
         write<uint32_t>(fh, static_cast<uint32_t>(var.type));
         write<uint32_t>(fh, var.location);
      }
   }

   return true;
}

static bool
writeAttribVarsData(std::vector<uint8_t> &fh,
                    DataPatch &patch,
                    std::vector<DataPatch> &dataPatches,
                    std::vector<TextPatch> &textPatches,
                    const std::vector<GFDAttribVar> &attribVars)
{
   if (attribVars.size() > 0) {
      patch.target = fh.size();
      dataPatches.push_back(patch);

      for (auto &var : attribVars) {
         textPatches.push_back(TextPatch { fh.size(), 0, var.name.c_str() });
         write<uint32_t>(fh, 0); // var.name
         write<uint32_t>(fh, static_cast<uint32_t>(var.type));
         write<uint32_t>(fh, var.count);
         write<uint32_t>(fh, var.location);
      }
   }

   return true;
}

static bool
writeRelocationData(std::vector<uint8_t> &fh,
                    std::vector<DataPatch> &dataPatches,
                    std::vector<TextPatch> &textPatches)
{
   // Now write text
   auto textOffset = fh.size();

   for (auto &patch : textPatches) {
      patch.target = fh.size();
      writeNullTerminatedString(fh, patch.text);
   }

   auto textSize = fh.size() - textOffset;

   auto patchOffset = fh.size();
   auto dataSize = patchOffset;
   auto dataOffset = 0;

   // Now write patches
   for (auto &patch : dataPatches) {
      writeAt<uint32_t>(fh, patch.offset, static_cast<uint32_t>(patch.target) | GFDPatchData);
      write<uint32_t>(fh, static_cast<uint32_t>(patch.offset) | GFDPatchData);
   }

   for (auto &patch : textPatches) {
      writeAt<uint32_t>(fh, patch.offset, static_cast<uint32_t>(patch.target) | GFDPatchText);
      write<uint32_t>(fh, static_cast<uint32_t>(patch.offset) | GFDPatchText);
   }

   // Write relocation header
   write<uint32_t>(fh, GFDRelocationHeader::Magic);
   write<uint32_t>(fh, GFDRelocationHeader::HeaderSize);
   write<uint32_t>(fh, 0); // unk1
   write<uint32_t>(fh, static_cast<uint32_t>(dataSize));
   write<uint32_t>(fh, static_cast<uint32_t>(dataOffset) | GFDPatchData);
   write<uint32_t>(fh, static_cast<uint32_t>(textSize));
   write<uint32_t>(fh, static_cast<uint32_t>(textOffset) | GFDPatchData);
   write<uint32_t>(fh, 0); // patchBase
   write<uint32_t>(fh, static_cast<uint32_t>(dataPatches.size() + textPatches.size()));
   write<uint32_t>(fh, static_cast<uint32_t>(patchOffset) | GFDPatchData);
   return true;
}

static bool
writeVertexShader(std::vector<uint8_t> &fh,
                  const GFDVertexShader &vsh)
{
   std::vector<uint8_t> text;
   std::vector<DataPatch> dataPatches;
   std::vector<TextPatch> textPatches;
   decaf_check(fh.size() == 0);

   write<uint32_t>(fh, vsh.regs.sq_pgm_resources_vs.value);
   write<uint32_t>(fh, vsh.regs.vgt_primitiveid_en.value);
   write<uint32_t>(fh, vsh.regs.spi_vs_out_config.value);
   write<uint32_t>(fh, vsh.regs.num_spi_vs_out_id);

   for (auto &spi_vs_out_id : vsh.regs.spi_vs_out_id) {
      write<uint32_t>(fh, spi_vs_out_id.value);
   }

   write<uint32_t>(fh, vsh.regs.pa_cl_vs_out_cntl.value);
   write<uint32_t>(fh, vsh.regs.sq_vtx_semantic_clear.value);
   write<uint32_t>(fh, vsh.regs.num_sq_vtx_semantic);

   for (auto &sq_vtx_semantic : vsh.regs.sq_vtx_semantic) {
      write<uint32_t>(fh, sq_vtx_semantic.value);
   }

   write<uint32_t>(fh, vsh.regs.vgt_strmout_buffer_en.value);
   write<uint32_t>(fh, vsh.regs.vgt_vertex_reuse_block_cntl.value);
   write<uint32_t>(fh, vsh.regs.vgt_hos_reuse_depth.value);

   write<uint32_t>(fh, static_cast<uint32_t>(vsh.data.size()));
   write<uint32_t>(fh, 0); // vsh.data
   write<uint32_t>(fh, static_cast<uint32_t>(vsh.mode));

   auto uniformBlocksPatch = DataPatch { fh.size() + 4, 0 };
   write<uint32_t>(fh, static_cast<uint32_t>(vsh.uniformBlocks.size()));
   write<uint32_t>(fh, 0); // vsh.uniformBlocks.data

   auto uniformVarsPatch = DataPatch { fh.size() + 4, 0 };
   write<uint32_t>(fh, static_cast<uint32_t>(vsh.uniformVars.size()));
   write<uint32_t>(fh, 0); // vsh.uniformVars.data

   auto initialValuesPatch = DataPatch { fh.size() + 4, 0 };
   write<uint32_t>(fh, static_cast<uint32_t>(vsh.initialValues.size()));
   write<uint32_t>(fh, 0); // vsh.initialValues.data

   auto loopVarsPatch = DataPatch { fh.size() + 4, 0 };
   write<uint32_t>(fh, static_cast<uint32_t>(vsh.loopVars.size()));
   write<uint32_t>(fh, 0); // vsh.loopVars.data

   auto samplerVarsPatch = DataPatch { fh.size() + 4, 0 };
   write<uint32_t>(fh, static_cast<uint32_t>(vsh.samplerVars.size()));
   write<uint32_t>(fh, 0); // vsh.samplerVars.data

   auto attribVarsPatch = DataPatch { fh.size() + 4, 0 };
   write<uint32_t>(fh, static_cast<uint32_t>(vsh.attribVars.size()));
   write<uint32_t>(fh, 0); // vsh.attribVars.data

   write<uint32_t>(fh, vsh.ringItemSize);
   write<uint32_t>(fh, vsh.hasStreamOut ? 1 : 0);

   for (auto &stride : vsh.streamOutStride) {
      write<uint32_t>(fh, stride);
   }

   writeGX2RBuffer(fh, vsh.gx2rData);
   decaf_check(fh.size() == 0x134);

   // Now write relocated data
   writeUniformBlocksData(fh, uniformBlocksPatch, dataPatches, textPatches, vsh.uniformBlocks);
   writeUniformVarsData(fh, uniformVarsPatch, dataPatches, textPatches, vsh.uniformVars);
   writeInitialValuesData(fh, initialValuesPatch, dataPatches, textPatches, vsh.initialValues);
   writeLoopVarsData(fh, loopVarsPatch, dataPatches, textPatches, vsh.loopVars);
   writeSamplerVarsData(fh, samplerVarsPatch, dataPatches, textPatches, vsh.samplerVars);
   writeAttribVarsData(fh, attribVarsPatch, dataPatches, textPatches, vsh.attribVars);
   writeRelocationData(fh, dataPatches, textPatches);
   return true;
}

static bool
writePixelShader(std::vector<uint8_t> &fh,
                 const GFDPixelShader &psh)
{
   std::vector<uint8_t> text;
   std::vector<DataPatch> dataPatches;
   std::vector<TextPatch> textPatches;
   decaf_check(fh.size() == 0);

   write<uint32_t>(fh, psh.regs.sq_pgm_resources_ps.value);
   write<uint32_t>(fh, psh.regs.sq_pgm_exports_ps.value);
   write<uint32_t>(fh, psh.regs.spi_ps_in_control_0.value);
   write<uint32_t>(fh, psh.regs.spi_ps_in_control_1.value);
   write<uint32_t>(fh, psh.regs.num_spi_ps_input_cntl);

   for (auto &spi_ps_input_cntl : psh.regs.spi_ps_input_cntls) {
      write<uint32_t>(fh, spi_ps_input_cntl.value);
   }

   write<uint32_t>(fh, psh.regs.cb_shader_mask.value);
   write<uint32_t>(fh, psh.regs.cb_shader_control.value);
   write<uint32_t>(fh, psh.regs.db_shader_control.value);
   write<uint32_t>(fh, psh.regs.spi_input_z.value);

   write<uint32_t>(fh, static_cast<uint32_t>(psh.data.size()));
   write<uint32_t>(fh, 0); // psh.data
   write<uint32_t>(fh, static_cast<uint32_t>(psh.mode));

   auto uniformBlocksPatch = DataPatch { fh.size() + 4, 0 };
   write<uint32_t>(fh, static_cast<uint32_t>(psh.uniformBlocks.size()));
   write<uint32_t>(fh, 0); // vsh.uniformBlocks.data

   auto uniformVarsPatch = DataPatch { fh.size() + 4, 0 };
   write<uint32_t>(fh, static_cast<uint32_t>(psh.uniformVars.size()));
   write<uint32_t>(fh, 0); // vsh.uniformVars.data

   auto initialValuesPatch = DataPatch { fh.size() + 4, 0 };
   write<uint32_t>(fh, static_cast<uint32_t>(psh.initialValues.size()));
   write<uint32_t>(fh, 0); // vsh.initialValues.data

   auto loopVarsPatch = DataPatch { fh.size() + 4, 0 };
   write<uint32_t>(fh, static_cast<uint32_t>(psh.loopVars.size()));
   write<uint32_t>(fh, 0); // vsh.loopVars.data

   auto samplerVarsPatch = DataPatch { fh.size() + 4, 0 };
   write<uint32_t>(fh, static_cast<uint32_t>(psh.samplerVars.size()));
   write<uint32_t>(fh, 0); // vsh.samplerVars.data

   writeGX2RBuffer(fh, psh.gx2rData);
   decaf_check(fh.size() == 0xE8);

   // Now write relocated data
   writeUniformBlocksData(fh, uniformBlocksPatch, dataPatches, textPatches, psh.uniformBlocks);
   writeUniformVarsData(fh, uniformVarsPatch, dataPatches, textPatches, psh.uniformVars);
   writeInitialValuesData(fh, initialValuesPatch, dataPatches, textPatches, psh.initialValues);
   writeLoopVarsData(fh, loopVarsPatch, dataPatches, textPatches, psh.loopVars);
   writeSamplerVarsData(fh, samplerVarsPatch, dataPatches, textPatches, psh.samplerVars);
   writeRelocationData(fh, dataPatches, textPatches);
   return true;
}

static bool
writeGeometryShader(std::vector<uint8_t> &fh,
                    const GFDGeometryShader &gsh)
{
   std::vector<uint8_t> text;
   std::vector<DataPatch> dataPatches;
   std::vector<TextPatch> textPatches;
   decaf_check(fh.size() == 0);

   write<uint32_t>(fh, gsh.regs.sq_pgm_resources_gs.value);
   write<uint32_t>(fh, gsh.regs.vgt_gs_out_prim_type.value);
   write<uint32_t>(fh, gsh.regs.vgt_gs_mode.value);
   write<uint32_t>(fh, gsh.regs.pa_cl_vs_out_cntl.value);
   write<uint32_t>(fh, gsh.regs.sq_pgm_resources_vs.value);
   write<uint32_t>(fh, gsh.regs.sq_gs_vert_itemsize.value);
   write<uint32_t>(fh, gsh.regs.spi_vs_out_config.value);
   write<uint32_t>(fh, gsh.regs.num_spi_vs_out_id);

   for (auto &spi_vs_out_id : gsh.regs.spi_vs_out_id) {
      write<uint32_t>(fh, spi_vs_out_id.value);
   }

   write<uint32_t>(fh, gsh.regs.vgt_strmout_buffer_en.value);

   write<uint32_t>(fh, static_cast<uint32_t>(gsh.data.size()));
   write<uint32_t>(fh, 0); // gsh.data

   write<uint32_t>(fh, static_cast<uint32_t>(gsh.vertexShaderData.size()));
   write<uint32_t>(fh, 0); // gsh.vertexShaderData

   write<uint32_t>(fh, static_cast<uint32_t>(gsh.mode));

   auto uniformBlocksPatch = DataPatch { fh.size() + 4, 0 };
   write<uint32_t>(fh, static_cast<uint32_t>(gsh.uniformBlocks.size()));
   write<uint32_t>(fh, 0); // vsh.uniformBlocks.data

   auto uniformVarsPatch = DataPatch { fh.size() + 4, 0 };
   write<uint32_t>(fh, static_cast<uint32_t>(gsh.uniformVars.size()));
   write<uint32_t>(fh, 0); // vsh.uniformVars.data

   auto initialValuesPatch = DataPatch { fh.size() + 4, 0 };
   write<uint32_t>(fh, static_cast<uint32_t>(gsh.initialValues.size()));
   write<uint32_t>(fh, 0); // vsh.initialValues.data

   auto loopVarsPatch = DataPatch { fh.size() + 4, 0 };
   write<uint32_t>(fh, static_cast<uint32_t>(gsh.loopVars.size()));
   write<uint32_t>(fh, 0); // vsh.loopVars.data

   auto samplerVarsPatch = DataPatch { fh.size() + 4, 0 };
   write<uint32_t>(fh, static_cast<uint32_t>(gsh.samplerVars.size()));
   write<uint32_t>(fh, 0); // vsh.samplerVars.data

   write<uint32_t>(fh, gsh.ringItemSize);
   write<uint32_t>(fh, gsh.hasStreamOut ? 1 : 0);

   for (auto &stride : gsh.streamOutStride) {
      write<uint32_t>(fh, stride);
   }

   writeGX2RBuffer(fh, gsh.gx2rData);
   writeGX2RBuffer(fh, gsh.gx2rVertexShaderData);
   decaf_check(fh.size() == 0xC0);

   // Now write relocated data
   writeUniformBlocksData(fh, uniformBlocksPatch, dataPatches, textPatches, gsh.uniformBlocks);
   writeUniformVarsData(fh, uniformVarsPatch, dataPatches, textPatches, gsh.uniformVars);
   writeInitialValuesData(fh, initialValuesPatch, dataPatches, textPatches, gsh.initialValues);
   writeLoopVarsData(fh, loopVarsPatch, dataPatches, textPatches, gsh.loopVars);
   writeSamplerVarsData(fh, samplerVarsPatch, dataPatches, textPatches, gsh.samplerVars);
   writeRelocationData(fh, dataPatches, textPatches);
   return true;
}

static bool
writeTexture(MemoryFile &fh,
             const GFDTexture &texture)
{
   write<uint32_t>(fh, static_cast<uint32_t>(texture.surface.dim));
   write<uint32_t>(fh, texture.surface.width);
   write<uint32_t>(fh, texture.surface.height);
   write<uint32_t>(fh, texture.surface.depth);
   write<uint32_t>(fh, texture.surface.mipLevels);
   write<uint32_t>(fh, static_cast<uint32_t>(texture.surface.format));
   write<uint32_t>(fh, static_cast<uint32_t>(texture.surface.aa));
   write<uint32_t>(fh, static_cast<uint32_t>(texture.surface.use));

   write<uint32_t>(fh, static_cast<uint32_t>(texture.surface.image.size()));
   write<uint32_t>(fh, 0); // texture.surface.image

   write<uint32_t>(fh, static_cast<uint32_t>(texture.surface.mipmap.size()));
   write<uint32_t>(fh, 0); // texture.surface.mipmap

   write<uint32_t>(fh, static_cast<uint32_t>(texture.surface.tileMode));
   write<uint32_t>(fh, texture.surface.swizzle);
   write<uint32_t>(fh, texture.surface.alignment);
   write<uint32_t>(fh, texture.surface.pitch);

   for (auto &mipLevelOffset : texture.surface.mipLevelOffset) {
      write<uint32_t>(fh, mipLevelOffset);
   }

   write<uint32_t>(fh, texture.viewFirstMip);
   write<uint32_t>(fh, texture.viewNumMips);
   write<uint32_t>(fh, texture.viewFirstSlice);
   write<uint32_t>(fh, texture.viewNumSlices);
   write<uint32_t>(fh, texture.compMap);

   write<uint32_t>(fh, texture.regs.word0.value);
   write<uint32_t>(fh, texture.regs.word1.value);
   write<uint32_t>(fh, texture.regs.word4.value);
   write<uint32_t>(fh, texture.regs.word5.value);
   write<uint32_t>(fh, texture.regs.word6.value);
   decaf_check(fh.size() == 0x9C);
   return true;
}

static bool
writeBlock(MemoryFile &fh,
           const GFDBlockHeader &header,
           const std::vector<uint8_t> &data)
{
   write<uint32_t>(fh, GFDBlockHeader::Magic);
   write<uint32_t>(fh, GFDBlockHeader::HeaderSize);
   write<uint32_t>(fh, header.majorVersion);
   write<uint32_t>(fh, header.minorVersion);
   write<uint32_t>(fh, static_cast<uint32_t>(header.type));
   write<uint32_t>(fh, static_cast<uint32_t>(data.size()));
   write<uint32_t>(fh, header.id);
   write<uint32_t>(fh, header.index);
   writeBinary(fh, data);
   return true;
}

static bool
alignNextBlock(MemoryFile &fh,
               uint32_t &blockID)
{
   auto paddingSize = (0x200 - ((fh.size() + sizeof(GFDBlockHeader)) & 0x1FF)) & 0x1FF;

   if (paddingSize) {
      if (paddingSize < sizeof(GFDBlockHeader)) {
         paddingSize += 0x200;
      }

      paddingSize -= sizeof(GFDBlockHeader);

      GFDBlockHeader paddingHeader;
      paddingHeader.majorVersion = GFDBlockMajorVersion;
      paddingHeader.minorVersion = 0;
      paddingHeader.type = GFDBlockType::Padding;
      paddingHeader.id = blockID++;
      paddingHeader.index = 0;

      writeBlock(fh, paddingHeader, std::vector<uint8_t>(paddingSize, 0));
   }

   return true;
}

bool
writeFile(const GFDFile &file,
          const std::string &path,
          bool align)
{
   MemoryFile fh;
   auto blockID = uint32_t { 0 };

   // Write File Header
   write<uint32_t>(fh, GFDFileHeader::Magic);
   write<uint32_t>(fh, GFDFileHeader::HeaderSize);
   write<uint32_t>(fh, GFDFileMajorVersion);
   write<uint32_t>(fh, GFDFileMinorVersion);
   write<uint32_t>(fh, GFDFileGpuVersion);
   write<uint32_t>(fh, align ? 1 : 0); // align
   write<uint32_t>(fh, 0); // unk1
   write<uint32_t>(fh, 0); // unk2

   // Write vertex shaders
   for (auto i = 0u; i < file.vertexShaders.size(); ++i) {
      std::vector<uint8_t> vertexShaderHeader;
      writeVertexShader(vertexShaderHeader, file.vertexShaders[i]);

      GFDBlockHeader vshHeader;
      vshHeader.majorVersion = GFDBlockMajorVersion;
      vshHeader.minorVersion = 0;
      vshHeader.type = GFDBlockType::VertexShaderHeader;
      vshHeader.id = blockID++;
      vshHeader.index = i;
      writeBlock(fh, vshHeader, vertexShaderHeader);

      if (align) {
         alignNextBlock(fh, blockID);
      }

      GFDBlockHeader dataHeader;
      dataHeader.majorVersion = GFDBlockMajorVersion;
      dataHeader.minorVersion = 0;
      dataHeader.type = GFDBlockType::VertexShaderProgram;
      dataHeader.id = blockID++;
      dataHeader.index = i;
      writeBlock(fh, dataHeader, file.vertexShaders[i].data);
   }

   // Write pixel shaders
   for (auto i = 0u; i < file.pixelShaders.size(); ++i) {
      std::vector<uint8_t> pixelShaderHeader;
      writePixelShader(pixelShaderHeader, file.pixelShaders[i]);

      GFDBlockHeader pshHeader;
      pshHeader.majorVersion = GFDBlockMajorVersion;
      pshHeader.minorVersion = 0;
      pshHeader.type = GFDBlockType::PixelShaderHeader;
      pshHeader.id = blockID++;
      pshHeader.index = i;
      writeBlock(fh, pshHeader, pixelShaderHeader);

      if (align) {
         alignNextBlock(fh, blockID);
      }

      GFDBlockHeader dataHeader;
      dataHeader.majorVersion = GFDBlockMajorVersion;
      dataHeader.minorVersion = 0;
      dataHeader.type = GFDBlockType::PixelShaderProgram;
      dataHeader.id = blockID++;
      dataHeader.index = i;
      writeBlock(fh, dataHeader, file.pixelShaders[i].data);
   }

   // Write geometry shaders
   for (auto i = 0u; i < file.geometryShaders.size(); ++i) {
      std::vector<uint8_t> geometryShaderHeader;
      writeGeometryShader(geometryShaderHeader, file.geometryShaders[i]);

      GFDBlockHeader gshHeader;
      gshHeader.majorVersion = GFDBlockMajorVersion;
      gshHeader.minorVersion = 0;
      gshHeader.type = GFDBlockType::GeometryShaderHeader;
      gshHeader.id = blockID++;
      gshHeader.index = i;
      writeBlock(fh, gshHeader, geometryShaderHeader);

      if (align) {
         alignNextBlock(fh, blockID);
      }

      if (file.geometryShaders[i].data.size()) {
         GFDBlockHeader dataHeader;
         dataHeader.majorVersion = GFDBlockMajorVersion;
         dataHeader.minorVersion = 0;
         dataHeader.type = GFDBlockType::GeometryShaderProgram;
         dataHeader.id = blockID++;
         dataHeader.index = i;
         writeBlock(fh, dataHeader, file.geometryShaders[i].data);
      }

      if (align) {
         alignNextBlock(fh, blockID);
      }

      if (file.geometryShaders[i].vertexShaderData.size()) {
         GFDBlockHeader dataHeader;
         dataHeader.majorVersion = GFDBlockMajorVersion;
         dataHeader.minorVersion = 0;
         dataHeader.type = GFDBlockType::GeometryShaderCopyProgram;
         dataHeader.id = blockID++;
         dataHeader.index = i;
         writeBlock(fh, dataHeader, file.geometryShaders[i].vertexShaderData);
      }
   }

   // Write textures
   for (auto i = 0u; i < file.textures.size(); ++i) {
      std::vector<uint8_t> textureHeader;
      writeTexture(textureHeader, file.textures[i]);

      GFDBlockHeader texHeader;
      texHeader.majorVersion = GFDBlockMajorVersion;
      texHeader.minorVersion = 0;
      texHeader.type = GFDBlockType::TextureHeader;
      texHeader.id = blockID++;
      texHeader.index = i;
      writeBlock(fh, texHeader, textureHeader);

      if (align) {
         alignNextBlock(fh, blockID);
      }

      GFDBlockHeader imageHeader;
      imageHeader.majorVersion = GFDBlockMajorVersion;
      imageHeader.minorVersion = 0;
      imageHeader.type = GFDBlockType::TextureImage;
      imageHeader.id = blockID++;
      imageHeader.index = i;
      writeBlock(fh, imageHeader, file.textures[i].surface.image);

      if (align) {
         alignNextBlock(fh, blockID);
      }

      if (file.textures[i].surface.mipmap.size()) {
         GFDBlockHeader mipmapHeader;
         mipmapHeader.majorVersion = GFDBlockMajorVersion;
         mipmapHeader.minorVersion = 0;
         mipmapHeader.type = GFDBlockType::TextureImage;
         mipmapHeader.id = blockID++;
         mipmapHeader.index = i;
         writeBlock(fh, mipmapHeader, file.textures[i].surface.mipmap);
      }
   }

   // Write EOF
   GFDBlockHeader eofHeader;
   eofHeader.majorVersion = GFDBlockMajorVersion;
   eofHeader.minorVersion = 0;
   eofHeader.type = GFDBlockType::EndOfFile;
   eofHeader.id = blockID++;
   eofHeader.index = 0;
   writeBlock(fh, eofHeader, {});

   std::ofstream out { path, std::ofstream::binary };
   out.write(reinterpret_cast<const char *>(fh.data()), fh.size());
   return true;
}

} // namespace gfd
