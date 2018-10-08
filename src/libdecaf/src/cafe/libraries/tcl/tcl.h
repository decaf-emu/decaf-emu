#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::tcl
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::tcl, "tcl.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
   void registerApertureSymbols();
   void registerDriverSymbols();
   void registerInterruptHandlerSymbols();
   void registerRegisterSymbols();
   void registerRingSymbols();
};

} // namespace cafe::tcl
