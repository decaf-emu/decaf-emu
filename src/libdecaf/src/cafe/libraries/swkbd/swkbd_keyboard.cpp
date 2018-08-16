#include "swkbd.h"
#include "swkbd_keyboard.h"
#include <codecvt>
#include <locale>
#include <string>

namespace cafe::swkbd
{

struct StaticKeyboardData
{
   be2_val<State> inputFormState;
   be2_val<State> keyboardState;
   be2_val<bool> okButtonPressed;
   be2_val<bool> cancelButtonPressed;
   be2_virt_ptr<uint8_t> workMemory;
};

static virt_ptr<StaticKeyboardData>
sKeyboardData = nullptr;

static std::string
sInputBuffer;

bool
AppearInputForm(virt_ptr<const AppearArg> arg)
{
   sKeyboardData->inputFormState = State::Visible;
   sKeyboardData->keyboardState = State::Visible;
   sKeyboardData->okButtonPressed = false;
   sKeyboardData->cancelButtonPressed = false;
   sInputBuffer.clear();
   return true;
}


bool
AppearKeyboard(virt_ptr<const KeyboardArg> arg)
{
   sKeyboardData->keyboardState = State::Visible;
   sKeyboardData->okButtonPressed = false;
   sKeyboardData->cancelButtonPressed = false;
   sInputBuffer.clear();
   return true;
}


void
CalcSubThreadFont()
{
}


void
CalcSubThreadPredict()
{
}


void
Calc(virt_ptr<const ControllerInfo> info)
{
}


void
ConfirmUnfixAll()
{
}


void
Create(virt_ptr<unsigned char> workMemory,
       RegionType regionType,
       unsigned int unk,
       virt_ptr<coreinit::FSClient> fsclient)
{
   sKeyboardData->workMemory = workMemory;
}


void
Destroy()
{
}


bool
DisappearInputForm()
{
   return true;
}


bool
DisappearKeyboard()
{
   return true;
}


void
DrawDRC()
{
}


void
DrawTV()
{
}


void
GetDrawStringInfo(virt_ptr<DrawStringInfo> info)
{
}


const virt_ptr<swkbd_char_t>
GetInputFormString()
{
   std::wstring_convert<std::codecvt_utf8_utf16<swkbd_char_t>, swkbd_char_t> converter;
   auto wide = converter.from_bytes(sInputBuffer);
   auto swapped = virt_cast<swkbd_char_t *>(sKeyboardData->workMemory);

   for (auto i = 0; i < wide.size(); ++i) {
      swapped[i] = static_cast<swkbd_char_t>(wide[i]);
   }

   swapped[wide.size()] = swkbd_char_t { 0 };
   return swapped;
}


void
GetKeyboardCondition(virt_ptr<KeyboardCondition> condition)
{
}


State
GetStateInputForm()
{
   return sKeyboardData->inputFormState;
}


State
GetStateKeyboard()
{
   return sKeyboardData->keyboardState;
}


void
InactivateSelectCursor()
{
}


bool
InitLearnDic(virt_ptr<void> dictionary)
{
   return true;
}


bool
IsCoveredWithSubWindow()
{
   return false;
}


bool
IsDecideCancelButton(virt_ptr<bool> outIsSelected)
{
   return sKeyboardData->cancelButtonPressed;
}


bool
IsDecideOkButton(virt_ptr<bool> outIsSelected)
{
   return sKeyboardData->okButtonPressed;
}


bool
IsKeyboardTarget(virt_ptr<const IEventReceiver> receiver)
{
   return false;
}


bool
IsNeedCalcSubThreadFont()
{
   return false;
}


bool
IsNeedCalcSubThreadPredict()
{
   return false;
}


bool
IsSelectCursorActive()
{
   return false;
}


void
MuteAllSound(bool mute)
{
}


void
SetControllerRemo(ControllerType controller)
{
}


void
SetCursorPos(int32_t pos)
{
}


void
SetEnableOkButton(bool enable)
{
}


void
SetInputFormString(virt_ptr<const swkbd_char_t> str)
{
   std::wstring_convert<std::codecvt_utf8_utf16<swkbd_char_t>, swkbd_char_t> converter;
   std::basic_string<swkbd_char_t> wide;

   while (auto c = *str) {
      wide.push_back(c);
      ++str;
   }

   sInputBuffer = converter.to_bytes(wide);
}


void
SetReceiver(virt_ptr<const ReceiverArg> arg)
{
}


void
SetSelectFrom(int32_t pos)
{
}


void
SetUserControllerEventObj(virt_ptr<IControllerEventObj> obj)
{
}


void
SetUserSoundObj(virt_ptr<ISoundObj> obj)
{
}


void
SetVersion(int32_t version)
{
}

void
Library::registerKeyboardSymbols()
{
   RegisterFunctionExportName("SwkbdAppearInputForm__3RplFRCQ3_2nn5swkbd9AppearArg", AppearInputForm);
   RegisterFunctionExportName("SwkbdAppearKeyboard__3RplFRCQ3_2nn5swkbd11KeyboardArg", AppearKeyboard);
   RegisterFunctionExportName("SwkbdCalcSubThreadFont__3RplFv", CalcSubThreadFont);
   RegisterFunctionExportName("SwkbdCalcSubThreadPredict__3RplFv", CalcSubThreadPredict);
   RegisterFunctionExportName("SwkbdCalc__3RplFRCQ3_2nn5swkbd14ControllerInfo", Calc);
   RegisterFunctionExportName("SwkbdConfirmUnfixAll__3RplFv", ConfirmUnfixAll);
   RegisterFunctionExportName("SwkbdCreate__3RplFPUcQ3_2nn5swkbd10RegionTypeUiP8FSClient", Create);
   RegisterFunctionExportName("SwkbdDestroy__3RplFv", Destroy);
   RegisterFunctionExportName("SwkbdDisappearInputForm__3RplFv", DisappearInputForm);
   RegisterFunctionExportName("SwkbdDisappearKeyboard__3RplFv", DisappearKeyboard);
   RegisterFunctionExportName("SwkbdDrawDRC__3RplFv", DrawDRC);
   RegisterFunctionExportName("SwkbdDrawTV__3RplFv", DrawTV);
   RegisterFunctionExportName("SwkbdGetDrawStringInfo__3RplFPQ3_2nn5swkbd14DrawStringInfo", GetDrawStringInfo);
   RegisterFunctionExportName("SwkbdGetInputFormString__3RplFv", GetInputFormString);
   RegisterFunctionExportName("SwkbdGetKeyboardCondition__3RplFPQ3_2nn5swkbd17KeyboardCondition", GetKeyboardCondition);
   RegisterFunctionExportName("SwkbdGetStateInputForm__3RplFv", GetStateInputForm);
   RegisterFunctionExportName("SwkbdGetStateKeyboard__3RplFv", GetStateKeyboard);
   RegisterFunctionExportName("SwkbdInactivateSelectCursor__3RplFv", InactivateSelectCursor);
   RegisterFunctionExportName("SwkbdInitLearnDic__3RplFPv", InitLearnDic);
   RegisterFunctionExportName("SwkbdIsCoveredWithSubWindow__3RplFv", IsCoveredWithSubWindow);
   RegisterFunctionExportName("SwkbdIsDecideCancelButton__3RplFPb", IsDecideCancelButton);
   RegisterFunctionExportName("SwkbdIsDecideOkButton__3RplFPb", IsDecideOkButton);
   RegisterFunctionExportName("SwkbdIsKeyboardTarget__3RplFPQ3_2nn5swkbd14IEventReceiver", IsKeyboardTarget);
   RegisterFunctionExportName("SwkbdIsNeedCalcSubThreadFont__3RplFv", IsNeedCalcSubThreadFont);
   RegisterFunctionExportName("SwkbdIsNeedCalcSubThreadPredict__3RplFv", IsNeedCalcSubThreadPredict);
   RegisterFunctionExportName("SwkbdIsSelectCursorActive__3RplFv", IsSelectCursorActive);
   RegisterFunctionExportName("SwkbdMuteAllSound__3RplFb", MuteAllSound);
   RegisterFunctionExportName("SwkbdSetControllerRemo__3RplFQ3_2nn5swkbd14ControllerType", SetControllerRemo);
   RegisterFunctionExportName("SwkbdSetCursorPos__3RplFi", SetCursorPos);
   RegisterFunctionExportName("SwkbdSetEnableOkButton__3RplFb", SetEnableOkButton);
   RegisterFunctionExportName("SwkbdSetInputFormString__3RplFPCw", SetInputFormString);
   RegisterFunctionExportName("SwkbdSetReceiver__3RplFRCQ3_2nn5swkbd11ReceiverArg", SetReceiver);
   RegisterFunctionExportName("SwkbdSetSelectFrom__3RplFi", SetSelectFrom);
   RegisterFunctionExportName("SwkbdSetUserControllerEventObj__3RplFPQ3_2nn5swkbd19IControllerEventObj", SetUserControllerEventObj);
   RegisterFunctionExportName("SwkbdSetUserSoundObj__3RplFPQ3_2nn5swkbd9ISoundObj", SetUserSoundObj);
   RegisterFunctionExportName("SwkbdSetVersion__3RplFi", SetVersion);
}

namespace internal
{

void
injectTextInput(const char *input)
{
   if (sKeyboardData->keyboardState == State::Visible) {
      sInputBuffer += input;
   }
}

void
injectKeyInput(decaf::input::KeyboardKey key,
               decaf::input::KeyboardAction action)
{
   if (sKeyboardData->keyboardState != State::Visible) {
      return;
   }

   if (action == decaf::input::KeyboardAction::Release) {
      if (key == decaf::input::KeyboardKey::Enter) {
         sKeyboardData->okButtonPressed = true;
         sKeyboardData->inputFormState = State::Hidden;
         sKeyboardData->keyboardState = State::Hidden;
      } else if (key == decaf::input::KeyboardKey::Escape) {
         sKeyboardData->cancelButtonPressed = true;
         sKeyboardData->inputFormState = State::Hidden;
         sKeyboardData->keyboardState = State::Hidden;
      }
   }
}

} // namespace internal

} // namespace cafe::swkbd

