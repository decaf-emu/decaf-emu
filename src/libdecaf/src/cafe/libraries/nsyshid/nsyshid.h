#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::nsyshid
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::nsyshid, "nsyshid.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
};

} // namespace cafe::nsyshid
