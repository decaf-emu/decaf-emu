#pragma once
#include "cafe/libraries/nn_ec/nn_ec_rootobject.h"
#include "cafe/libraries/nn_ec/nn_ec_noncopyable.h"

#include "cafe/libraries/ghs/cafe_ghs_typeinfo.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn_ec
{

struct ItemList : RootObject, NonCopyable<ItemList>
{
   static virt_ptr<ghs::VirtualTable> VirtualTable;
   static virt_ptr<ghs::TypeDescriptor> TypeDescriptor;

   be2_virt_ptr<void> impl;
   be2_virt_ptr<ghs::VirtualTable> virtualTable;
};

virt_ptr<ItemList>
ItemList_Constructor(virt_ptr<ItemList> self);

void
ItemList_Destructor(virt_ptr<ItemList> self,
                    ghs::DestructorFlags flags);

} // namespace cafe::nn_ec
