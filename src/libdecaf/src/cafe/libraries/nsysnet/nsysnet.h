#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::nsysnet
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::nsysnet, "nsysnet.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
   void registerEndianSymbols();
   void registerSocketLibSymbols();
   void registerSslSymbols();
};

} // namespace cafe::nsysnet
