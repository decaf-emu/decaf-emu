#pragma once
#include "debugui.h"
#include "debugui_state.h"
#include "debugui_window.h"

#include <array>
#include <map>
#include <libdecaf/decaf_debug_api.h>
#include <string>
#include <vector>

namespace debugui
{

using CafeThread = decaf::debug::CafeThread;

enum class WindowID
{
   Invalid,
   DisassemblyWindow,
   InfoWindow,
   MemoryWindow,
   RegistersWindow,
   SegmentsWindow,
   StackWindow,
   StatsWindow,
   ThreadsWindow,
   VoicesWindow,
   PerformanceWindow,
};

struct HotKey
{
   HotKey() :
      HotKey(KeyboardKey::Unknown, KeyboardKey::Unknown, false)
   {
   }

   HotKey(KeyboardKey key) :
      HotKey(KeyboardKey::Unknown, key, false)
   {
   }

   HotKey(KeyboardKey key, bool repeat) :
      HotKey(KeyboardKey::Unknown, key, repeat)
   {
   }

   HotKey(KeyboardKey modifier, KeyboardKey key) :
      HotKey(modifier, key, false)
   {
   }

   HotKey(KeyboardKey m, KeyboardKey k, bool r) :
      modifier(m),
      key(k),
      repeat(r)
   {
      switch (modifier) {
      case KeyboardKey::LeftControl:
         str += "CTRL+";
         break;
      case KeyboardKey::RightShift:
      case KeyboardKey::LeftShift:
         str += "SHIFT+";
         break;
      case KeyboardKey::LeftAlt:
      case KeyboardKey::RightAlt:
         str += "ALT+";
         break;
      case KeyboardKey::LeftSuper:
      case KeyboardKey::RightSuper:
         str += "CMD+";
         break;
      }

      if (key >= KeyboardKey::A && key <= KeyboardKey::Z) {
         str += 'A' + static_cast<int>(key) - static_cast<int>(KeyboardKey::A);
      } else if (key >= KeyboardKey::F1 && key <= KeyboardKey::F12) {
         str += 'F';
         str += std::to_string(1 + static_cast<int>(key) - static_cast<int>(KeyboardKey::F1));
      }
   }

   operator const char *()
   {
      return str.c_str();
   }

   KeyboardKey modifier;
   KeyboardKey key;
   bool repeat;
   std::string str;
};

class Manager : public StateTracker
{
public:
   Manager(const std::string &uiConfigPath);
   ~Manager();

   void
   draw(unsigned width, unsigned height);

   void
   drawMenu();

   void
   updateMouseState();

   bool
   onMouseAction(MouseButton button, MouseAction action);

   bool
   onMouseMove(float x, float y);

   bool
   onMouseScroll(float x, float y);

   bool
   onKeyAction(KeyboardKey key, KeyboardAction action);

   bool
   onText(const char *text);

   bool
   checkHotKey(const HotKey &hk);

   void
   checkHotKeys();

   void
   setClipboardCallbacks(ClipboardTextGetCallback getClipboardFn,
                         ClipboardTextSetCallback setClipboardFn);

   const char *
   getClipboardText();

   void
   setClipboardText(const char *text);

protected:
   virtual void
   setActiveThread(const CafeThread &thread) override;

   virtual const CafeThread &
   getActiveThread() override;

   virtual unsigned
   getResumeCount() override;

   virtual void
   gotoDisassemblyAddress(uint32_t address) override;

   virtual void
   gotoMemoryAddress(uint32_t address) override;

   virtual void
   gotoStackAddress(uint32_t address) override;

private:
   void
   onPaused();

   void
   onResumed();

   void
   onFirstActivation();

   void
   onFocusThread(CafeThread &thread);

   void
   addWindow(WindowID id,
             Window *window,
             HotKey hotkey = HotKey { });

   Window *
   getWindow(WindowID id);

private:
   std::string mConfigPath;

   // Clipboard
   std::string mClipboardText;
   ClipboardTextGetCallback mGetClipboardFn;
   ClipboardTextSetCallback mSetClipboardFn;

   // Mouse state
   std::array<bool, 3> mMouseClicked = { false, false, false };
   std::array<bool, 3> mMousePressed = { false, false, false };
   float mMouseScrollX = 0.0f;
   float mMouseScrollY = 0.0f;
   float mMousePosX = -1.0f;
   float mMousePosY = -1.0f;

   // Renderer
   bool mPaused = false;
   uint32_t mPausedNia = 0;
   bool mHasBeenActivated = false;
   bool mWasPaused = false;
   bool mVisible = false;
   std::map<WindowID, Window *> mWindows;
   std::map<WindowID, HotKey> mWindowHotKeys;

   // UI State
   bool mJitProfilingEnabled = false;
   unsigned mJitProfilingMask = 0;
   unsigned mResumeCount = 0;
   CafeThread mActiveThread { };
};

} // namespace namespace debugui
