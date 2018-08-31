#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::nfc
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::nfc, "nfc.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
};

} // namespace cafe::nfc
