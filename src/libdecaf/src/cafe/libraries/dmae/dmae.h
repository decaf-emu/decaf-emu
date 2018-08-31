#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::dmae
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::dmae, "dmae.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
   void registerRingSymbols();
};

} // namespace cafe::dmae
