#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::snd_core
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::snd_core, "snd_core.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
};

} // namespace cafe::snd_core
