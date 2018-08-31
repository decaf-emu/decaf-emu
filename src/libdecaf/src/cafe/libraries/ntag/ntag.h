#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::ntag
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::ntag, "ntag.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
};

} // namespace cafe::ntag
