#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::snd_user
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::snd_user, "snd_user.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
};

} // namespace cafe::snd_user
