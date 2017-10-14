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
loadMlcFont(OSSharedDataType type,
            const char *filename)
{
   auto fs = kernel::getFileSystem();
   auto path = fs::Path { "/vol/storage_mlc01/sys/title/0005001B/10042400/content" }.join(filename);
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

static bool
loadResourcesFont(OSSharedDataType type,
                  const char *filename)
{
   auto file = std::ifstream { decaf::config::system::resources_path + "/fonts/" + filename,
                               std::ifstream::in | std::ifstream::binary };

   if (!file.is_open()) {
      return false;
   }

   file.seekg(0, std::ifstream::end);
   auto size = gsl::narrow_cast<uint32_t>(file.tellg());
   auto data = reinterpret_cast<uint8_t *>(sSharedHeap->alloc(size));
   file.seekg(0, std::ifstream::beg);
   file.read(reinterpret_cast<char*>(data), size);
   file.close();

   sSharedData[type].data = data;
   sSharedData[type].size = size;
   return true;
}

void
loadSharedData()
{
   // Initialise shared heap
   auto bounds = kernel::getVirtualRange(kernel::VirtualRegion::SharedData);
   auto ptr = virt_cast<void *>(bounds.start).getRawPointer();
   sSharedHeap = new TeenyHeap { ptr, bounds.size };

   // Try and load the fonts from mlc, if that fails try fall back
   // to resources/fonts.
   if (!loadMlcFont(OSSharedDataType::FontChinese, "CafeCn.ttf")) {
      loadResourcesFont(OSSharedDataType::FontChinese, "CafeCn.ttf");
   }

   if (!loadMlcFont(OSSharedDataType::FontKorean, "CafeKr.ttf")) {
      loadResourcesFont(OSSharedDataType::FontKorean, "CafeKr.ttf");
   }

   if (!loadMlcFont(OSSharedDataType::FontStandard, "CafeStd.ttf")) {
      loadResourcesFont(OSSharedDataType::FontStandard, "CafeStd.ttf");
   }

   if (!loadMlcFont(OSSharedDataType::FontTaiwanese, "CafeTw.ttf")) {
      loadResourcesFont(OSSharedDataType::FontTaiwanese, "CafeTw.ttf");
   }
}

} // namespace internal

void
Module::registerSharedFunctions()
{
   RegisterKernelFunction(OSGetSharedData);
}

} // namespace coreinit
