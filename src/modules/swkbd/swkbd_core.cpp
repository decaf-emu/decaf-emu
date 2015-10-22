#include "swkbd.h"
#include "swkbd_core.h"

namespace nn
{

namespace swkbd
{

bool AppearInputForm(/*something*/)
{
   gLog->info("nn::swkbd::AppearInputForm");
   return true;
}

bool AppearKeyboard(/*something*/)
{
   gLog->info("nn::swkbd::AppearKeyboard");
   return true;
}

void Calc(/*something*/)
{
   gLog->info("nn::swkbd::Calc");
}

void CalcSubThreadFont()
{
   gLog->info("nn::swkbd::CalcSubThreadFont");
}

void CalcSubThreadPredict()
{
   gLog->info("nn::swkbd::CalcSubThreadPredict");
}

void ConfirmUnfixAll()
{
   gLog->info("nn::swkbd::ConfirmUnfixAll");
}

bool Create(/*something*/)
{
   gLog->info("nn::swkbd::Create");
   return true;
}

void Destroy()
{
   gLog->info("nn::swkbd::Destroy");
}

bool DisappearInputForm()
{
   gLog->info("nn::swkbd::DisappearInputForm");
   return true;
}

bool DisappearKeyboard()
{
   gLog->info("nn::swkbd::DisappearKeyboard");
   return true;
}

void DrawDRC()
{
   gLog->info("nn::swkbd::DrawDRC");
}

void DrawTV()
{
   gLog->info("nn::swkbd::DrawTV");
}

void GetDrawStringInfo(void*)
{
   gLog->info("nn::swkbd::GetDrawStringInfo");
}

const wchar_t* GetInputFormString()
{
   gLog->info("nn::swkbd::GetInputFormString");
   return nullptr;
}

void GetKeyboardCondition(void*)
{
   gLog->info("nn::swkbd::GetKeyboardCondition");
}

State::Mode GetStateInputForm()
{
   gLog->info("nn::swkbd::GetStateInputForm");
   return (State::Mode)0;
}

State::Mode GetStateKeyboard()
{
   gLog->info("nn::swkbd::GetStateKeyboard");
   return static_cast<State::Mode>(0);
}

uint32_t GetWorkMemorySize(uint32_t)
{
   gLog->info("nn::swkbd::GetWorkMemorySize");
   return 1;
}

void InactivateSelectCursor()
{
   gLog->info("nn::swkbd::InactivateSelectCursor");
}

bool InitLearnDic(void*)
{
   gLog->info("nn::swkbd::InitLearnDic");
   return true;
}

bool IsCoveredWithSubWindow()
{
   gLog->info("nn::swkbd::IsCoveredWithSubWindow");
   return false;
}

bool IsDecideCancelButton(bool*)
{
   gLog->info("nn::swkbd::IsDecideCancelButton");
   return false;
}

bool IsDecideOkButton(bool*)
{
   gLog->info("nn::swkbd::IsDecideOkButton");
   return false;
}

bool IsKeyboardTarget(void*)
{
   gLog->info("nn::swkbd::IsKeyboardTarget");
   return false;
}

bool IsNeedCalcSubThreadFont()
{
   gLog->info("nn::swkbd::IsNeedCalcSubThreadFont");
   return false;
}

bool IsNeedCalcSubThreadPredict()
{
   gLog->info("nn::swkbd::IsNeedCalcSubThreadPredict");
   return false;
}

bool IsSelectCursorActive()
{
   gLog->info("nn::swkbd::IsSelectCursorActive");
   return true;
}

void MuteAllSound(bool shouldMute)
{
   gLog->info("nn::swkbd::MuteAllSound({})", shouldMute);
}

void SetControllerRemo(ControllerType::Type controllerType)
{
   gLog->info("nn::swkbd::SetControllerRemo({})", static_cast<uint32_t>(controllerType));
}

void SetCursorPos(int32_t pos)
{
   gLog->info("nn::swkbd::SetCursorPos({})", pos);
}

void SetEnableOkButton(bool enabled)
{
   gLog->info("nn::swkbd::SetEnableOkButton({})", enabled);
}

void SetInputFormString(const wchar_t *value)
{
   gLog->info("nn::swkbd::SetInputFormString");
}

void SetReceiver()
{
   gLog->info("nn::swkbd::SetReceiver");
}

void SetSelectFrom(int32_t)
{
   gLog->info("nn::swkbd::SetSelectFrom");
}

void SetUserControllerEventObj(void*)
{
   gLog->info("nn::swkbd::SetUserControllerEventObj");
}

void SetUserSoundObj(void*)
{
   gLog->info("nn::swkbd::SetUserSoundObj");
}

void SetVersion(int version)
{
   gLog->info("nn::swkbd::SetVersion");
}

}

}

