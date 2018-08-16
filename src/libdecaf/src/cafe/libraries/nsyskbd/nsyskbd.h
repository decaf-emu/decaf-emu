#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::nsyskbd
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::nsyskbd, "nsyskbd.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
   void registerKprSymbols();
   void registerSkbdSymbols();
};

} // namespace cafe::nsyskbd
