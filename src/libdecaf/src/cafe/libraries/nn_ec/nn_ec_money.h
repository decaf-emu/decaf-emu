#pragma once
#include "cafe/libraries/nn_ec/nn_ec_rootobject.h"

#include "cafe/nn/cafe_nn_os_criticalsection.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn_ec
{

struct Money : RootObject
{
   be2_array<char, 44> amount;
   be2_array<char, 16> value;
   be2_array<char, 4> currency;
};
CHECK_OFFSET(Money, 0x00, amount);
CHECK_OFFSET(Money, 0x2C, value);
CHECK_OFFSET(Money, 0x3C, currency);
CHECK_SIZE(Money, 0x40);

virt_ptr<Money>
Money_Constructor(virt_ptr<Money> self,
                  virt_ptr<const char> value,
                  virt_ptr<const char> currency,
                  virt_ptr<const char> amount);

} // namespace cafe::nn_ec
