#include "cafe/libraries/nn_ec/nn_ec.h"
#include "cafe/libraries/nn_ec/nn_ec_query.h"
#include "cafe/libraries/nn_ec/nn_ec_rootobject.h"

#include "cafe/libraries/cafe_hle_stub.h"

namespace cafe::nn_ec
{

virt_ptr<Query>
Query_Constructor(virt_ptr<Query> self)
{
   if (!self) {
      self = virt_cast<Query *>(RootObject_New(sizeof(Query)));
      if (!self) {
         return nullptr;
      }
   }

   self->impl = nullptr;
   return self;
}

void
Query_Destructor(virt_ptr<Query> self,
                 ghs::DestructorFlags flags)
{
   Query_Clear(self);

   if (flags & ghs::DestructorFlags::FreeMemory) {
      RootObject_Free(self);
   }
}

void
Query_Clear(virt_ptr<Query> self)
{
   if (self->impl) {
      decaf_warn_stub();
   }

   self->impl = nullptr;
}

void
Library::registerQuerySymbols()
{
   RegisterFunctionExportName("__ct__Q3_2nn2ec5QueryFv",
                              Query_Constructor);
   RegisterFunctionExportName("__dt__Q3_2nn2ec5QueryFv",
                              Query_Destructor);
   RegisterFunctionExportName("Clear__Q3_2nn2ec5QueryFv",
                              Query_Clear);
}

} // namespace cafe::nn_ec
