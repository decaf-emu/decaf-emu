#include "cafe_kernel_shareddata.h"
#include "decaf_config.h"
#include "kernel/kernel_filesystem.h"

#include <common/align.h>
#include <fstream>
#include <libcpu/be2_struct.h>

namespace cafe::kernel
{

static SharedArea sFontChinese;
static SharedArea sFontKorean;
static SharedArea sFontStandard;
static SharedArea sFontTaiwanese;

static uint32_t
loadSharedData(const char *filename,
               SharedArea &area,
               virt_addr addr)
{
   auto fs = ::kernel::getFileSystem();
   auto path = fs::Path { "/vol/storage_mlc01/sys/title/0005001B/10042400/content" }.join(filename);
   auto result = fs->openFile(path, fs::File::Read);

   if (!result) {
      area.size = 0u;
      area.address = virt_addr { 0 };
      return 0;
   }

   auto fh = result.value();
   area.size = static_cast<uint32_t>(fh->size());
   area.address = addr;
   fh->read(virt_cast<void *>(area.address).getRawPointer(), 1, area.size);
   fh->close();
   return area.size;
}

static uint32_t
loadResourcesFile(const char *filename,
                  SharedArea &area,
                  virt_addr addr)
{
   auto file = std::ifstream { decaf::config::system::resources_path + "/fonts/" + filename,
                               std::ifstream::in | std::ifstream::binary };

   if (!file.is_open()) {
      area.size = 0u;
      area.address = virt_addr { 0 };
      return 0;
   }

   file.seekg(0, std::ifstream::end);
   area.size = static_cast<uint32_t>(file.tellg());
   area.address = addr;

   file.seekg(0, std::ifstream::beg);
   file.read(virt_cast<char *>(area.address).getRawPointer(), area.size);
   file.close();

   return area.size;
}

void
loadShared()
{
   auto fs = ::kernel::getFileSystem();
   auto addr = virt_addr { 0xF8000000 };

   // FontChinese
   auto size = loadSharedData("CafeCn.ttf", sFontChinese, addr);
   if (!size) {
      size = loadResourcesFile("CafeCn.ttf", sFontChinese, addr);
   }

   addr = align_up(addr + size, 0x10);

   // FontKorean
   size = loadSharedData("CafeKr.ttf", sFontKorean, addr);
   if (!size) {
      size = loadResourcesFile("CafeKr.ttf", sFontKorean, addr);
   }

   addr = align_up(addr + size, 0x10);

   // FontStandard
   size = loadSharedData("CafeStd.ttf", sFontStandard, addr);
   if (!size) {
      size = loadResourcesFile("CafeStd.ttf", sFontStandard, addr);
   }

   addr = align_up(addr + size, 0x10);

   // FontTaiwanese
   size = loadSharedData("CafeTw.ttf", sFontTaiwanese, addr);
   if (!size) {
      size = loadResourcesFile("CafeTw.ttf", sFontTaiwanese, addr);
   }

   addr = align_up(addr + size, 0x10);
}

SharedArea
getSharedArea(SharedAreaId id)
{
   auto area = SharedArea { };

   switch (id) {
   case SharedAreaId::FontChinese:
      area = sFontChinese;
      break;
   case SharedAreaId::FontKorean:
      area = sFontKorean;
      break;
   case SharedAreaId::FontStandard:
      area = sFontStandard;
      break;
   case SharedAreaId::FontTaiwanese:
      area = sFontTaiwanese;
      break;
   default:
      area.address = virt_addr { 0 };
      area.size = 0u;
   }

   return area;
}

} // namespace cafe::kernel
