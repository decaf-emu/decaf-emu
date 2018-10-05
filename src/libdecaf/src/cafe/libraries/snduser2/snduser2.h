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

   // For snd_user.rpl to use
   Library(hle::LibraryId id,
           const char *name) :
      hle::Library(id, name)
   {
   }

protected:
   virtual void registerSymbols() override;

private:
   void registerAxArtSymbols();
   void registerAxfxChorusSymbols();
   void registerAxfxChorusExpSymbols();
   void registerAxfxDelaySymbols();
   void registerAxfxDelayExpSymbols();
   void registerAxfxHooksSymbols();
   void registerAxfxMultiChReverbSymbols();
   void registerAxfxReverbHiSymbols();
   void registerAxfxReverbHiExpSymbols();
   void registerAxfxReverbStdSymbols();
   void registerAxfxReverbStdExpSymbols();
   void registerMixSymbols();
   void registerSpSymbols();
};

} // namespace cafe::snduser2
