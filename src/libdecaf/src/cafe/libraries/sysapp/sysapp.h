#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::sysapp
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::sysapp, "sysapp.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
   void registerTitleSymbols();
};

} // namespace cafe::zlib125
