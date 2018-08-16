#pragma once
#include "cafe_hle_library_symbol.h"
#include <libcpu/be2_struct.h>

namespace cafe::hle
{

struct LibraryData : LibrarySymbol
{
   LibraryData() :
      LibrarySymbol(LibrarySymbol::Data)
   {
   }

   //! Pointer to the host pointer to guest memory which we should update
   virt_ptr<void> *hostPointer = nullptr;

   //! Size of this data symbol
   uint32_t size = 0;
};

} // namespace cafe
