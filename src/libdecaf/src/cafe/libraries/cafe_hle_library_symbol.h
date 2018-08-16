#pragma once
#include <libcpu/be2_struct.h>
#include <string>

namespace cafe::hle
{

struct LibrarySymbol
{
   static constexpr auto InvalidOffset = 0xCD000000;

   enum Type
   {
      Undefined,
      Function,
      Data,
   };

   LibrarySymbol(Type type) :
      type(type)
   {
   }

   //! Symbol type
   Type type = Undefined;

   //! Symbol index in library
   uint32_t index = 0;

   //! Symbol name
   std::string name;

   //! Whether the symbol is exported or not
   bool exported = false;

   //! Offset in .text or .data section
   uint32_t offset = InvalidOffset;

   //! Virtual address of this symbol
   //! TODO: Change stuff to use offset when we go multi-process!
   virt_addr address = virt_addr { 0 };
};

} // namespace cafe
