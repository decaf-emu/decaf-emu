#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::vpad
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::vpad, "vpad.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
   void registerControllerSymbols();
   void registerGyroSymbols();
   void registerMotorSymbols();
};

} // namespace cafe::vpad
