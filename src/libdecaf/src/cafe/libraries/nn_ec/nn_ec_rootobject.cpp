#include "cafe/libraries/nn_ec/nn_ec.h"
#include "cafe/libraries/nn_ec/nn_ec_memorymanager.h"
#include "cafe/libraries/nn_ec/nn_ec_rootobject.h"

namespace cafe::nn_ec
{

virt_ptr<ghs::TypeDescriptor> RootObject::TypeDescriptor = nullptr;

virt_ptr<void>
RootObject_New(uint32_t size)
{
   return MemoryManager_Allocate(MemoryManager_GetSingleton(), size, 8);
}

virt_ptr<void>
RootObject_PlacementNew(uint32_t size,
                        virt_ptr<void> ptr)
{
   return ptr;
}

void
RootObject_Free(virt_ptr<void> ptr)
{
   MemoryManager_Free(MemoryManager_GetSingleton(), ptr);
}

void
Library::registerRootObjectSymbols()
{
   RegisterFunctionExportName("__nw__Q3_2nn2ec10RootObjectSFUi",
                              RootObject_New);
   RegisterFunctionExportName("__nwa__Q3_2nn2ec10RootObjectSFUi",
                              RootObject_New);
   RegisterFunctionExportName("__nw__Q3_2nn2ec10RootObjectSFUiPv",
                              RootObject_PlacementNew);
   RegisterFunctionExportName("__nwa__Q3_2nn2ec10RootObjectSFUiPv",
                              RootObject_PlacementNew);
   RegisterFunctionExportName("__dl__Q3_2nn2ec10RootObjectSFPv",
                              RootObject_Free);
   RegisterFunctionExportName("__dla__Q3_2nn2ec10RootObjectSFPv",
                              RootObject_Free);

   RegisterTypeInfo(
      RootObject,
      "nn::ec::RootObject",
      {
      },
      {
      });
}

} // namespace cafe::nn_ec
