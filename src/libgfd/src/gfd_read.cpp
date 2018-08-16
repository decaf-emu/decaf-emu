#include "gfd.h"

#include <common/byte_swap.h>
#include <fstream>
#include <fmt/format.h>
#include <string>

namespace gfd
{

struct MemoryFile
{
   bool eof()
   {
      return pos >= data.size();
   }

   uint32_t pos = 0;
   std::vector<uint8_t> data;
};

static bool
openFile(const std::string &path,
         MemoryFile & file)
{
   std::ifstream fh { path, std::ifstream::binary };
   if (!fh.is_open()) {
      return false;
   }

   fh.seekg(0, std::istream::end);
   file.data.resize(fh.tellg());
   fh.seekg(0);
   fh.read(reinterpret_cast<char *>(file.data.data()), file.data.size());
   file.pos = 0;
   return true;
}

template<typename Type>
inline Type
read(MemoryFile &fh)
{
   if (fh.pos + sizeof(Type) > fh.data.size()) {
      throw GFDReadException { "Tried to read past end of file" };
   }

   auto value = byte_swap(*reinterpret_cast<Type *>(fh.data.data() + fh.pos));
   fh.pos += sizeof(Type);
   return value;
}

inline void
readBinary(MemoryFile &fh,
           std::vector<uint8_t> &value,
           uint32_t size)
{
   if (fh.pos + size > fh.data.size()) {
      throw GFDReadException { "Tried to read past end of file" };
   }

   value.resize(size);
   std::memcpy(value.data(), fh.data.data() + fh.pos, size);
   fh.pos += size;
}

static bool
readFileHeader(MemoryFile &fh,
               GFDFileHeader &header)
{
   header.magic = read<uint32_t>(fh);
   if (header.magic != GFDFileHeader::Magic) {
      throw GFDReadException { fmt::format("Unexpected file magic {:X}", header.magic) };
   }

   header.headerSize = read<uint32_t>(fh);
   if (header.headerSize != GFDFileHeader::HeaderSize) {
      throw GFDReadException { fmt::format("Unexpected file header size {}",
                                           header.headerSize) };
   }

   header.majorVersion = read<uint32_t>(fh);
   if (header.majorVersion != GFDFileMajorVersion) {
      throw GFDReadException { fmt::format("Unsupported file version {}.{}.{}",
                                           header.majorVersion, header.minorVersion, header.gpuVersion) };
   }

   header.minorVersion = read<uint32_t>(fh);
   header.gpuVersion = read<uint32_t>(fh);
   header.align = read<uint32_t>(fh);
   header.unk1 = read<uint32_t>(fh);
   header.unk2 = read<uint32_t>(fh);
   return true;
}

static bool
readBlockHeader(MemoryFile &fh,
                GFDBlockHeader &header)
{
   if (fh.eof()) {
      return false;
   }

   header.magic = read<uint32_t>(fh);
   if (header.magic != GFDBlockHeader::Magic) {
      throw GFDReadException { fmt::format("Unexpected block magic {:X}", header.magic) };
   }

   header.headerSize = read<uint32_t>(fh);
   if (header.headerSize != GFDBlockHeader::HeaderSize) {
      throw GFDReadException { fmt::format("Unexpected block header size {:X}", header.headerSize) };
   }

   header.majorVersion = read<uint32_t>(fh);
   header.minorVersion = read<uint32_t>(fh);
   header.type = static_cast<GFDBlockType>(read<uint32_t>(fh));
   header.dataSize = read<uint32_t>(fh);
   header.id = read<uint32_t>(fh);
   header.index = read<uint32_t>(fh);
   return true;
}

static bool
readRelocationHeader(MemoryFile &fh,
                     GFDRelocationHeader &header)
{
   header.magic = read<uint32_t>(fh);
   if (header.magic != GFDRelocationHeader::Magic) {
      return false;
   }

   header.headerSize = read<uint32_t>(fh);
   if (header.headerSize != GFDRelocationHeader::HeaderSize) {
      throw GFDReadException { fmt::format("Unexpected relocation header size {:X}", header.headerSize) };
   }

   header.unk1 = read<uint32_t>(fh);
   header.dataSize = read<uint32_t>(fh);
   header.dataOffset = read<uint32_t>(fh);
   header.textSize = read<uint32_t>(fh);
   header.textOffset = read<uint32_t>(fh);
   header.patchBase = read<uint32_t>(fh);
   header.patchCount = read<uint32_t>(fh);
   header.patchOffset = read<uint32_t>(fh);
   return true;
}

static bool
readBlockRelocations(MemoryFile &fh,
                     uint32_t blockBase,
                     uint32_t blockSize,
                     GFDBlockRelocations &block)
{
   fh.pos = blockBase + blockSize - GFDRelocationHeader::HeaderSize;

   if (!readRelocationHeader(fh, block.header)) {
      return false;
   }

   fh.pos = blockBase + (block.header.patchOffset & ~GFDPatchMask);

   for (auto i = 0u; i < block.header.patchCount; ++i) {
      block.patches.push_back(read<uint32_t>(fh));
   }

   return true;
}

static bool
checkRelocation(const GFDBlockRelocations &relocations,
                uint32_t pos)
{
   for (auto &patch : relocations.patches) {
      if (pos == (patch & ~GFDPatchMask)) {
         return true;
      }
   }

   return false;
}

static std::string
readString(MemoryFile &fh,
           uint32_t blockBase,
           const GFDBlockRelocations &relocations)
{
   auto offset = read<uint32_t>(fh) & ~GFDPatchMask;
   if (!checkRelocation(relocations, fh.pos - 4 - blockBase)) {
      return { };
   }

   return reinterpret_cast<const char *>(fh.data.data() + blockBase + offset);
}

static bool
readUniformBlocks(MemoryFile &fh,
                  uint32_t blockBase,
                  const GFDBlockRelocations &relocations,
                  std::vector<GFDUniformBlock> &uniformBlocks)
{
   auto pos = fh.pos;
   auto count = read<uint32_t>(fh);
   auto offset = read<uint32_t>(fh) & ~GFDPatchMask;

   if (!count) {
      return true;
   }

   decaf_check(offset);
   decaf_check(checkRelocation(relocations, pos + 4 - blockBase));

   fh.pos = blockBase + offset;
   uniformBlocks.resize(count);

   for (auto &block : uniformBlocks) {
      block.name = readString(fh, blockBase, relocations);
      block.offset = read<uint32_t>(fh);
      block.size = read<uint32_t>(fh);
   }

   fh.pos = pos + 8;
   return true;
}

static bool
readUniformVars(MemoryFile &fh,
                uint32_t blockBase,
                const GFDBlockRelocations &relocations,
                std::vector<GFDUniformVar> &uniformVars)
{
   auto pos = fh.pos;
   auto count = read<uint32_t>(fh);
   auto offset = read<uint32_t>(fh) & ~GFDPatchMask;

   if (!count) {
      return true;
   }

   decaf_check(offset);
   decaf_check(checkRelocation(relocations, pos + 4 - blockBase));

   fh.pos = blockBase + offset;
   uniformVars.resize(count);

   for (auto &var : uniformVars) {
      var.name = readString(fh, blockBase, relocations);
      var.type = static_cast<GX2ShaderVarType>(read<uint32_t>(fh));
      var.count = read<uint32_t>(fh);
      var.offset = read<uint32_t>(fh);
      var.block = read<int32_t>(fh);
   }

   fh.pos = pos + 8;
   return true;
}

static bool
readInitialValues(MemoryFile &fh,
                  uint32_t blockBase,
                  const GFDBlockRelocations &relocations,
                  std::vector<GFDUniformInitialValue> &initialValues)
{
   auto pos = fh.pos;
   auto count = read<uint32_t>(fh);
   auto offset = read<uint32_t>(fh) & ~GFDPatchMask;

   if (!count) {
      return true;
   }

   decaf_check(offset);
   decaf_check(checkRelocation(relocations, pos + 4 - blockBase));

   fh.pos = blockBase + offset;
   initialValues.resize(count);

   for (auto &var : initialValues) {
      for (auto &value : var.value) {
         value = read<float>(fh);
      }

      var.offset = read<uint32_t>(fh);
   }

   fh.pos = pos + 8;
   return true;
}

static bool
readLoopVars(MemoryFile &fh,
             uint32_t blockBase,
             const GFDBlockRelocations &relocations,
             std::vector<GFDLoopVar> &loopVars)
{
   auto pos = fh.pos;
   auto count = read<uint32_t>(fh);
   auto offset = read<uint32_t>(fh) & ~GFDPatchMask;

   if (!count) {
      return true;
   }

   decaf_check(offset);
   decaf_check(checkRelocation(relocations, pos + 4 - blockBase));

   fh.pos = blockBase + offset;
   loopVars.resize(count);

   for (auto &var : loopVars) {
      var.offset = read<uint32_t>(fh);
      var.value = read<uint32_t>(fh);
   }

   fh.pos = pos + 8;
   return true;
}

static bool
readSamplerVars(MemoryFile &fh,
                uint32_t blockBase,
                const GFDBlockRelocations &relocations,
                std::vector<GFDSamplerVar> &samplerVars)
{
   auto pos = fh.pos;
   auto count = read<uint32_t>(fh);
   auto offset = read<uint32_t>(fh) & ~GFDPatchMask;

   if (!count) {
      return true;
   }

   decaf_check(offset);
   decaf_check(checkRelocation(relocations, pos + 4 - blockBase));

   fh.pos = blockBase + offset;
   samplerVars.resize(count);

   for (auto &var : samplerVars) {
      var.name = readString(fh, blockBase, relocations);
      var.type = static_cast<GX2SamplerVarType>(read<uint32_t>(fh));
      var.location = read<uint32_t>(fh);
   }

   fh.pos = pos + 8;
   return true;
}

static bool
readAttribVars(MemoryFile &fh,
               uint32_t blockBase,
               const GFDBlockRelocations &relocations,
               std::vector<GFDAttribVar> &attribVars)
{
   auto pos = fh.pos;
   auto count = read<uint32_t>(fh);
   auto offset = read<uint32_t>(fh) & ~GFDPatchMask;

   if (!count) {
      return true;
   }

   decaf_check(offset);
   decaf_check(checkRelocation(relocations, pos + 4 - blockBase));

   fh.pos = blockBase + offset;
   attribVars.resize(count);

   for (auto &var : attribVars) {
      var.name = readString(fh, blockBase, relocations);
      var.type = static_cast<GX2ShaderVarType>(read<uint32_t>(fh));
      var.count = read<uint32_t>(fh);
      var.location = read<uint32_t>(fh);
   }

   fh.pos = pos + 8;
   return true;
}

static bool
readGx2rBuffer(MemoryFile &fh,
               GFDRBuffer &gx2r)
{
   gx2r.flags = static_cast<GX2RResourceFlags>(read<uint32_t>(fh));
   gx2r.elemSize = read<uint32_t>(fh);
   gx2r.elemCount = read<uint32_t>(fh);
   decaf_check(read<uint32_t>(fh) == 0);
   return true;
}

static bool
readVertexShaderHeader(MemoryFile &fh,
                       GFDBlockHeader &block,
                       GFDVertexShader &vsh)
{
   GFDBlockRelocations relocations;
   auto blockBase = fh.pos;

   if (!readBlockRelocations(fh, blockBase, block.dataSize, relocations)) {
      return false;
   }

   fh.pos = blockBase;

   if (block.majorVersion == 1) {
      vsh.regs.sq_pgm_resources_vs = latte::SQ_PGM_RESOURCES_VS::get(read<uint32_t>(fh));
      vsh.regs.vgt_primitiveid_en = latte::VGT_PRIMITIVEID_EN::get(read<uint32_t>(fh));
      vsh.regs.spi_vs_out_config = latte::SPI_VS_OUT_CONFIG::get(read<uint32_t>(fh));
      vsh.regs.num_spi_vs_out_id = read<uint32_t>(fh);

      for (auto i = 0u; i < vsh.regs.spi_vs_out_id.size(); ++i) {
         vsh.regs.spi_vs_out_id[i] = latte::SPI_VS_OUT_ID_N::get(read<uint32_t>(fh));
      }

      vsh.regs.pa_cl_vs_out_cntl = latte::PA_CL_VS_OUT_CNTL::get(read<uint32_t>(fh));
      vsh.regs.sq_vtx_semantic_clear = latte::SQ_VTX_SEMANTIC_CLEAR::get(read<uint32_t>(fh));
      vsh.regs.num_sq_vtx_semantic = read<uint32_t>(fh);

      for (auto i = 0u; i < vsh.regs.sq_vtx_semantic.size(); ++i) {
         vsh.regs.sq_vtx_semantic[i] = latte::SQ_VTX_SEMANTIC_N::get(read<uint32_t>(fh));
      }

      vsh.regs.vgt_strmout_buffer_en = latte::VGT_STRMOUT_BUFFER_EN::get(read<uint32_t>(fh));
      vsh.regs.vgt_vertex_reuse_block_cntl = latte::VGT_VERTEX_REUSE_BLOCK_CNTL::get(read<uint32_t>(fh));
      vsh.regs.vgt_hos_reuse_depth = latte::VGT_HOS_REUSE_DEPTH::get(read<uint32_t>(fh));
      decaf_check(fh.pos - blockBase == 0xD0);
   } else {
      throw GFDReadException { fmt::format("Unsupported VertexShaderHeader version {}.{}",
                                           block.majorVersion, block.minorVersion) };
   }

   vsh.data.reserve(read<uint32_t>(fh));
   decaf_check(read<uint32_t>(fh) == 0); // vsh.data
   vsh.mode = static_cast<GX2ShaderMode>(read<uint32_t>(fh));

   readUniformBlocks(fh, blockBase, relocations, vsh.uniformBlocks);
   readUniformVars(fh, blockBase, relocations, vsh.uniformVars);
   readInitialValues(fh, blockBase, relocations, vsh.initialValues);
   readLoopVars(fh, blockBase, relocations, vsh.loopVars);
   readSamplerVars(fh, blockBase, relocations, vsh.samplerVars);
   readAttribVars(fh, blockBase, relocations, vsh.attribVars);

   vsh.ringItemSize = read<uint32_t>(fh);
   vsh.hasStreamOut = read<uint32_t>(fh) ? true : false;

   for (auto &stride : vsh.streamOutStride) {
      stride = read<uint32_t>(fh);
   }

   readGx2rBuffer(fh, vsh.gx2rData);

   if (block.majorVersion == 1) {
      decaf_check(fh.pos - blockBase == 0x134);
   }

   fh.pos = blockBase + block.dataSize;
   return true;
}

static bool
readVertexShaderProgram(MemoryFile &fh,
                        GFDBlockHeader &block,
                        GFDVertexShader &vsh)
{
   if (block.dataSize != vsh.data.capacity()) {
      throw GFDReadException { fmt::format("VertexShaderProgram block.dataSize {} != vsh.size {}",
                                           block.dataSize, vsh.data.capacity()) };
   }

   readBinary(fh, vsh.data, block.dataSize);
   return true;
}

static bool
readPixelShaderHeader(MemoryFile &fh,
                      GFDBlockHeader &block,
                      GFDPixelShader &psh)
{
   GFDBlockRelocations relocations;
   auto blockBase = fh.pos;

   if (!readBlockRelocations(fh, blockBase, block.dataSize, relocations)) {
      return false;
   }

   if (block.majorVersion != 0 && block.majorVersion != 1) {
      throw GFDReadException { fmt::format("Unsupported PixelShaderHeader version {}.{}",
                                           block.majorVersion, block.minorVersion) };
   }

   fh.pos = blockBase;

   psh.regs.sq_pgm_resources_ps = latte::SQ_PGM_RESOURCES_PS::get(read<uint32_t>(fh));
   psh.regs.sq_pgm_exports_ps = latte::SQ_PGM_EXPORTS_PS::get(read<uint32_t>(fh));
   psh.regs.spi_ps_in_control_0 = latte::SPI_PS_IN_CONTROL_0::get(read<uint32_t>(fh));
   psh.regs.spi_ps_in_control_1 = latte::SPI_PS_IN_CONTROL_1::get(read<uint32_t>(fh));
   psh.regs.num_spi_ps_input_cntl = read<uint32_t>(fh);

   for (auto &spi_ps_input_cntl : psh.regs.spi_ps_input_cntls) {
      spi_ps_input_cntl = latte::SPI_PS_INPUT_CNTL_N::get(read<uint32_t>(fh));
   }

   psh.regs.cb_shader_mask = latte::CB_SHADER_MASK::get(read<uint32_t>(fh));
   psh.regs.cb_shader_control = latte::CB_SHADER_CONTROL::get(read<uint32_t>(fh));
   psh.regs.db_shader_control = latte::DB_SHADER_CONTROL::get(read<uint32_t>(fh));
   psh.regs.spi_input_z = latte::SPI_INPUT_Z::get(read<uint32_t>(fh));

   psh.data.reserve(read<uint32_t>(fh));
   decaf_check(read<uint32_t>(fh) == 0); // psh.data
   psh.mode = static_cast<GX2ShaderMode>(read<uint32_t>(fh));

   readUniformBlocks(fh, blockBase, relocations, psh.uniformBlocks);
   readUniformVars(fh, blockBase, relocations, psh.uniformVars);
   readInitialValues(fh, blockBase, relocations, psh.initialValues);
   readLoopVars(fh, blockBase, relocations, psh.loopVars);
   readSamplerVars(fh, blockBase, relocations, psh.samplerVars);

   if (block.majorVersion == 0) {
      std::memset(&psh.gx2rData, 0, sizeof(GFDRBuffer));
      decaf_check(fh.pos - blockBase == 0xD8);
   } else if (block.majorVersion == 1) {
      readGx2rBuffer(fh, psh.gx2rData);
      decaf_check(fh.pos - blockBase == 0xE8);
   }

   fh.pos = blockBase + block.dataSize;
   return true;
}

static bool
readPixelShaderProgram(MemoryFile &fh,
                       GFDBlockHeader &block,
                       GFDPixelShader &psh)
{
   if (block.dataSize != psh.data.capacity()) {
      throw GFDReadException { fmt::format("PixelShaderProgram block.dataSize {} != vsh.size {}",
                                           block.dataSize, psh.data.capacity()) };
   }

   readBinary(fh, psh.data, block.dataSize);
   return true;
}

static bool
readGeometryShaderHeader(MemoryFile &fh,
                         GFDBlockHeader &block,
                         GFDGeometryShader &gsh)
{
   GFDBlockRelocations relocations;
   auto blockBase = fh.pos;

   if (!readBlockRelocations(fh, blockBase, block.dataSize, relocations)) {
      return false;
   }

   if (block.majorVersion != 1) {
      throw GFDReadException { fmt::format("Unsupported GeometryShaderHeader version {}.{}",
                                           block.majorVersion, block.minorVersion) };
   }

   fh.pos = blockBase;

   gsh.regs.sq_pgm_resources_gs = latte::SQ_PGM_RESOURCES_GS::get(read<uint32_t>(fh));
   gsh.regs.vgt_gs_out_prim_type = latte::VGT_GS_OUT_PRIM_TYPE::get(read<uint32_t>(fh));
   gsh.regs.vgt_gs_mode = latte::VGT_GS_MODE::get(read<uint32_t>(fh));
   gsh.regs.pa_cl_vs_out_cntl = latte::PA_CL_VS_OUT_CNTL::get(read<uint32_t>(fh));
   gsh.regs.sq_pgm_resources_vs = latte::SQ_PGM_RESOURCES_VS::get(read<uint32_t>(fh));
   gsh.regs.sq_gs_vert_itemsize = latte::SQ_GS_VERT_ITEMSIZE::get(read<uint32_t>(fh));
   gsh.regs.spi_vs_out_config = latte::SPI_VS_OUT_CONFIG::get(read<uint32_t>(fh));
   gsh.regs.num_spi_vs_out_id = read<uint32_t>(fh);

   for (auto &spi_vs_out_id : gsh.regs.spi_vs_out_id) {
      spi_vs_out_id = latte::SPI_VS_OUT_ID_N::get(read<uint32_t>(fh));
   }

   gsh.regs.vgt_strmout_buffer_en = latte::VGT_STRMOUT_BUFFER_EN::get(read<uint32_t>(fh));

   gsh.data.reserve(read<uint32_t>(fh));
   decaf_check(read<uint32_t>(fh) == 0); // psh.data

   gsh.vertexShaderData.reserve(read<uint32_t>(fh));
   decaf_check(read<uint32_t>(fh) == 0); // psh.vertexShaderData

   gsh.mode = static_cast<GX2ShaderMode>(read<uint32_t>(fh));
   readUniformBlocks(fh, blockBase, relocations, gsh.uniformBlocks);
   readUniformVars(fh, blockBase, relocations, gsh.uniformVars);
   readInitialValues(fh, blockBase, relocations, gsh.initialValues);
   readLoopVars(fh, blockBase, relocations, gsh.loopVars);
   readSamplerVars(fh, blockBase, relocations, gsh.samplerVars);

   gsh.ringItemSize = read<uint32_t>(fh);
   gsh.hasStreamOut = read<uint32_t>(fh) ? true : false;

   for (auto &stride : gsh.streamOutStride) {
      stride = read<uint32_t>(fh);
   }

   readGx2rBuffer(fh, gsh.gx2rData);
   readGx2rBuffer(fh, gsh.gx2rVertexShaderData);

   if (block.majorVersion == 1) {
      decaf_check(fh.pos - blockBase == 0xC0);
   }

   fh.pos = blockBase + block.dataSize;
   return true;
}

static bool
readGeometryShaderProgram(MemoryFile &fh,
                          GFDBlockHeader &block,
                          GFDGeometryShader &gsh)
{
   if (block.dataSize != gsh.data.capacity()) {
      throw GFDReadException { fmt::format("GeometryShaderProgram block.dataSize {} != gsh.size {}",
                                           block.dataSize, gsh.data.capacity()) };
   }

   readBinary(fh, gsh.data, block.dataSize);
   return true;
}

static bool
readGeometryShaderCopyProgram(MemoryFile &fh,
                              GFDBlockHeader &block,
                              GFDGeometryShader &gsh)
{
   if (block.dataSize != gsh.vertexShaderData.capacity()) {
      throw GFDReadException { fmt::format("GeometryShaderCopyProgram block.dataSize {} != gsh.vertexShaderSize {}",
                                           block.dataSize, gsh.vertexShaderData.capacity()) };
   }

   readBinary(fh, gsh.vertexShaderData, block.dataSize);
   return true;
}

static bool
readTextureHeader(MemoryFile &fh,
                  GFDBlockHeader &block,
                  GFDTexture &tex)
{
   auto blockBase = fh.pos;

   if (block.majorVersion != 1) {
      throw GFDReadException { fmt::format("Unsupported TextureHeader version {}.{}",
                                           block.majorVersion, block.minorVersion) };
   }

   tex.surface.dim = static_cast<GX2SurfaceDim>(read<uint32_t>(fh));
   tex.surface.width = read<uint32_t>(fh);
   tex.surface.height = read<uint32_t>(fh);
   tex.surface.depth = read<uint32_t>(fh);
   tex.surface.mipLevels = read<uint32_t>(fh);
   tex.surface.format = static_cast<GX2SurfaceFormat>(read<uint32_t>(fh));
   tex.surface.aa = static_cast<GX2AAMode>(read<uint32_t>(fh));
   tex.surface.use = static_cast<GX2SurfaceUse>(read<uint32_t>(fh));

   tex.surface.image.reserve(read<uint32_t>(fh));
   decaf_check(read<uint32_t>(fh) == 0); // tex.surface.image

   tex.surface.mipmap.reserve(read<uint32_t>(fh));
   decaf_check(read<uint32_t>(fh) == 0); // tex.surface.mipmap

   tex.surface.tileMode = static_cast<GX2TileMode>(read<uint32_t>(fh));
   tex.surface.swizzle = read<uint32_t>(fh);
   tex.surface.alignment = read<uint32_t>(fh);
   tex.surface.pitch = read<uint32_t>(fh);

   for (auto &mipLevelOffset : tex.surface.mipLevelOffset) {
      mipLevelOffset = read<uint32_t>(fh);
   }

   tex.viewFirstMip = read<uint32_t>(fh);
   tex.viewNumMips = read<uint32_t>(fh);
   tex.viewFirstSlice = read<uint32_t>(fh);
   tex.viewNumSlices = read<uint32_t>(fh);
   tex.compMap = read<uint32_t>(fh);

   tex.regs.word0 = latte::SQ_TEX_RESOURCE_WORD0_N::get(read<uint32_t>(fh));
   tex.regs.word1 = latte::SQ_TEX_RESOURCE_WORD1_N::get(read<uint32_t>(fh));
   tex.regs.word4 = latte::SQ_TEX_RESOURCE_WORD4_N::get(read<uint32_t>(fh));
   tex.regs.word5 = latte::SQ_TEX_RESOURCE_WORD5_N::get(read<uint32_t>(fh));
   tex.regs.word6 = latte::SQ_TEX_RESOURCE_WORD6_N::get(read<uint32_t>(fh));

   if (block.majorVersion == 1) {
      decaf_check(fh.pos - blockBase == 0x9c);
   }

   fh.pos = blockBase + block.dataSize;
   return true;
}

static bool
readTextureImage(MemoryFile &fh,
                 GFDBlockHeader &block,
                 GFDTexture &tex)
{
   if (block.dataSize != tex.surface.image.capacity()) {
      throw GFDReadException { fmt::format("TextureImage block.dataSize {} != tex.surface.imageSize {}",
                                           block.dataSize, tex.surface.image.capacity()) };
   }

   readBinary(fh, tex.surface.image, block.dataSize);
   return true;
}

static bool
readTextureMipmap(MemoryFile &fh,
                  GFDBlockHeader &block,
                  GFDTexture &tex)
{
   if (block.dataSize != tex.surface.mipmap.capacity()) {
      throw GFDReadException { fmt::format("TextureMipmap block.dataSize {} != tex.surface.mipmapSize {}",
                                           block.dataSize, tex.surface.mipmap.capacity()) };
   }

   readBinary(fh, tex.surface.mipmap, block.dataSize);
   return true;
}

bool
readFile(GFDFile &file,
         const std::string &path)
{
   MemoryFile fh;
   GFDBlock block;
   GFDFileHeader header;

   if (!openFile(path, fh)) {
      return false;
   }

   if (!readFileHeader(fh, header)) {
      return false;
   }

   while (readBlockHeader(fh, block.header)) {
      auto pos = fh.pos;

      if (block.header.type == GFDBlockType::EndOfFile) {
         break;
      }

      switch (block.header.type) {
      case GFDBlockType::VertexShaderHeader:
      {
         file.vertexShaders.emplace_back();
         readVertexShaderHeader(fh, block.header, file.vertexShaders.back());
         break;
      }
      case GFDBlockType::VertexShaderProgram:
      {
         decaf_check(file.vertexShaders.size());
         readVertexShaderProgram(fh, block.header, file.vertexShaders.back());
         break;
      }
      case GFDBlockType::PixelShaderHeader:
      {
         file.pixelShaders.emplace_back();
         readPixelShaderHeader(fh, block.header, file.pixelShaders.back());
         break;
      }
      case GFDBlockType::PixelShaderProgram:
      {
         decaf_check(file.pixelShaders.size());
         readPixelShaderProgram(fh, block.header, file.pixelShaders.back());
         break;
      }
      case GFDBlockType::GeometryShaderHeader:
      {
         file.geometryShaders.emplace_back();
         readGeometryShaderHeader(fh, block.header, file.geometryShaders.back());
         break;
      }
      case GFDBlockType::GeometryShaderProgram:
      {
         decaf_check(file.geometryShaders.size());
         readGeometryShaderProgram(fh, block.header, file.geometryShaders.back());
         break;
      }
      case GFDBlockType::GeometryShaderCopyProgram:
      {
         decaf_check(file.geometryShaders.size());
         readGeometryShaderCopyProgram(fh, block.header, file.geometryShaders.back());
         break;
      }
      case GFDBlockType::TextureHeader:
      {
         file.textures.emplace_back();
         readTextureHeader(fh, block.header, file.textures.back());
         break;
      }
      case GFDBlockType::TextureImage:
      {
         decaf_check(file.textures.size());
         readTextureImage(fh, block.header, file.textures.back());
         break;
      }
      case GFDBlockType::TextureMipmap:
      {
         decaf_check(file.textures.size());
         readTextureMipmap(fh, block.header, file.textures.back());
         break;
      }
      case GFDBlockType::ComputeShaderHeader:
      case GFDBlockType::ComputeShaderProgram:
      case GFDBlockType::Padding:
      default:
         fh.pos += block.header.dataSize;
      }

      decaf_check(fh.pos == pos + block.header.dataSize);
   }

   return true;
}

} // namespace gfd
