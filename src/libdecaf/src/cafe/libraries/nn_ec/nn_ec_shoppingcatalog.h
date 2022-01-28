#pragma once
#include "cafe/libraries/nn_ec/nn_ec_catalog.h"

#include "cafe/libraries/ghs/cafe_ghs_typeinfo.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn_ec
{

struct ShoppingCatalog : Catalog
{
   static virt_ptr<ghs::VirtualTable> VirtualTable;
   static virt_ptr<ghs::TypeDescriptor> TypeDescriptor;
};

virt_ptr<ShoppingCatalog>
ShoppingCatalog_Constructor(virt_ptr<ShoppingCatalog> self);

void
ShoppingCatalog_Destructor(virt_ptr<ShoppingCatalog> self,
                           ghs::DestructorFlags flags);

} // namespace cafe::nn_ec
