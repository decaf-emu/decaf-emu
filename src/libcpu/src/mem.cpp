#include "common/decaf_assert.h"
#include "common/log.h"
#include "common/platform_memory.h"
#include "mem.h"

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
   { "SystemData",   SystemBase,       SystemEnd,        0 },
   { "MEM2",         MEM2Base,         MEM2End,          0 },
   { "Apertures",    AperturesBase,    AperturesEnd,     0 },
   { "Foreground",   ForegroundBase,   ForegroundEnd,    0 },
   { "MEM1",         MEM1Base,         MEM1End,          0 },
   { "LockedCache",  LockedCacheBase,  LockedCacheEnd,   0 },
   { "SharedData",   SharedDataBase,   SharedDataEnd,    0 },
};

static size_t
gMemoryBase = 0;

static bool
tryMapMemory(size_t base);

// Attempt to map all memory regions with base
static bool
tryMapMemory(size_t base)
{
   if (!platform::reserveMemory(base, 0x100000000ull)) {
      return false;
   }

   for (auto &map : gMemoryMap) {
      auto size = map.end - map.start;
      map.address = base + map.start;

      if (!platform::commitMemory(map.address, map.end - map.start)) {
         map.address = 0;
         platform::freeMemory(base, 0x100000000ull);
         return false;
      }
   }

   return true;
}

// Initialise system memory, mapping all valid address space
void
initialise()
{
   // Find a good base address
   gMemoryBase = 0;

   for (auto n = 32; n < 64; ++n) {
      auto base = 1ull << n;

      if (tryMapMemory(base)) {
         gMemoryBase = base;
         break;
      }
   }

   decaf_assert(gMemoryBase, "Failed to find a valid memory base address");
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

// Cleanup memory, unmapping all views
void
shutdown()
{
   if (gMemoryBase) {
      platform::freeMemory(gMemoryBase, 0x100000000ull);
   }
}

} // namespace mem
