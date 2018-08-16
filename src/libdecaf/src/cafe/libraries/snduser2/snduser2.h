#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::snduser2
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::snduser2, "snduser2.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
   void registerAxfxSymbols();
};

} // namespace cafe::snduser2
