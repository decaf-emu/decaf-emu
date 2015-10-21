#pragma once
#include <cassert>
#include "types.h"

namespace mem
{

extern uint8_t *
gBase;

void
initialise();

size_t
base();

bool
valid(ppcaddr_t address);

bool
protect(ppcaddr_t address, size_t size);

bool
alloc(ppcaddr_t address, size_t size);

bool
free(ppcaddr_t address);


// Translate WiiU virtual address to host address
template<typename Type = uint8_t>
static inline Type *
translate(ppcaddr_t address)
{
   if (!address) {
      return nullptr;
   } else {
      return reinterpret_cast<Type*>(gBase + address);
   }
}

template<typename Type>
static inline Type *
translatePtr(Type *ptr)
{
   return reinterpret_cast<Type*>(translate(reinterpret_cast<uint32_t>(ptr)));
}

// Translate host address to WiiU virtual address
static inline ppcaddr_t
untranslate(const void *ptr)
{
   if (!ptr) {
      return 0;
   }

   auto sptr = reinterpret_cast<size_t>(ptr);
   auto sbase = reinterpret_cast<size_t>(gBase);
   assert(sptr > sbase);
   assert(sptr <= sbase + 0xFFFFFFFF);
   return static_cast<ppcaddr_t>(sptr - sbase);
}

// Read Type from virtual address
template<typename Type>
static inline Type
read(ppcaddr_t address)
{
   return byte_swap(readNoSwap<Type>(address));
}

// Read Type from virtual address with no endian byte_swap
template<typename Type>
static inline Type
readNoSwap(ppcaddr_t address)
{
   return *reinterpret_cast<Type*>(translate(address));
}

// Write Type to virtual address
template<typename Type>
static inline void
write(ppcaddr_t address, Type value)
{
   writeNoSwap(address, byte_swap(value));
}

// Write Type to virtual address with no endian byte_swap
template<typename Type>
static inline void
writeNoSwap(ppcaddr_t address, Type value)
{
   *reinterpret_cast<Type*>(translate(address)) = value;
}

} // namespace mem
