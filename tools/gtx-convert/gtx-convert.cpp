#include "gfd.h"
#include "untile.h"
#include "gpu/latte_tiling.h"
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
      uint32_t untiledPitch = 0;
      untileSurface(&tex.header->surface, tex.imageData.data(), untiledData, untiledPitch);

      // MAGIC DDS
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
