#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::camera
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::camera, "camera.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
   void registerCamSymbols();
};

} // namespace cafe::camera
