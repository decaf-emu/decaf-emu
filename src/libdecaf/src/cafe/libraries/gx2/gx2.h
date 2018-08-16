#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::gx2
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::gx2, "gx2.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
   void registerApertureSymbols();
   void registerClearSymbols();
   void registerContextStateSymbols();
   void registerCounterSymbols();
   void registerDebugSymbols();
   void registerDisplaySymbols();
   void registerDisplayListSymbols();
   void registerDrawSymbols();
   void registerEventSymbols();
   void registerFetchShadersSymbols();
   void registerFormatSymbols();
   void registerMemorySymbols();
   void registerQuerySymbols();
   void registerRegistersSymbols();
   void registerSamplerSymbols();
   void registerShadersSymbols();
   void registerStateSymbols();
   void registerSurfaceSymbols();
   void registerSwapSymbols();
   void registerTempSymbols();
   void registerTessellationSymbols();
   void registerTextureSymbols();
   void registerGx2rBufferSymbols();
   void registerGx2rDisplayListSymbols();
   void registerGx2rDrawSymbols();
   void registerGx2rMemorySymbols();
   void registerGx2rResourceSymbols();
   void registerGx2rShadersSymbols();
   void registerGx2rSurfaceSymbols();
};

} // namespace cafe::gx2
