#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "mem.h"
#include "utils/log.h"

namespace mem
{

union PageEntry
{
   struct
   {
      uint32_t base : 20;           // First page in region
      uint32_t count : 20;          // Number of pages in region (only valid in base page)
      uint32_t allocated : 1;       // Is page allocated?
      uint32_t : 22;
   };

   uint64_t value = 0;
};

enum class MemoryType
{
   SystemData,
   Application,
   Foreground,
   MEM1
};

struct MemoryView
{
   MemoryView() :
      address(0)
   {
   }

   MemoryView(MemoryType type, uint32_t start, uint32_t end, uint32_t pageSize) :
      type(type), start(start), end(end), pageSize(pageSize), address(0)
   {
   }

   MemoryType type;
   uint32_t start;
   uint32_t end;
   uint8_t *address;
   uint32_t pageSize;
   std::vector<PageEntry> pageTable;
};


uint8_t *gBase = nullptr;
void *sFile = NULL;
std::vector<MemoryView> sViews;

void unmapViews()
{
   for (auto &view : sViews) {
      if (view.address) {
         UnmapViewOfFile(view.address);
         view.address = nullptr;
      }
   }
}

bool tryMapViews(uint8_t *base)
{
   for (auto &view : sViews) {
      auto lo = static_cast<DWORD>(view.start);
      auto hi = static_cast<DWORD>(0);
      auto size = static_cast<SIZE_T>(view.end - view.start);
      auto target = base + view.start;

      // Attempt to map
      view.address = reinterpret_cast<uint8_t*>(MapViewOfFileEx(sFile, FILE_MAP_WRITE, hi, lo, size, target));

      if (!view.address) {
         unmapViews();
         return false;
      }
   }

   return true;
}

void initialise()
{
   // Setup memory views
   sViews = {
      { MemoryType::SystemData,        0x01000000, 0x02000000,   4 * 1024 },
      { MemoryType::Application,       0x02000000, 0x42000000,   4 * 1024 },
      { MemoryType::Foreground,        0xe0000000, 0xe4000000,   4 * 1024 },
      { MemoryType::MEM1,              0xf4000000, 0xf6000000,   4 * 1024 },
   };

   // Create file map
   sFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE | SEC_RESERVE, 0x1, 0x00000000, NULL);

   // Find a good base address
   for (auto n = 32; n < 64; ++n) {
      auto base = reinterpret_cast<uint8_t*>(1ull << n);

      if (tryMapViews(base)) {
         gBase = base;
         break;
      }
   }

   if (!gBase) {
      gLog->error("Could not find a valid base address for memory!");
      throw;
   }

   // Setup page table
   for (auto &view : sViews) {
      auto size = view.end - view.start;
      auto pages = size / view.pageSize;
      assert(size % view.pageSize == 0);
      view.pageTable.resize(pages);
      memset(view.pageTable.data(), 0, pages * sizeof(PageEntry));
   }
}

MemoryView * getView(MemoryType type)
{
   for (auto &view : sViews) {
      if (view.type == type) {
         return &view;
      }
   }

   return nullptr;
}

MemoryView * getView(uint32_t address)
{
   for (auto &view : sViews) {
      if (address >= view.start && address < view.end) {
         return &view;
      }
   }

   return nullptr;
}

bool valid(uint32_t address)
{
   auto view = getView(address);

   if (!view) {
      return false;
   }

   auto page = (address - view->start) / view->pageSize;
   return view->pageTable[page].allocated;
}

bool protect(uint32_t address, size_t size)
{
   auto result = VirtualAlloc(gBase + address, size, MEM_RESERVE, PAGE_NOACCESS);
   return result != NULL;
}

bool alloc(uint32_t address, size_t size)
{
   auto view = getView(address);

   if (!view) {
      gLog->error("Could not find valid memory view for allocating {} bytes at {}", size, address);
      return false;
   }

   // Address is in view!
   auto start = address - view->start;
   auto end = address + size - view->start - 1;
   auto startPage = start / view->pageSize;
   auto endPage = end / view->pageSize;
   auto pageCount = endPage - startPage + 1;

   // Ensure all pages in region are free
   for (auto i = startPage; i <= endPage; ++i) {
      auto &page = view->pageTable[i];

      if (page.allocated) {
         gLog->debug("Tried to reallocate an existing page");
         return false;
      }
   }

   // Allocate from host memory
   auto result = VirtualAlloc(view->address + startPage * view->pageSize, pageCount * view->pageSize, MEM_COMMIT, PAGE_READWRITE);

   if (!result) {
      gLog->error("Failed to commit host memory");
      return false;
   }

   // Mark pages as allocated
   for (auto i = startPage; i <= endPage; ++i) {
      auto &page = view->pageTable[i];
      page.allocated = true;
      page.base = startPage;
      page.count = pageCount;
   }

   return true;
}

uint32_t alloc(MemoryType type, size_t size)
{
   auto view = getView(type);
   auto n = (size - 1) / view->pageSize;
   auto startPage = 0u;
   auto endPage = 0u;

   // Find n free contiguous pages
   for (auto i = 0u; i < view->pageTable.size(); ++i) {
      if (!view->pageTable[i].allocated) {
         startPage = i;

         for (auto j = i; j <= i + n && j < view->pageTable.size(); ++j) {
            if (view->pageTable[j].allocated) {
               break;
            }

            endPage = j;
         }

         if (endPage - startPage == n) {
            break;
         }
      }
   }

   auto address = view->start + startPage * view->pageSize;

   if (alloc(address, size)) {
      return address;
   } else {
      return 0;
   }
}

bool free(uint32_t address)
{
   MemoryView *view = getView(address);

   if (!view) {
      gLog->error("Could not find valid memory view for free at {}", address);
      return false;
   }

   // Check the page is valid
   auto start = address - view->start;
   auto startPage = start / view->pageSize;
   auto &page = view->pageTable[startPage];

   if (page.base != startPage) {
      gLog->error("Could not free memory as it is not a base page");
      return false;
   }

   // Decommit from host memory (TODO: Is this correct?)
   auto result = VirtualFree(view->address + startPage * view->pageSize, page.count * view->pageSize, MEM_DECOMMIT);

   if (!result) {
      gLog->error("Failed to decommit from host memory");
      return false;
   }

   // Set pages as unallocated
   auto endPage = startPage + page.count;

   for (auto i = startPage; i <= endPage; ++i) {
      view->pageTable[i].value = 0;
   }

   return true;
}

void shutdown()
{
   if (sFile) {
      unmapViews();
      CloseHandle(sFile);
   }
}

} // namespace mem
