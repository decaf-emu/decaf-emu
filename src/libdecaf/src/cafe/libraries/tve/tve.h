#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::tve
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::tve, "tve.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
};

} // namespace cafe::tve
