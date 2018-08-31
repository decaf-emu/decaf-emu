#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::mic
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::mic, "mic.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
   void registerMicSymbols();
};

} // namespace cafe::mic
