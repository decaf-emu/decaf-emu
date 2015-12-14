#include <map>
#include "fakevirtualmemory.h"
#include "memory_translate.h"
#include "gpu/pm4_buffer.h"

class FakeVirtualMemory
{
public:
   void mapMemory(uint32_t virtualAddress, void *hostAddress)
   {
      mToVirtual.emplace(virtualAddress, hostAddress);
      mFromVirtual.emplace(hostAddress, virtualAddress);
   }

   uint32_t mapMemory(void *hostAddress)
   {
      while (mToVirtual.find(mUniqueAddress) != mToVirtual.end()) {
         mUniqueAddress++;
      }

      mapMemory(mUniqueAddress, hostAddress);
      return mUniqueAddress;
   }

   void *translate(uint32_t virtualAddress)
   {
      auto itr = mToVirtual.find(virtualAddress);

      if (itr == mToVirtual.end()) {
         throw std::logic_error("Tried to translate unmapped virtual memory");
      }

      return itr->second;
   }

   uint32_t untranslate(const void *hostAddress)
   {
      auto itr = mFromVirtual.find(hostAddress);

      if (itr == mFromVirtual.end()) {
         throw std::logic_error("Tried to translate unmapped virtual memory");
      }

      return itr->second;
   }

private:
   uint32_t mUniqueAddress = 1;
   std::map<uint32_t, void *> mToVirtual;
   std::map<const void *, uint32_t> mFromVirtual;
};

static FakeVirtualMemory
gFakeMemory;

void
memory_virtualmap(uint32_t addr, void *pointer)
{
   if (addr != 0 && pointer != nullptr) {
      gFakeMemory.mapMemory(addr, pointer);
   }
}

uint32_t
memory_virtualmap(void *pointer)
{
   if (pointer == nullptr) {
      return 0;
   } else {
      return gFakeMemory.mapMemory(pointer);
   }
}

void *
memory_translate(ppcaddr_t address)
{
   if (address == 0) {
      return nullptr;
   } else {
      return gFakeMemory.translate(address);
   }
}

ppcaddr_t
memory_untranslate(const void *pointer)
{
   if (pointer == nullptr) {
      return 0;
   } else {
      return gFakeMemory.untranslate(pointer);
   }
}

namespace pm4
{

Buffer *
getBuffer(uint32_t size)
{
   return nullptr;
}

Buffer *
flushBuffer(Buffer *buffer)
{
   return nullptr;
}

} // namespace pm4
