#include <common/decaf_assert.h>
#include <common/log.h>
#include <common/platform_memory.h>
#include "mem.h"

namespace mem
{

enum MapFlags
{
   None           = 0,
   ManualCommit   = 0 << 0,
   AutoCommit     = 1 << 0,
};

struct Mapping
{
   std::string name;
   size_t start;
   size_t end;
   size_t address;
   MapFlags flags;
};

static std::vector<Mapping>
gMemoryMap =
{
   { "SystemData",   SystemBase,       SystemEnd,        0, MapFlags::AutoCommit },
   { "MEM2",         MEM2Base,         MEM2End,          0, MapFlags::AutoCommit },
   { "OverlayArena", OverlayArenaBase, OverlayArenaEnd,  0, MapFlags::ManualCommit },
   { "Apertures",    AperturesBase,    AperturesEnd,     0, MapFlags::ManualCommit },
   { "Foreground",   ForegroundBase,   ForegroundEnd,    0, MapFlags::AutoCommit },
   { "MEM1",         MEM1Base,         MEM1End,          0, MapFlags::AutoCommit },
   { "LockedCache",  LockedCacheBase,  LockedCacheEnd,   0, MapFlags::AutoCommit },
   { "SharedData",   SharedDataBase,   SharedDataEnd,    0, MapFlags::AutoCommit },
   { "Loader",       LoaderBase,       LoaderEnd,        0, MapFlags::ManualCommit },
};

static size_t
gMemoryBase = 0;

static bool
tryMapMemory(size_t base);

static Mapping *
findMapping(ppcaddr_t base, size_t size)
{
   auto end = base + size;

   for (auto &map : gMemoryMap) {
      if (map.start == base && map.end == end) {
         return &map;
      }
   }

   return nullptr;
}

/**
 * Attempt to map all memory regions with specified base
 */
static bool
tryMapMemory(size_t base)
{
   if (!platform::reserveMemory(base, 0x100000000ull)) {
      return false;
   }

   for (auto &map : gMemoryMap) {
      auto size = map.end - map.start;

      if (map.flags & MapFlags::AutoCommit) {
         map.address = base + map.start;

         if (!platform::commitMemory(map.address, map.end - map.start)) {
            map.address = 0;
            platform::freeMemory(base, 0x100000000ull);
            return false;
         }
      }
   }

   return true;
}

/**
 * Initialise memory, mapping all valid address space
 */
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

/**
 * Return the base address of mapped memory
 */
size_t
base()
{
   return gMemoryBase;
}

/**
 * Returns true if address is in a mapped Wii U memory address range
 */
bool
valid(ppcaddr_t address)
{
   for (auto &map : gMemoryMap) {
      if (!map.address) {
         // Uncommitted memory is not valid
         continue;
      }

      if (address >= map.start && address < map.end) {
         return true;
      }
   }

   return false;
}

/**
 * Cleanup memory, unmapping all views
 */
void
shutdown()
{
   if (gMemoryBase) {
      platform::freeMemory(gMemoryBase, 0x100000000ull);
   }

   for (auto &map : gMemoryMap) {
      map.address = 0;
   }
}

/**
 * Manually commit a memory region, must match a known mapping in gMemoryMap.
 */
bool
commit(ppcaddr_t address,
       ppcaddr_t size)
{
   auto mapping = findMapping(address, size);

   if (!mapping) {
      decaf_abort("Invalid mem::commit attempt, must exactly match a know mapping");
      return false;
   }

   if (mapping->flags & MapFlags::AutoCommit) {
      decaf_abort("Attempted to manually commit an autocommit mapping");
      return false;
   }

   if (mapping->address) {
      // Already committed
      return true;
   }

   mapping->address = gMemoryBase + address;

   if (!platform::commitMemory(mapping->address, size)) {
      // Failed to commit
      mapping->address = 0;
      return false;
   }

   return true;
}

/**
 * Manually uncommit a memory region, must match a known mapping in gMemoryMap.
 */
bool
uncommit(ppcaddr_t address,
         ppcaddr_t size)
{
   auto mapping = findMapping(address, size);

   if (!mapping) {
      decaf_abort("Invalid mem::commit attempt, must exactly match a know mapping");
      return false;
   }

   if (mapping->flags & MapFlags::AutoCommit) {
      decaf_abort("Attempted to manually uncommit an autocommit mapping");
      return false;
   }

   if (!mapping->address) {
      // Already uncommitted
      return true;
   }

   if (!platform::uncommitMemory(mapping->address, size)) {
      // Failed to uncommit? Really?
      return false;
   }

   mapping->address = 0;
   return true;
}

} // namespace mem
