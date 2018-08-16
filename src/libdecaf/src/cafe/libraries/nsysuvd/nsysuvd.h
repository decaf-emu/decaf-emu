#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::nsysuvd
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::nsysuvd, "nsysuvd.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
};

} // namespace cafe::nsysuvd
