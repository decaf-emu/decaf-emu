#pragma once
#include "cafe/libraries/nn_ec/nn_ec_rootobject.h"

#include "cafe/nn/cafe_nn_os_criticalsection.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn_ec
{

template<typename T>
struct NonCopyable
{
   static virt_ptr<ghs::TypeDescriptor> TypeDescriptor;

   NonCopyable(const NonCopyable &) = delete;
   NonCopyable &operator =(const NonCopyable &) = delete;
};

} // namespace cafe::nn_ec
