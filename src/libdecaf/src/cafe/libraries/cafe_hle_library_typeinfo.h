#pragma once
#include "cafe/libraries/ghs/cafe_ghs_typeinfo.h"

#include <libcpu/be2_struct.h>
#include <vector>

namespace cafe::hle
{

struct LibraryTypeInfo
{
   const char *name = nullptr;
   std::vector<const char *> virtualTable;
   std::vector<const char *> baseTypes;
   virt_ptr<ghs::VirtualTable> *hostVirtualTablePtr = nullptr;
   virt_ptr<ghs::TypeDescriptor> *hostTypeDescriptorPtr = nullptr;

   uint32_t nameOffset = 0u;
   uint32_t baseTypeOffset = 0u;
   uint32_t typeDescriptorOffset = 0u;
   uint32_t typeIdOffset = 0u;
   uint32_t virtualTableOffset = 0u;
};

} // namespace cafe::hle
