#include "cafe/libraries/nn_ec/nn_ec.h"
#include "cafe/libraries/nn_ec/nn_ec_catalog.h"
#include "cafe/libraries/nn_ec/nn_ec_rootobject.h"

namespace cafe::nn_ec
{

virt_ptr<ghs::VirtualTable> Catalog::VirtualTable = nullptr;
virt_ptr<ghs::TypeDescriptor> Catalog::TypeDescriptor = nullptr;

virt_ptr<Catalog>
Catalog_Constructor(virt_ptr<Catalog> self)
{
   if (!self) {
      self = virt_cast<Catalog *>(RootObject_New(sizeof(Catalog)));
      if (!self) {
         return nullptr;
      }
   }

   ItemList_Constructor(virt_cast<ItemList *>(self));

   self->impl = nullptr;
   self->virtualTable = Catalog::VirtualTable;
   return self;
}

void
Catalog_Destructor(virt_ptr<Catalog> self,
                  ghs::DestructorFlags flags)
{
   self->virtualTable = Catalog::VirtualTable;

   ItemList_Destructor(virt_cast<ItemList *>(self),
                        ghs::DestructorFlags::None);

   if (flags & ghs::DestructorFlags::FreeMemory) {
      RootObject_Free(self);
   }
}

void
Library::registerCatalogSymbols()
{
   RegisterFunctionExportName("__ct__Q3_2nn2ec7CatalogFv",
                              Catalog_Constructor);
   RegisterFunctionExportName("__dt__Q3_2nn2ec7CatalogFv",
                              Catalog_Destructor);

   RegisterTypeInfo(
      Catalog,
      "nn::ec::Catalog",
      {
         "__dt__Q3_2nn2ec7CatalogFv",
      },
      {
         "nn::ec::ItemList",
      });
}

} // namespace cafe::nn_ec
