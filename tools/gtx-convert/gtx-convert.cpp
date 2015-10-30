#include "gpu/gfd.h"
#include "gpu/latte_untile.h"
#include "modules/gx2/gx2_texture.h"
#include "modules/gx2/gx2_enum_string.h"
#include "utils/binaryfile.h"
#include <spdlog/spdlog.h>
#include <cassert>
#include <docopt.h>

static const char USAGE[] =
R"(GTX Texture Converter

Usage:
   gtx-convert info <file in>
   gtx-convert convert <file in> <file out>

   Options:
   -h --help     Show this screen.
   --version     Show version.
)";

bool printInfo(const std::string &filename)
{
   gfd::File file;

   if (!gfd::readFile(filename, file)) {
      return false;
   }

   for (auto &block : file.blocks) {
      switch (block.header.type) {
      case gfd::BlockType::TextureHeader:
         {
            auto tex = reinterpret_cast<GX2Texture *>(block.data.data());
            assert(block.data.size() >= sizeof(GX2Texture));

            std::cout
               << "TextureHeader" << std::endl
               << "  index      = " << block.header.index << std::endl
               << "  dim        = " << GX2EnumAsString(tex->surface.dim) << std::endl
               << "  width      = " << tex->surface.width << std::endl
               << "  height     = " << tex->surface.height << std::endl
               << "  depth      = " << tex->surface.depth << std::endl
               << "  mipLevels  = " << tex->surface.mipLevels << std::endl
               << "  format     = " << GX2EnumAsString(tex->surface.format) << std::endl
               << "  aa         = " << GX2EnumAsString(tex->surface.aa) << std::endl
               << "  use        = " << GX2EnumAsString(tex->surface.use) << std::endl
               << "  imageSize  = " << tex->surface.imageSize << std::endl
               << "  mipmapSize = " << tex->surface.mipmapSize << std::endl
               << "  tileMode   = " << GX2EnumAsString(tex->surface.tileMode) << std::endl
               << "  swizzle    = " << tex->surface.swizzle << std::endl
               << "  alignment  = " << tex->surface.alignment << std::endl
               << "  pitch      = " << tex->surface.pitch << std::endl;
         }
         break;
      }
   }

   return true;
}

struct Texture
{
   GX2Texture *header;
   gsl::array_view<uint8_t> imageData;
   gsl::array_view<uint8_t> mipmapData;
};

#pragma pack(1)

struct DdsPixelFormat
{
   uint32_t	dwSize;
   uint32_t	dwFlags;
   uint32_t	dwFourCC;
   uint32_t	dwRGBBitCount;
   uint32_t	dwRBitMask;
   uint32_t	dwGBitMask;
   uint32_t	dwBBitMask;
   uint32_t	dwABitMask;
};

struct DdsHeader
{
   uint32_t	dwSize;
   uint32_t	dwFlags;
   uint32_t	dwHeight;
   uint32_t	dwWidth;
   uint32_t	dwPitchOrLinearSize;
   uint32_t	dwDepth;
   uint32_t	dwMipMapCount;
   uint32_t	dwReserved1[11];
   DdsPixelFormat	ddspf;
   uint32_t	dwCaps;
   uint32_t	dwCaps2;
   uint32_t	dwCaps3;
   uint32_t	dwCaps4;
   uint32_t	dwReserved2;
};
static_assert(sizeof(DdsHeader) == 124, "dds header should be 124 bytes long");
#pragma pack()
bool
convert(const std::string &filenameIn, const std::string &filenameOut)
{
   gfd::File file;
   std::vector<Texture> textures;

   if (!gfd::readFile(filenameIn, file)) {
      return false;
   }

   for (auto &block : file.blocks) {
      switch (block.header.type) {
      case gfd::BlockType::TextureHeader: {
         auto tex = reinterpret_cast<GX2Texture *>(block.data.data());
         assert(block.header.index == textures.size());
         textures.push_back({ tex, {}, {} });
         } break;
      case gfd::BlockType::TextureImage:
         textures[block.header.index].imageData = block.data;
         break;
      case gfd::BlockType::TextureMipmap:
         textures[block.header.index].mipmapData = block.data;
         break;
      }
   }

   for (auto &tex : textures) {
      std::vector<uint8_t> untiledData;
      size_t untiledPitch = 0;
      latte::untileSurface(&tex.header->surface, tex.imageData.data(), untiledData, untiledPitch);

      // MAGIC DDS
      DdsHeader ddsHeader;
      memset(&ddsHeader, 0, sizeof(ddsHeader));
      ddsHeader.dwSize = sizeof(ddsHeader);
      ddsHeader.dwFlags = 0x1 | 0x2 | 0x4 | 0x1000 | 0x80000;
      ddsHeader.dwHeight = tex.header->surface.height;
      ddsHeader.dwWidth = tex.header->surface.width;
      ddsHeader.dwPitchOrLinearSize = (uint32_t)tex.imageData.size();
      ddsHeader.ddspf.dwSize = sizeof(ddsHeader.ddspf);
      ddsHeader.ddspf.dwFlags = 0x1 | 0x4;
      ddsHeader.dwCaps = 0x1000;

      switch (tex.header->surface.format) {
      case GX2SurfaceFormat::UNORM_BC1:
         ddsHeader.ddspf.dwFourCC = '1TXD';
         break;
      case GX2SurfaceFormat::UNORM_BC2:
         ddsHeader.ddspf.dwFourCC = '3TXD';
         break;
      case GX2SurfaceFormat::UNORM_BC3:
         ddsHeader.ddspf.dwFourCC = '5TXD';
         break;
      case GX2SurfaceFormat::UNORM_BC4:
         ddsHeader.ddspf.dwFourCC = '1ITA';
         break;
      case GX2SurfaceFormat::UNORM_BC5:
         ddsHeader.ddspf.dwFourCC = '2ITA';
         break;
      default:
         std::cout << "Unsupported surface format for DDS output " << GX2EnumAsString(tex.header->surface.format) << std::endl;
         return false;
      }

      auto binaryDds = std::ofstream { filenameOut, std::ofstream::out | std::ofstream::binary };
      binaryDds.write("DDS ", 4);
      binaryDds.write(reinterpret_cast<char*>(&ddsHeader), sizeof(ddsHeader));
      binaryDds.write(reinterpret_cast<char*>(&untiledData[0]), untiledData.size());

      // Only output 1 file for now
      break;
   }

   return true;
}

int main(int argc, char **argv)
{
   auto args = docopt::docopt(USAGE, { argv + 1, argv + argc }, true, "gtx-convert 0.1");

   if (args["info"].asBool()) {
      auto in = args["<file in>"].asString();
      return printInfo(in) ? 0 : -1;
   } else if (args["convert"].asBool()) {
      auto in = args["<file in>"].asString();
      auto out = args["<file out>"].asString();
      return convert(in, out) ? 0 : -1;
   }

   return 0;
}

// Stubs to appease the linker gods
void *
memory_translate(ppcaddr_t address)
{
   return nullptr;
}

ppcaddr_t
memory_untranslate(const void *pointer)
{
   return 0;
}
