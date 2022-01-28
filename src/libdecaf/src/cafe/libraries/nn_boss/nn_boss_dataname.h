#pragma once
#include "cafe/libraries/ghs/cafe_ghs_typeinfo.h"
#include "nn/nn_result.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn_boss
{

struct DataName
{
   be2_array<char, 32> name;
};
CHECK_SIZE(DataName, 0x20);

virt_ptr<DataName>
DataName_Constructor(virt_ptr<DataName> self);

virt_ptr<DataName>
DataName_Constructor(virt_ptr<DataName> self,
                     virt_ptr<const char> name);

virt_ptr<DataName>
DataName_Constructor(virt_ptr<DataName> self,
                     virt_ptr<const DataName> other);

virt_ptr<const char>
DataName_OperatorCastConstCharPtr(virt_ptr<const DataName> self);

virt_ptr<DataName>
DataName_OperatorAssign(virt_ptr<DataName> self,
                        virt_ptr<const char> name);

bool
DataName_OperatorEqual(virt_ptr<const DataName> self,
                       virt_ptr<const DataName> other);

bool
DataName_OperatorEqual(virt_ptr<const DataName> self,
                       virt_ptr<const char> name);

bool
DataName_OperatorNotEqual(virt_ptr<const DataName> self,
                          virt_ptr<const DataName> other);

bool
DataName_OperatorNotEqual(virt_ptr<const DataName> self,
                          virt_ptr<const char> name);

}  // namespace cafe::nn_boss
