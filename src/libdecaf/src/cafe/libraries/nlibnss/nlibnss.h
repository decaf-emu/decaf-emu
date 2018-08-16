#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::nlibnss
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::nlibnss, "nlibnss.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
};

} // namespace cafe::nlibnss
