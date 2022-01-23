#include "cafe/libraries/nn_ec/nn_ec.h"
#include "cafe/libraries/nn_ec/nn_ec_itemlist.h"
#include "cafe/libraries/nn_ec/nn_ec_rootobject.h"

namespace cafe::nn_ec
{

virt_ptr<ghs::TypeDescriptor> NonCopyable<ItemList>::TypeDescriptor = nullptr;

virt_ptr<ghs::VirtualTable> ItemList::VirtualTable = nullptr;
virt_ptr<ghs::TypeDescriptor> ItemList::TypeDescriptor = nullptr;

virt_ptr<ItemList>
ItemList_Constructor(virt_ptr<ItemList> self)
{
   if (!self) {
      self = virt_cast<ItemList *>(RootObject_New(sizeof(ItemList)));
      if (!self) {
         return nullptr;
      }
   }

   self->impl = nullptr;
   self->virtualTable = ItemList::VirtualTable;
   return self;
}

void
ItemList_Destructor(virt_ptr<ItemList> self,
                    ghs::DestructorFlags flags)
{
   self->virtualTable = ItemList::VirtualTable;

   if (flags & ghs::DestructorFlags::FreeMemory) {
      RootObject_Free(self);
   }
}


void
Library::registerItemListSymbols()
{
   RegisterFunctionExportName("__ct__Q3_2nn2ec8ItemListFv",
                              ItemList_Constructor);
   RegisterFunctionExportName("__dt__Q3_2nn2ec8ItemListFv",
                              ItemList_Destructor);

   RegisterTypeInfo(
      NonCopyable<ItemList>,
      "nn::ec::NonCopyable<nn::ec::ItemList>",
      {
      },
      {
      });

   RegisterTypeInfo(
      ItemList,
      "nn::ec::ItemList",
      {
         "__dt__Q3_2nn2ec8ItemListFv",
      },
      {
         "nn::ec::RootObject",
         "nn::ec::NonCopyable<nn::ec::ItemList>",
      });
}

} // namespace cafe::nn_ec
