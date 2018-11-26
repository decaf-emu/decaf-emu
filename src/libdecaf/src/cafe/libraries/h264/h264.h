#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::h264
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::h264, "h264.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
   void registerDecodeSymbols();
   void registerStreamSymbols();
};

} // namespace cafe::h264
