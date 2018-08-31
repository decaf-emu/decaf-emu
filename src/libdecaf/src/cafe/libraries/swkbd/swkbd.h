#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::swkbd
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::swkbd, "swkbd.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
   void registerKeyboardSymbols();
};

} // namespace cafe::swkbd
