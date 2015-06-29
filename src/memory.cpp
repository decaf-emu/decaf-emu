#include "memory.h"
#include "log.h"
#include <vector>
#include <Windows.h>

Memory gMemory;

Memory::~Memory()
{
   if (mFile) {
      unmapViews();
      CloseHandle(mFile);
   }
}

bool
Memory::initialise()
{
   // Setup memory views
   mViews = {
      { MemoryType::SystemData,        0x01000000, 0x02000000,   4 * 1024 },
      { MemoryType::Application,       0x02000000, 0x42000000,   4 * 1024 },
      { MemoryType::Foreground,        0xe0000000, 0xe4000000,   4 * 1024 },
      { MemoryType::MEM1,              0xf4000000, 0xf6000000,   4 * 1024 },
   };

   // Create file map
   mFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE | SEC_RESERVE, 0x1, 0x00000000, NULL);

   // Find a good base address
   for (auto n = 32; n < 64; ++n) {
      auto base = reinterpret_cast<uint8_t*>(1ull << n);

      if (tryMapViews(base)) {
         mBase = base;
         break;
      }
   }

   if (!mBase) {
      xError() << "Could not find a valid base address for memory!";
      return false;
   }

   // Setup page table
   for (auto &view : mViews) {
      auto size = view.end - view.start;
      auto pages = size / view.pageSize;
      assert(size % view.pageSize == 0);
      view.pageTable.resize(pages);
      memset(view.pageTable.data(), 0, pages * sizeof(PageEntry));
   }
   
   return true;
}

MemoryView *
Memory::getView(MemoryType type)
{
   for (auto &view : mViews) {
      if (view.type == type) {
         return &view;
      }
   }

   return nullptr;
}

MemoryView *
Memory::getView(uint32_t address)
{
   for (auto &view : mViews) {
      if (address >= view.start && address < view.end) {
         return &view;
      }
   }

   return nullptr;
}

bool
Memory::valid(uint32_t address)
{
   auto view = getView(address);

   if (!view) {
      return nullptr;
   }

   auto page = (address - view->start) / view->pageSize;
   return view->pageTable[page].allocated;
}

uint32_t
Memory::alloc(MemoryType type, size_t size)
{
   auto view = getView(type);
   auto n = size / view->pageSize;
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

bool
Memory::alloc(uint32_t address, size_t size)
{
   auto view = getView(address);

   if (!view) {
      xError() << "Could not find valid memory view for allocation at " << Log::hex(address) << " [" << Log::hex(size) << "]";
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
         xError() << "Tried to reallocate an existing page";
         return false;
      }
   }

   // Allocate from host memory
   auto result = VirtualAlloc(view->address + startPage * view->pageSize, pageCount * view->pageSize, MEM_COMMIT, PAGE_READWRITE);

   if (!result) {
      xError() << "Failed to commit host memory";
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

bool
Memory::free(uint32_t address)
{
   MemoryView *view = getView(address);

   if (!view) {
      xError() << "Could not find valid memory view for free at " << Log::hex(address);
      return false;
   }

   // Check the page is valid
   auto start = address - view->start;
   auto startPage = start / view->pageSize;
   auto &page = view->pageTable[startPage];

   if (page.base != startPage) {
      xError() << "Could not free memory as it is not a base page";
      return false;
   }

   // Decommit from host memory (TODO: Is this correct?)
   auto result = VirtualFree(view->address + startPage * view->pageSize, page.count * view->pageSize, MEM_DECOMMIT);

   if (!result) {
      xError() << "Failed to decommit from host memory";
      return false;
   }

   // Set pages as unallocated
   auto endPage = startPage + page.count;

   for (auto i = startPage; i <= endPage; ++i) {
      view->pageTable[i].value = 0;
   }

   return true;
}

bool
Memory::tryMapViews(uint8_t *base)
{
   for (auto &view : mViews) {
      auto lo = static_cast<DWORD>(view.start);
      auto hi = static_cast<DWORD>(0);
      auto size = static_cast<SIZE_T>(view.end - view.start);
      auto target = base + view.start;

      // Attempt to map
      view.address = reinterpret_cast<uint8_t*>(MapViewOfFileEx(mFile, FILE_MAP_WRITE, hi, lo, size, target));

      if (!view.address) {
         unmapViews();
         return false;
      }
   }

   return true;
}

void
Memory::unmapViews()
{
   for (auto &view : mViews) {
      if (view.address) {
         UnmapViewOfFile(view.address);
         view.address = nullptr;
      }
   }
}
