#pragma once
#include "cafe/nn/cafe_nn_os_criticalsection.h"

#include "cafe/libraries/ghs/cafe_ghs_typeinfo.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn_ec
{

struct RootObject
{
   static virt_ptr<ghs::TypeDescriptor> TypeDescriptor;
};

virt_ptr<void>
RootObject_New(uint32_t size);

virt_ptr<void>
RootObject_PlacementNew(uint32_t size,
                        virt_ptr<void> ptr);

void
RootObject_Free(virt_ptr<void> ptr);

} // namespace cafe::nn_ec
