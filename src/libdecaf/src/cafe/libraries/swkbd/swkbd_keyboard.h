#pragma once
#include "decaf_input.h"
#include "swkbd_enum.h"

#include <common/platform.h>
#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{
struct FSClient;
} // namespace cafe::coreinit

namespace cafe::kpad
{
struct KPADStatus;
} // namespace cafe::kpad

namespace cafe::vpad
{
struct VPADStatus;
} // namespace cafe::vpad

namespace cafe::swkbd
{

struct ConfigArg
{
   be2_val<LanguageType> languageType;
   be2_val<uint32_t> unk_0x04;
   be2_val<uint32_t> unk_0x08;
   be2_val<uint32_t> unk_0x0C;
   be2_val<uint32_t> unk_0x10;
   be2_val<int32_t> unk_0x14;
   UNKNOWN(0x9C - 0x18);
   be2_val<uint32_t> unk_0x9C;
   UNKNOWN(4);
   be2_val<int32_t> unk_0xA4;
};
CHECK_OFFSET(ConfigArg, 0x00, languageType);
CHECK_OFFSET(ConfigArg, 0x04, unk_0x04);
CHECK_OFFSET(ConfigArg, 0x08, unk_0x08);
CHECK_OFFSET(ConfigArg, 0x0C, unk_0x0C);
CHECK_OFFSET(ConfigArg, 0x10, unk_0x10);
CHECK_OFFSET(ConfigArg, 0x14, unk_0x14);
CHECK_OFFSET(ConfigArg, 0x9C, unk_0x9C);
CHECK_OFFSET(ConfigArg, 0xA4, unk_0xA4);
CHECK_SIZE(ConfigArg, 0xA8);

struct ReceiverArg
{
   be2_val<uint32_t> unk_0x00;
   be2_val<uint32_t> unk_0x04;
   be2_val<uint32_t> unk_0x08;
   be2_val<int32_t> unk_0x0C;
   be2_val<uint32_t> unk_0x10;
   be2_val<int32_t> unk_0x14;
};
CHECK_OFFSET(ReceiverArg, 0x00, unk_0x00);
CHECK_OFFSET(ReceiverArg, 0x04, unk_0x04);
CHECK_OFFSET(ReceiverArg, 0x08, unk_0x08);
CHECK_OFFSET(ReceiverArg, 0x0C, unk_0x0C);
CHECK_OFFSET(ReceiverArg, 0x10, unk_0x10);
CHECK_OFFSET(ReceiverArg, 0x14, unk_0x14);
CHECK_SIZE(ReceiverArg, 0x18);

struct KeyboardArg
{
   ConfigArg configArg;
   ReceiverArg receiverArg;
};
CHECK_SIZE(KeyboardArg, 0xC0);

struct InputFormArg
{
   be2_val<uint32_t> unk_0x00;
   be2_val<int32_t> unk_0x04;
   be2_val<uint32_t> unk_0x08;
   be2_val<uint32_t> unk_0x0C;
   be2_val<int32_t> maxTextLength;
   be2_val<uint32_t> unk_0x14;
   be2_val<uint32_t> unk_0x18;
   be2_val<bool> unk_0x1C;
   be2_val<bool> unk_0x1D;
   be2_val<bool> unk_0x1E;
   PADDING(1);
};
CHECK_OFFSET(InputFormArg, 0x00, unk_0x00);
CHECK_OFFSET(InputFormArg, 0x04, unk_0x04);
CHECK_OFFSET(InputFormArg, 0x08, unk_0x08);
CHECK_OFFSET(InputFormArg, 0x0C, unk_0x0C);
CHECK_OFFSET(InputFormArg, 0x10, maxTextLength);
CHECK_OFFSET(InputFormArg, 0x14, unk_0x14);
CHECK_OFFSET(InputFormArg, 0x18, unk_0x18);
CHECK_OFFSET(InputFormArg, 0x1C, unk_0x1C);
CHECK_OFFSET(InputFormArg, 0x1D, unk_0x1D);
CHECK_OFFSET(InputFormArg, 0x1E, unk_0x1E);
CHECK_SIZE(InputFormArg, 0x20);

struct AppearArg
{
   be2_struct<KeyboardArg> keyboardArg;
   be2_struct<InputFormArg> inputFormArg;
};
CHECK_OFFSET(AppearArg, 0x00, keyboardArg);
CHECK_OFFSET(AppearArg, 0xC0, inputFormArg);
CHECK_SIZE(AppearArg, 0xE0);

struct CreateArg
{
   be2_virt_ptr<void> workMemory;
   be2_val<RegionType> regionType;
   be2_val<uint32_t> unk_0x08;
   be2_virt_ptr<coreinit::FSClient> fsClient;
};
CHECK_OFFSET(CreateArg, 0x00, workMemory);
CHECK_OFFSET(CreateArg, 0x04, regionType);
CHECK_OFFSET(CreateArg, 0x08, unk_0x08);
CHECK_OFFSET(CreateArg, 0x0C, fsClient);
CHECK_SIZE(CreateArg, 0x10);

struct ControllerInfo
{
   be2_virt_ptr<vpad::VPADStatus> vpad;
   be2_array<virt_ptr<kpad::KPADStatus>, 4> kpad;
};
CHECK_OFFSET(ControllerInfo, 0x00, vpad);
CHECK_OFFSET(ControllerInfo, 0x04, kpad);
CHECK_SIZE(ControllerInfo, 0x14);

struct DrawStringInfo
{
   UNKNOWN(0x1C);
};
CHECK_SIZE(DrawStringInfo, 0x1C);

struct KeyboardCondition
{
   be2_val<uint32_t> unk_0x00;
   be2_val<uint32_t> unk_0x04;
};
CHECK_OFFSET(KeyboardCondition, 0x00, unk_0x00);
CHECK_OFFSET(KeyboardCondition, 0x04, unk_0x04);
CHECK_SIZE(KeyboardCondition, 0x8);

struct IEventReceiver;
struct IControllerEventObj;
struct ISoundObj;

#ifdef PLATFORM_WINDOWS
using swkbd_char_t = int16_t;
#else
using swkbd_char_t = char16_t;
#endif

bool
AppearInputForm(virt_ptr<const AppearArg> arg);

bool
AppearKeyboard(virt_ptr<const KeyboardArg> arg);

void
CalcSubThreadFont();

void
CalcSubThreadPredict();

void
Calc(virt_ptr<const ControllerInfo> info);

void
ConfirmUnfixAll();

void
Create(virt_ptr<unsigned char> buffer,
       RegionType regionType,
       unsigned int unk,
       virt_ptr<coreinit::FSClient> fsclient);

void
Destroy();

bool
DisappearInputForm();

bool
DisappearKeyboard();

void
DrawDRC();

void
DrawTV();

void
GetDrawStringInfo(virt_ptr<DrawStringInfo> info);

const virt_ptr<swkbd_char_t>
GetInputFormString();

void
GetKeyboardCondition(virt_ptr<KeyboardCondition> condition);

State
GetStateInputForm();

State
GetStateKeyboard();

void
InactivateSelectCursor();

bool
InitLearnDic(virt_ptr<void> dictionary);

bool
IsCoveredWithSubWindow();

bool
IsDecideCancelButton(virt_ptr<bool> outIsSelected);

bool
IsDecideOkButton(virt_ptr<bool> outIsSelected);

bool
IsKeyboardTarget(virt_ptr<const IEventReceiver> receiver);

bool
IsNeedCalcSubThreadFont();

bool
IsNeedCalcSubThreadPredict();

bool
IsSelectCursorActive();

void
MuteAllSound(bool mute);

void
SetControllerRemo(ControllerType type);

void
SetCursorPos(int32_t pos);

void
SetEnableOkButton(bool enable);

void
SetInputFormString(virt_ptr<const swkbd_char_t> str);

void
SetReceiver(virt_ptr<const ReceiverArg> arg);

void
SetSelectFrom(int32_t pos);

void
SetUserControllerEventObj(virt_ptr<IControllerEventObj> obj);

void
SetUserSoundObj(virt_ptr<ISoundObj> obj);

void
SetVersion(int32_t version);

namespace internal
{

void
injectTextInput(const char *input);

void
injectKeyInput(decaf::input::KeyboardKey key,
               decaf::input::KeyboardAction action);

} // namespace internal

} // namespace cafe::swkbd
