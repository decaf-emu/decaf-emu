#pragma once
#include "cafe/libraries/snduser2/snduser2.h"

namespace cafe::snd_user
{

class Library : public snduser2::Library
{
public:
   Library() :
      snduser2::Library(hle::LibraryId::snd_user, "snd_user.rpl")
   {
   }
};

} // namespace cafe::snd_user
