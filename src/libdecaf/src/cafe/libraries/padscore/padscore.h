#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::padscore
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::padscore, "padscore.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
   void registerKpadSymbols();
   void registerWpadSymbols();
};

} // namespace cafe::padscore
