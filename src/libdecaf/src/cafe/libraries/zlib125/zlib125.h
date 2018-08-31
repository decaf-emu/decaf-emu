#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::zlib125
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::zlib125, "zlib125.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
   void registerZlibSymbols();
};

} // namespace cafe::zlib125
