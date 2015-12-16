#include <fstream>
#include <gsl.h>
#include "coreinit.h"
#include "coreinit_shared.h"
#include "mem/mem.h"
#include "utils/virtual_ptr.h"
#include "utils/teenyheap.h"

struct FontData
{
   virtual_ptr<uint8_t> data = nullptr;
   uint32_t size = 0;
};

static FontData
gFonts[4];

static TeenyHeap *
gSharedHeap = nullptr;

BOOL
OSGetSharedData(OSSharedDataType type, uint32_t, be_ptr<uint8_t> *addr, be_val<uint32_t> *size)
{
   switch (type) {
   case OSSharedDataType::FontChinese:
      *addr = gFonts[0].data;
      *size = gFonts[0].size;
      break;
   case OSSharedDataType::FontKorean:
      *addr = gFonts[1].data;
      *size = gFonts[1].size;
      break;
   case OSSharedDataType::FontStandard:
      *addr = gFonts[2].data;
      *size = gFonts[2].size;
      break;
   case OSSharedDataType::FontTaiwanese:
      *addr = gFonts[3].data;
      *size = gFonts[3].size;
      break;
   default:
      *addr = nullptr;
      *size = 0;
   }

   return (*size > 0) ? TRUE : FALSE;
}

void
readFont(FontData &dst, const char *src)
{
   auto file = std::ifstream { src, std::ifstream::in | std::ifstream::binary };

   if (file.is_open()) {
      file.seekg(0, std::ifstream::end);
      dst.size = gsl::narrow_cast<uint32_t>(file.tellg());
      dst.data = reinterpret_cast<uint8_t *>(gSharedHeap->alloc(dst.size));
      file.seekg(0, std::ifstream::beg);
      file.read(reinterpret_cast<char*>(dst.data.get()), dst.size);
   } else {
      dst.size = 0;
      dst.data = nullptr;
   }
}

void
CoreInit::initialiseShared()
{
   gSharedHeap = new TeenyHeap(memory_translate(mem::SharedDataBase), mem::SharedDataSize);
   readFont(gFonts[0], "resources/fonts/SourceSansPro-Regular.ttf");
   gFonts[1] = gFonts[0];
   gFonts[2] = gFonts[0];
   gFonts[3] = gFonts[0];
}

void
CoreInit::registerSharedFunctions()
{
   RegisterKernelFunction(OSGetSharedData);
}


