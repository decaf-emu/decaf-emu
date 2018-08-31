#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::nlibnss2
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::nlibnss2, "nlibnss2.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
};

} // namespace cafe::nlibnss2
