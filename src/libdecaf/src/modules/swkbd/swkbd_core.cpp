#include "swkbd.h"
#include "swkbd_core.h"
#include <codecvt>
#include <locale>

namespace nn
{

namespace swkbd
{

static State::State
sInputFormState = State::Hidden;

static State::State
sKeyboardState = State::Hidden;

static bool
sOkButtonPressed = false;

static bool
sCancelButtonPressed = false;

static std::string
sInputBuffer;

static uint8_t *
sWorkMemory = nullptr;

bool
AppearInputForm(const AppearArg *arg)
{
   sInputFormState = State::Visible;
   sKeyboardState = State::Visible;
   sOkButtonPressed = false;
   sCancelButtonPressed = false;
   sInputBuffer.clear();
   return true;
}


bool
AppearKeyboard(const KeyboardArg *arg)
{
   sKeyboardState = State::Visible;
   sOkButtonPressed = false;
   sCancelButtonPressed = false;
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
Calc(const ControllerInfo *info)
{
}


void
ConfirmUnfixAll()
{
}


void
Create(unsigned char *workMemory,
       RegionType::Region region,
       unsigned int,
       FSClient *fsclient)
{
   sWorkMemory = workMemory;
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
GetDrawStringInfo(DrawStringInfo *info)
{
}


const be_val<uint16_t> *
GetInputFormString()
{
   std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
   auto wide = converter.from_bytes(sInputBuffer);
   auto result = reinterpret_cast<be_val<uint16_t> *>(sWorkMemory);
   for (auto i = 0; i < wide.size(); ++i) {
      result[i] = static_cast<uint16_t>(wide[i]);
   }
   result[wide.size()] = 0;
   return result;
}


void
GetKeyboardCondition(KeyboardCondition *condition)
{
}


State::State
GetStateInputForm()
{
   return sInputFormState;
}


State::State
GetStateKeyboard()
{
   return sKeyboardState;
}


void
InactivateSelectCursor()
{
}


bool
InitLearnDic(void *dictionary)
{
   return true;
}


bool
IsCoveredWithSubWindow()
{
   return false;
}


bool
IsDecideCancelButton(bool *v)
{
   return sCancelButtonPressed;
}


bool
IsDecideOkButton(bool *v)
{
   return sOkButtonPressed;
}


bool
IsKeyboardTarget(const IEventReceiver *receiver)
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
SetControllerRemo(ControllerType::Type controller)
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
SetInputFormString(const char_t *str)
{
}


void
SetReceiver(const ReceiverArg *arg)
{
}


void
SetSelectFrom(int32_t pos)
{
}


void
SetUserControllerEventObj(IControllerEventObj *obj)
{
}


void
SetUserSoundObj(ISoundObj *obj)
{
}


void
SetVersion(int32_t version)
{
}

void
Module::registerCoreFunctions()
{
   RegisterKernelFunctionName("SwkbdAppearInputForm__3RplFRCQ3_2nn5swkbd9AppearArg", nn::swkbd::AppearInputForm);
   RegisterKernelFunctionName("SwkbdAppearKeyboard__3RplFRCQ3_2nn5swkbd11KeyboardArg", nn::swkbd::AppearKeyboard);
   RegisterKernelFunctionName("SwkbdCalcSubThreadFont__3RplFv", nn::swkbd::CalcSubThreadFont);
   RegisterKernelFunctionName("SwkbdCalcSubThreadPredict__3RplFv", nn::swkbd::CalcSubThreadPredict);
   RegisterKernelFunctionName("SwkbdCalc__3RplFRCQ3_2nn5swkbd14ControllerInfo", nn::swkbd::Calc);
   RegisterKernelFunctionName("SwkbdConfirmUnfixAll__3RplFv", nn::swkbd::ConfirmUnfixAll);
   RegisterKernelFunctionName("SwkbdCreate__3RplFPUcQ3_2nn5swkbd10RegionTypeUiP8FSClient", nn::swkbd::Create);
   RegisterKernelFunctionName("SwkbdDestroy__3RplFv", nn::swkbd::Destroy);
   RegisterKernelFunctionName("SwkbdDisappearInputForm__3RplFv", nn::swkbd::DisappearInputForm);
   RegisterKernelFunctionName("SwkbdDisappearKeyboard__3RplFv", nn::swkbd::DisappearKeyboard);
   RegisterKernelFunctionName("SwkbdDrawDRC__3RplFv", nn::swkbd::DrawDRC);
   RegisterKernelFunctionName("SwkbdDrawTV__3RplFv", nn::swkbd::DrawTV);
   RegisterKernelFunctionName("SwkbdGetDrawStringInfo__3RplFPQ3_2nn5swkbd14DrawStringInfo", nn::swkbd::GetDrawStringInfo);
   RegisterKernelFunctionName("SwkbdGetInputFormString__3RplFv", nn::swkbd::GetInputFormString);
   RegisterKernelFunctionName("SwkbdGetKeyboardCondition__3RplFPQ3_2nn5swkbd17KeyboardCondition", nn::swkbd::GetKeyboardCondition);
   RegisterKernelFunctionName("SwkbdGetStateInputForm__3RplFv", nn::swkbd::GetStateInputForm);
   RegisterKernelFunctionName("SwkbdGetStateKeyboard__3RplFv", nn::swkbd::GetStateKeyboard);
   RegisterKernelFunctionName("SwkbdInactivateSelectCursor__3RplFv", nn::swkbd::InactivateSelectCursor);
   RegisterKernelFunctionName("SwkbdInitLearnDic__3RplFPv", nn::swkbd::InitLearnDic);
   RegisterKernelFunctionName("SwkbdIsCoveredWithSubWindow__3RplFv", nn::swkbd::IsCoveredWithSubWindow);
   RegisterKernelFunctionName("SwkbdIsDecideCancelButton__3RplFPb", nn::swkbd::IsDecideCancelButton);
   RegisterKernelFunctionName("SwkbdIsDecideOkButton__3RplFPb", nn::swkbd::IsDecideOkButton);
   RegisterKernelFunctionName("SwkbdIsKeyboardTarget__3RplFPQ3_2nn5swkbd14IEventReceiver", nn::swkbd::IsKeyboardTarget);
   RegisterKernelFunctionName("SwkbdIsNeedCalcSubThreadFont__3RplFv", nn::swkbd::IsNeedCalcSubThreadFont);
   RegisterKernelFunctionName("SwkbdIsNeedCalcSubThreadPredict__3RplFv", nn::swkbd::IsNeedCalcSubThreadPredict);
   RegisterKernelFunctionName("SwkbdIsSelectCursorActive__3RplFv", nn::swkbd::IsSelectCursorActive);
   RegisterKernelFunctionName("SwkbdMuteAllSound__3RplFb", nn::swkbd::MuteAllSound);
   RegisterKernelFunctionName("SwkbdSetControllerRemo__3RplFQ3_2nn5swkbd14ControllerType", nn::swkbd::SetControllerRemo);
   RegisterKernelFunctionName("SwkbdSetCursorPos__3RplFi", nn::swkbd::SetCursorPos);
   RegisterKernelFunctionName("SwkbdSetEnableOkButton__3RplFb", nn::swkbd::SetEnableOkButton);
   RegisterKernelFunctionName("SwkbdSetInputFormString__3RplFPCw", nn::swkbd::SetInputFormString);
   RegisterKernelFunctionName("SwkbdSetReceiver__3RplFRCQ3_2nn5swkbd11ReceiverArg", nn::swkbd::SetReceiver);
   RegisterKernelFunctionName("SwkbdSetSelectFrom__3RplFi", nn::swkbd::SetSelectFrom);
   RegisterKernelFunctionName("SwkbdSetUserControllerEventObj__3RplFPQ3_2nn5swkbd19IControllerEventObj", nn::swkbd::SetUserControllerEventObj);
   RegisterKernelFunctionName("SwkbdSetUserSoundObj__3RplFPQ3_2nn5swkbd9ISoundObj", nn::swkbd::SetUserSoundObj);
   RegisterKernelFunctionName("SwkbdSetVersion__3RplFi", nn::swkbd::SetVersion);
}

namespace internal
{

void
injectTextInput(const char *input)
{
   if (sKeyboardState == State::Visible) {
      sInputBuffer += input;
   }
}

void
injectKeyInput(decaf::input::KeyboardKey key,
               decaf::input::KeyboardAction action)
{
   if (sKeyboardState != State::Visible) {
      return;
   }

   if (action == decaf::input::KeyboardAction::Release) {
      if (key == decaf::input::KeyboardKey::Enter) {
         sOkButtonPressed = true;
         sInputFormState = State::Hidden;
         sKeyboardState = State::Hidden;
      } else if (key == decaf::input::KeyboardKey::Escape) {
         sCancelButtonPressed = true;
         sInputFormState = State::Hidden;
         sKeyboardState = State::Hidden;
      }
   }
}

} // namespace internal

} // namespace swkbd

} // namespace nn
