#pragma once
#include <cstdint>
#include <string>

namespace tga
{

#pragma pack(push, 1)

struct FileHeader
{
   enum ImageType : uint8_t
   {
      None = 0,
      ColourMapped = 1,
      TrueColour = 2,
      Grayscale = 3,
      ColourMappedRLE = 9,
      TrueColourRLE = 10,
      GrayscaleRLE = 11,
   };

   uint8_t id;
   uint8_t colorMapType;
   uint8_t imageType;

   struct
   {
      uint16_t firstEntry;
      uint16_t numEntries;
      uint8_t entrySize;
   } colorMap;

   uint16_t x;
   uint16_t y;
   uint16_t width;
   uint16_t height;
   uint8_t bpp;
   uint8_t descriptor;
};

#pragma pack(pop)

bool
writeFile(const std::string &filename,
          uint32_t bpp,
          uint32_t alphaBits,
          uint32_t width,
          uint32_t height,
          void *data);

} // namespace tga
