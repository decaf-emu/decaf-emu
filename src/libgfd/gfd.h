#pragma once
#include "gfd_enum.h"
#include "gfd_gx2.h"

#include <cstdint>
#include <stdexcept>
#include <vector>

namespace gfd
{

struct GFDFileHeader
{
   static constexpr uint32_t Magic = 0x47667832;
   static constexpr uint32_t HeaderSize = 8 * 4;
   uint32_t magic;
   uint32_t headerSize;
   uint32_t majorVersion;
   uint32_t minorVersion;
   uint32_t gpuVersion;
   uint32_t align;
   uint32_t unk1;
   uint32_t unk2;
};

struct GFDBlockHeader
{
   static constexpr uint32_t Magic = 0x424C4B7B;
   static constexpr uint32_t HeaderSize = 8 * 4;
   uint32_t magic;
   uint32_t headerSize;
   uint32_t majorVersion;
   uint32_t minorVersion;
   GFDBlockType type;
   uint32_t dataSize;
   uint32_t id;
   uint32_t index;
};

struct GFDRelocationHeader
{
   static constexpr uint32_t Magic = 0x7D424C4B;
   static constexpr uint32_t HeaderSize = 10 * 4;
   uint32_t magic;
   uint32_t headerSize;
   uint32_t unk1;
   uint32_t dataSize;
   uint32_t dataOffset;
   uint32_t textSize;
   uint32_t textOffset;
   uint32_t patchBase;
   uint32_t patchCount;
   uint32_t patchOffset;
};

struct GFDBlockRelocations
{
   GFDRelocationHeader header;
   std::vector<uint32_t> patches;
};

struct GFDBlock
{
   GFDBlockHeader header;
   std::vector<uint8_t> data;
};

struct GFDFile
{
   std::vector<GFDVertexShader> vertexShaders;
   std::vector<GFDPixelShader> pixelShaders;
   std::vector<GFDGeometryShader> geometryShaders;
   std::vector<GFDTexture> textures;
};

static constexpr uint32_t GFDFileMajorVersion = 7u;
static constexpr uint32_t GFDFileMinorVersion = 1u;
static constexpr uint32_t GFDFileGpuVersion = 2u;
static constexpr uint32_t GFDBlockMajorVersion = 1u;
static constexpr uint32_t GFDPatchMask = 0xFFF00000u;
static constexpr uint32_t GFDPatchData = 0xD0600000u;
static constexpr uint32_t GFDPatchText = 0xCA700000u;

struct GFDReadException : public std::runtime_error
{
public:
   GFDReadException(const std::string &msg) :
      std::runtime_error(msg)
   {
   }
};

bool
readFile(GFDFile &file,
         const std::string &path);

bool
writeFile(const GFDFile &file,
          const std::string &path,
          bool align = false);

} // namespace gfd
