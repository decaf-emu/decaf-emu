#pragma once
#include "debugui_state.h"
#include <string>

#define HEXTOF(h) \
   (static_cast<float>(h & 0xFF) / 255.0f)

#define HEXTOIMV4(h, a) \
   ImVec4 { HEXTOF(h>>16), HEXTOF(h>>8), HEXTOF(h>>0), a }

namespace debugui
{

class Window
{
public:
   enum Flags
   {
      None = 0,
      AlwaysVisible = 1 << 0,
      BringToFront = 1 << 1,
   };

public:
   Window(const std::string &name) :
      mName(name)
   {
   }

   virtual ~Window() = default;

   virtual void draw() = 0;

   void show()
   {
      mVisible = true;
   }

   void bringToFront()
   {
      mFlags = static_cast<Flags>(mFlags | BringToFront);
      mVisible = true;
   }

   void hide()
   {
      if ((mFlags & AlwaysVisible) == 0) {
         mVisible = false;
      }
   }

   Flags flags() const
   {
      return mFlags;
   }

   void setFlags(Flags flags)
   {
      mFlags = flags;
   }

   bool visible() const
   {
      return mVisible;
   }

   const std::string &
   name() const
   {
      return mName;
   }

   void
   initialise(StateTracker *stateTracker)
   {
      mStateTracker = stateTracker;
   }

protected:
   StateTracker *mStateTracker = nullptr;
   bool mVisible = true;
   std::string mName;
   Flags mFlags = Flags::None;
};

} // namespace debugui
