#include <array>
#include <fstream>
#include <gsl.h>
#include "coreinit.h"
#include "coreinit_shared.h"
#include "libcpu/mem.h"
#include "virtual_ptr.h"
#include "common/teenyheap.h"

namespace coreinit
{

struct FontData
{
   uint8_t *data = nullptr;
   uint32_t size = 0;
};

static std::array<FontData, 4>
sFonts;

// TODO: Delete me on game unload
static TeenyHeap *
sSharedHeap = nullptr;

BOOL
OSGetSharedData(OSSharedDataType type, uint32_t, be_ptr<uint8_t> *addr, be_val<uint32_t> *size)
{
   switch (type) {
   case OSSharedDataType::FontChinese:
      *addr = sFonts[0].data;
      *size = sFonts[0].size;
      break;
   case OSSharedDataType::FontKorean:
      *addr = sFonts[1].data;
      *size = sFonts[1].size;
      break;
   case OSSharedDataType::FontStandard:
      *addr = sFonts[2].data;
      *size = sFonts[2].size;
      break;
   case OSSharedDataType::FontTaiwanese:
      *addr = sFonts[3].data;
      *size = sFonts[3].size;
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
      dst.data = reinterpret_cast<uint8_t *>(sSharedHeap->alloc(dst.size));
      file.seekg(0, std::ifstream::beg);
      file.read(reinterpret_cast<char*>(dst.data), dst.size);
   } else {
      dst.size = 0;
      dst.data = nullptr;
   }
}

void
Module::initialiseShared()
{
   sSharedHeap = new TeenyHeap(mem::translate(mem::SharedDataBase), mem::SharedDataSize);
   readFont(sFonts[0], "resources/fonts/SourceSansPro-Regular.ttf");
   sFonts[1] = sFonts[0];
   sFonts[2] = sFonts[0];
   sFonts[3] = sFonts[0];
}

void
Module::registerSharedFunctions()
{
   RegisterKernelFunction(OSGetSharedData);
}

} // namespace coreinit
