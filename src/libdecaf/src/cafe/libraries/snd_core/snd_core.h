#pragma once
#include "cafe/libraries/sndcore2/sndcore2.h"

namespace cafe::snd_core
{

class Library : public sndcore2::Library
{
public:
   Library() :
      sndcore2::Library(hle::LibraryId::snd_core, "snd_core.rpl")
   {
   }
};

} // namespace cafe::snd_core
