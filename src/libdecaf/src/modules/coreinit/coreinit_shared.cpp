#include "coreinit.h"
#include "coreinit_shared.h"
#include "kernel/kernel_filesystem.h"
#include "kernel/kernel_memory.h"

#include <array>
#include <common/be_ptr.h>
#include <common/be_val.h>
#include <common/structsize.h>
#include <common/teenyheap.h>
#include <fstream>
#include <gsl.h>
#include <libcpu/mem.h>


namespace coreinit
{

struct SharedData
{
   uint8_t *data = nullptr;
   uint32_t size = 0;
};

static std::array<SharedData, OSSharedDataType::Max>
sSharedData;

// TODO: Delete me on game unload
static TeenyHeap *
sSharedHeap = nullptr;

BOOL
OSGetSharedData(OSSharedDataType type,
                uint32_t unk_r4,
                be_ptr<uint8_t> *addr,
                be_val<uint32_t> *size)
{
   if (type >= sSharedData.size()) {
      *addr = nullptr;
      *size = 0;
      return FALSE;
   }

   *addr = sSharedData[type].data;
   *size = sSharedData[type].size;
   return TRUE;
}

namespace internal
{

static bool
loadFont(OSSharedDataType type, const char *filename)
{
   auto fs = kernel::getFileSystem();
   auto path = fs::Path("/vol/storage_mlc01/sys/title/0005001B/10042400/content").join(filename);
   auto result = fs->openFile(path, fs::File::Read);
   auto &font = sSharedData[type];

   if (!result) {
      font.size = 0;
      font.data = nullptr;
      return false;
   }

   auto fh = result.value();
   font.size = static_cast<uint32_t>(fh->size());
   font.data = reinterpret_cast<uint8_t *>(sSharedHeap->alloc(font.size));
   fh->read(font.data, 1, font.size);
   fh->close();
   return true;
}

void
loadSharedData()
{
   // Initialise shared heap
   auto bounds = kernel::getSharedDataBounds();
   auto ptr = cpu::VirtualPointer<void> { bounds.start }.getRawPointer();
   sSharedHeap = new TeenyHeap { ptr, bounds.size };

   // Try and load the fonts from mlc.
   auto allFound = true;
   allFound = allFound && loadFont(OSSharedDataType::FontChinese, "CafeCn.ttf");
   allFound = allFound && loadFont(OSSharedDataType::FontKorean, "CafeKr.ttf");
   allFound = allFound && loadFont(OSSharedDataType::FontStandard, "CafeStd.ttf");
   allFound = allFound && loadFont(OSSharedDataType::FontTaiwanese, "CafeTw.ttf");

   if (!allFound) {
      // As a backup, try load Source Sans Pro from resources folder.
      auto file = std::ifstream { "resources/fonts/SourceSansPro-Regular.ttf",
                                  std::ifstream::in | std::ifstream::binary };
      auto sourceSansProSize = uint32_t { 0 };
      uint8_t *sourceSansProData = nullptr;

      if (file.is_open()) {
         file.seekg(0, std::ifstream::end);
         sourceSansProSize = gsl::narrow_cast<uint32_t>(file.tellg());
         sourceSansProData = reinterpret_cast<uint8_t *>(sSharedHeap->alloc(sourceSansProSize));
         file.seekg(0, std::ifstream::beg);
         file.read(reinterpret_cast<char*>(sourceSansProData), sourceSansProSize);
      }

      if (!sSharedData[OSSharedDataType::FontChinese].size) {
         sSharedData[OSSharedDataType::FontChinese].data = sourceSansProData;
         sSharedData[OSSharedDataType::FontChinese].size = sourceSansProSize;
      }

      if (!sSharedData[OSSharedDataType::FontKorean].size) {
         sSharedData[OSSharedDataType::FontKorean].data = sourceSansProData;
         sSharedData[OSSharedDataType::FontKorean].size = sourceSansProSize;
      }

      if (!sSharedData[OSSharedDataType::FontStandard].size) {
         sSharedData[OSSharedDataType::FontStandard].data = sourceSansProData;
         sSharedData[OSSharedDataType::FontStandard].size = sourceSansProSize;
      }

      if (!sSharedData[OSSharedDataType::FontTaiwanese].size) {
         sSharedData[OSSharedDataType::FontTaiwanese].data = sourceSansProData;
         sSharedData[OSSharedDataType::FontTaiwanese].size = sourceSansProSize;
      }
   }
}

} // namespace internal

void
Module::registerSharedFunctions()
{
   RegisterKernelFunction(OSGetSharedData);
}

} // namespace coreinit
