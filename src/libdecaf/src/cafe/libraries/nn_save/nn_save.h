#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::nn::save
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::nn_save, "nn_save.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
   void registerCmdSymbols();
   void registerPathSymbols();
};

} // namespace cafe::nn::save
