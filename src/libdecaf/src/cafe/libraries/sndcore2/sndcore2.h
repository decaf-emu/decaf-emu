#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::sndcore2
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::sndcore2, "sndcore2.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
   void registerAiSymbols();
   void registerConfigSymbols();
   void registerDeviceSymbols();
   void registerFxSymbols();
   void registerMixSymbols();
   void registerRmtSymbols();
   void registerVoiceSymbols();
   void registerVsSymbols();
};

} // namespace cafe::sndcore2
