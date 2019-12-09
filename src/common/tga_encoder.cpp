#include "log.h"
#include "tga_encoder.h"
#include <fstream>

namespace tga
{

bool
writeFile(const std::string &filename,
          uint32_t bpp,
          uint32_t alphaBits,
          uint32_t width,
          uint32_t height,
          void *data)
{
   std::ofstream out { filename, std::ofstream::binary };
   if (!out.is_open()) {
      gLog->error("Could not open {} for writing", filename);
      return false;
   }

   FileHeader header;
   std::memset(&header, 0, sizeof(FileHeader));
   header.imageType = FileHeader::TrueColour;
   header.width = static_cast<uint16_t>(width);
   header.height = static_cast<uint16_t>(height);
   header.bpp = static_cast<uint8_t>(bpp);
   header.descriptor = alphaBits & 0b1111;

   out.write(reinterpret_cast<char *>(&header), sizeof(FileHeader));
   out.write(reinterpret_cast<char *>(data), width * height * (bpp / 8));
   return true;
}

} // namespace tga
