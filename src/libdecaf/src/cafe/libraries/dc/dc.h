#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::dc
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::dc, "dc.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
};

} // namespace cafe::dc
