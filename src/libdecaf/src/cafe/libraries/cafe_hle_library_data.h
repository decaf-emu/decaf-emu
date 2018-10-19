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

   virtual ~LibraryData()
   {
   }

   //! Pointer to the host pointer to guest memory which we should update
   virt_ptr<void> *hostPointer = nullptr;

   //! Host constructor to call on allocated memory
   void (*constructor)(void *) = nullptr;

   //! Size of this data symbol
   uint32_t size = 0;

   //! Align of this data symbol
   uint32_t align = 0;
};

} // namespace cafe
