#include "mem.h"
#include "platform/platform_memorymap.h"
#include "utils/log.h"

namespace mem
{

struct Mapping
{
   std::string name;
   size_t start;
   size_t end;
   size_t address;
};

static std::vector<Mapping>
gMemoryMap =
{
   { "SystemData",   0x01000000, 0x02000000, 0 },
   { "Application",  0x02000000, 0x42000000, 0 },
   { "Foreground",   0xE0000000, 0xE4000000, 0 },
   { "MEM1",         0xF4000000, 0xF6000000, 0 },
   { "LockedCache",  0xF8000000, 0xF800C000, 0 },
};

static size_t
gMemoryBase = 0;

static platform::MemoryMappedFile *
gMapHandle = nullptr;

static void
unmapMemory();

static bool
tryMapMemory(size_t base);

// Attempt to map all memory regions with base
static bool
tryMapMemory(size_t base)
{
   for (auto &map : gMemoryMap) {
      auto size = map.end - map.start;
      map.address = base + map.start;

      if (!platform::mapMemory(gMapHandle, map.start, map.address, size)) {
         map.address = 0;
         unmapMemory();
         return false;
      }
   }

   return true;
}

// Unmap all memory regions
static void
unmapMemory()
{
   for (auto &map : gMemoryMap) {
      if (map.address) {
         auto size = map.end - map.start;
         platform::unmapMemory(gMapHandle, map.address, size);
         map.address = 0;
      }
   }
}

// Initialise system memory, mapping all valid address space
void
initialise()
{
   // Create file map
   gMapHandle = platform::createMemoryMappedFile(0x100000000ull);

   // Find a good base address
   gMemoryBase = 0;

   for (auto n = 32; n < 64; ++n) {
      auto base = 1ull << n;

      if (tryMapMemory(base)) {
         gMemoryBase = base;
         break;
      }
   }

   if (!gMemoryBase) {
      gLog->critical("Failed to find a valid memory base address");
      throw std::runtime_error("Failed to find a valid memory base address");
   }

   // Allocate mapped memory
   for (auto &map : gMemoryMap) {
      auto result = platform::commitMemory(gMapHandle, map.address, map.end - map.start);

      if (!result) {
         gLog->critical("Failed to allocate mapped memory");
         throw std::runtime_error("Failed to allocate mapped memory");
      }
   }
}

// Return the base address of mapped memory
size_t
base()
{
   return gMemoryBase;
}

// Returns true if address is in a mapped Wii U memory address range
bool
valid(ppcaddr_t address)
{
   for (auto &map : gMemoryMap) {
      if (address >= map.start && address < map.end) {
         return true;
      }
   }

   return false;
}

// Effectively sets a memory breakpoint
bool
protect(ppcaddr_t address, size_t size)
{
   return platform::protectMemory(gMemoryBase + address, size);
}

// Cleanup memory, unmapping all views
void
shutdown()
{
   if (gMapHandle) {
      unmapMemory();
      platform::destroyMemoryMappedFile(gMapHandle);
      gMapHandle = nullptr;
   }
}

} // namespace mem
