#pragma once
#include <cstdint>

struct SystemObjectHeader
{
   uint32_t tag;
   void *object;
};

struct SystemObject
{
   virtual ~SystemObject()
   {
   }

   uint32_t objectTag;
   uint32_t objectVirtualAddress;
};
