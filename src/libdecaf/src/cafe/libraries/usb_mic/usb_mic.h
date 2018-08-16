#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::usb_mic
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::usb_mic, "usb_mic.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
};

} // namespace cafe::usb_mic
