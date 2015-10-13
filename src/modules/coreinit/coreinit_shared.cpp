#include <fstream>
#include <gsl.h>
#include "coreinit.h"
#include "coreinit_memheap.h"
#include "coreinit_shared.h"

struct FontData
{
   p32<uint8_t> data = nullptr;
   uint32_t size = 0;
};

FontData gFonts[4];

BOOL
OSGetSharedData(OSSharedDataType::Type type, uint32_t, be_ptr<uint8_t> *addr, be_val<uint32_t> *size)
{
   switch (type) {
   case OSSharedDataType::FontChinese:
      *addr = gFonts[0].data;
      *size = gFonts[0].size;
      return TRUE;
   case OSSharedDataType::FontKorean:
      *addr = gFonts[1].data;
      *size = gFonts[1].size;
      return TRUE;
   case OSSharedDataType::FontStandard:
      *addr = gFonts[2].data;
      *size = gFonts[2].size;
      return TRUE;
   case OSSharedDataType::FontTaiwanese:
      *addr = gFonts[3].data;
      *size = gFonts[3].size;
      return TRUE;
   default:
      *addr = nullptr;
      *size = 0;
      return FALSE;
   }
}

void
readFont(FontData &dst, const char *src)
{
   std::ifstream file { src, std::ifstream::in | std::ifstream::binary };
   file.seekg(0, std::ifstream::end);
   dst.size = gsl::narrow_cast<uint32_t>(file.tellg());
   dst.data = reinterpret_cast<uint8_t*>(OSAllocFromSystem(dst.size));
   file.seekg(0, std::ifstream::beg);
   file.read(reinterpret_cast<char*>(dst.data.get()), dst.size);
}

void
CoreInit::initialiseShared()
{
   readFont(gFonts[0], "resources/fonts/SourceSansPro-Regular.ttf");
   readFont(gFonts[1], "resources/fonts/SourceSansPro-Regular.ttf");
   readFont(gFonts[2], "resources/fonts/SourceSansPro-Regular.ttf");
   readFont(gFonts[3], "resources/fonts/SourceSansPro-Regular.ttf");
}

void
CoreInit::registerSharedFunctions()
{
   RegisterKernelFunction(OSGetSharedData);
}


