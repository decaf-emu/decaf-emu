#include "cafe/libraries/nn_ec/nn_ec.h"
#include "cafe/libraries/nn_ec/nn_ec_shoppingcatalog.h"
#include "cafe/libraries/nn_ec/nn_ec_rootobject.h"

namespace cafe::nn_ec
{

virt_ptr<ghs::VirtualTable> ShoppingCatalog::VirtualTable = nullptr;
virt_ptr<ghs::TypeDescriptor> ShoppingCatalog::TypeDescriptor = nullptr;

virt_ptr<ShoppingCatalog>
ShoppingCatalog_Constructor(virt_ptr<ShoppingCatalog> self)
{
   if (!self) {
      self = virt_cast<ShoppingCatalog *>(RootObject_New(sizeof(ShoppingCatalog)));
      if (!self) {
         return nullptr;
      }
   }

   Catalog_Constructor(virt_cast<Catalog *>(self));

   self->impl = nullptr;
   self->virtualTable = ShoppingCatalog::VirtualTable;
   return self;
}

void
ShoppingCatalog_Destructor(virt_ptr<ShoppingCatalog> self,
                           ghs::DestructorFlags flags)
{
   self->virtualTable = ShoppingCatalog::VirtualTable;

   Catalog_Destructor(virt_cast<Catalog *>(self),
                      ghs::DestructorFlags::None);

   if (flags & ghs::DestructorFlags::FreeMemory) {
      RootObject_Free(self);
   }
}

void
Library::registerShoppingCatalogSymbols()
{
   RegisterFunctionExportName("__ct__Q3_2nn2ec15ShoppingCatalogFv",
                              ShoppingCatalog_Constructor);
   RegisterFunctionExportName("__dt__Q3_2nn2ec15ShoppingCatalogFv",
                              ShoppingCatalog_Destructor);

   RegisterTypeInfo(
      ShoppingCatalog,
      "nn::ec::ShoppingCatalog",
      {
         "__dt__Q3_2nn2ec15ShoppingCatalogFv",
      },
      {
         "nn::ec::Catalog",
      });
}

} // namespace cafe::nn_ec
