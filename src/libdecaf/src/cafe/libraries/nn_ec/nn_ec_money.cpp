#include "cafe/libraries/nn_ec/nn_ec.h"
#include "cafe/libraries/nn_ec/nn_ec_money.h"

#include "common/strutils.h"

namespace cafe::nn_ec
{

virt_ptr<Money>
Money_Constructor(virt_ptr<Money> self,
                  virt_ptr<const char> value,
                  virt_ptr<const char> currency,
                  virt_ptr<const char> amount)
{
   if (!self) {
      self = virt_cast<Money *>(RootObject_New(sizeof(Money)));
      if (!self) {
         return nullptr;
      }
   }

   if (value) {
      string_copy(virt_addrof(self->value).get(), value.get(), self->value.size() - 1);
   } else {
      self->value[0] = '\0';
   }

   if (currency && strnlen(currency.get(), 4) == 3) {
      memcpy(virt_addrof(self->currency).get(), currency.get(), 4);
   } else {
      self->currency[0] = '\0';
   }

   if (amount) {
      string_copy(virt_addrof(self->amount).get(), amount.get(), self->amount.size() - 1);
   } else {
      // TODO: Does something with currency and value to generate an amount string
      self->amount = "0";
   }

   return self;
}

void
Library::registerMoneySymbols()
{
   RegisterFunctionExportName("__ct__Q3_2nn2ec5MoneyFPCcN21",
                              Money_Constructor);
}

} // namespace cafe::nn_ec
