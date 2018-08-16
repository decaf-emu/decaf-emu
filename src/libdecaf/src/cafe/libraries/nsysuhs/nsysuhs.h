#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::nsysuhs
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::nsysuhs, "nsysuhs.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
};

} // namespace cafe::nsysuhs
