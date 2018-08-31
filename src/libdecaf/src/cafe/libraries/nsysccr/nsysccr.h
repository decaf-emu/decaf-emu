#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::nsysccr
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::nsysccr, "nsysccr.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
};

} // namespace cafe::nsysccr
