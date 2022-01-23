#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::nn_ec
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::nn_ec, "nn_ec.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

   void registerCatalogSymbols();
   void registerItemListSymbols();
   void registerMemoryManagerSymbols();
   void registerMoneySymbols();
   void registerRootObjectSymbols();
   void registerShoppingCatalogSymbols();

private:
};

} // namespace cafe::nn_ec
