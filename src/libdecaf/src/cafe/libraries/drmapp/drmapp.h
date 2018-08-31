#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::drmapp
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::drmapp, "drmapp.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
};

} // namespace cafe::drmapp
