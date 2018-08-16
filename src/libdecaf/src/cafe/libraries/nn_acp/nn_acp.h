#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::nn::acp
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::nn_acp, "nn_acp.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
   void registerDeviceSymbols();
   void registerTitleSymbols();
};

} // namespace cafe::nn::acp
