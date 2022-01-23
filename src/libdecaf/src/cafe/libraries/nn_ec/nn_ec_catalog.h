#pragma once
#include "cafe/libraries/nn_ec/nn_ec_itemlist.h"

#include "cafe/libraries/ghs/cafe_ghs_typeinfo.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn_ec
{

struct Catalog : ItemList
{
   static virt_ptr<ghs::VirtualTable> VirtualTable;
   static virt_ptr<ghs::TypeDescriptor> TypeDescriptor;
};

virt_ptr<Catalog>
Catalog_Constructor(virt_ptr<Catalog> self);

void
Catalog_Destructor(virt_ptr<Catalog> self,
                  ghs::DestructorFlags flags);

} // namespace cafe::nn_ec