void
Swkbd::registerCoreFunctions()
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
   RegisterKernelFunctionName("SwkbdInitLearnDic__3RplFPv", nn::swkbd::InitLearnDic);
   RegisterKernelFunctionName("SwkbdGetStateInputForm__3RplFv", nn::swkbd::GetStateInputForm);
   RegisterKernelFunctionName("SwkbdGetStateKeyboard__3RplFv", nn::swkbd::GetStateKeyboard);
   RegisterKernelFunctionName("SwkbdIsCoveredWithSubWindow__3RplFv", nn::swkbd::IsCoveredWithSubWindow);
   RegisterKernelFunctionName("SwkbdIsDecideCancelButton__3RplFPb", nn::swkbd::IsDecideCancelButton);
   RegisterKernelFunctionName("SwkbdIsDecideOkButton__3RplFPb", nn::swkbd::IsDecideOkButton);
   RegisterKernelFunctionName("SwkbdIsKeyboardTarget__3RplFPQ3_2nn5swkbd14IEventReceiver", nn::swkbd::IsKeyboardTarget);
   RegisterKernelFunctionName("SwkbdIsNeedCalcSubThreadFont__3RplFv", nn::swkbd::IsNeedCalcSubThreadFont);
   RegisterKernelFunctionName("SwkbdIsNeedCalcSubThreadPredict__3RplFv", nn::swkbd::IsNeedCalcSubThreadPredict);
   RegisterKernelFunctionName("SwkbdIsSelectCursorActive__3RplFv", nn::swkbd::IsSelectCursorActive);
   RegisterKernelFunctionName("SwkbdInactivateSelectCursor__3RplFv", nn::swkbd::InactivateSelectCursor);
   RegisterKernelFunctionName("SwkbdSetControllerRemo__3RplFQ3_2nn5swkbd14ControllerType", nn::swkbd::SetControllerRemo);
   RegisterKernelFunctionName("SwkbdSetCursorPos__3RplFi", nn::swkbd::SetCursorPos);
   RegisterKernelFunctionName("SwkbdSetEnableOkButton__3RplFb", nn::swkbd::SetEnableOkButton);
   RegisterKernelFunctionName("SwkbdSetReceiver__3RplFRCQ3_2nn5swkbd11ReceiverArg", nn::swkbd::SetReceiver);
   RegisterKernelFunctionName("SwkbdSetSelectFrom__3RplFi", nn::swkbd::SetSelectFrom);
   RegisterKernelFunctionName("SwkbdSetUserControllerEventObj__3RplFPQ3_2nn5swkbd19IControllerEventObj", nn::swkbd::SetUserControllerEventObj);
   RegisterKernelFunctionName("SwkbdSetUserSoundObj__3RplFPQ3_2nn5swkbd9ISoundObj", nn::swkbd::SetUserSoundObj);
   RegisterKernelFunctionName("SwkbdSetInputFormString__3RplFPCw", nn::swkbd::SetInputFormString);
   RegisterKernelFunctionName("SwkbdMuteAllSound__3RplFb", nn::swkbd::MuteAllSound);
   RegisterKernelFunctionName("SwkbdSetVersion__3RplFi", nn::swkbd::SetVersion);
}
