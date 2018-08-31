#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::uvd
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::uvd, "uvd.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
};

} // namespace cafe::uvd
