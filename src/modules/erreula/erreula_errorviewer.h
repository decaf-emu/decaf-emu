#pragma once
#include "types.h"

struct FSClient;

namespace nn
{

namespace erreula
{

enum class ResultType
{
   // Unknown
};

enum class RegionType
{
   // Unknown
};

enum class LangType
{
   // Unknown
};

enum class ControllerType
{
   // Unknown
};

struct AppearArg
{
   // Unknown
};

struct ControllerInfo
{
   // Unknown
};

struct HomeNixSignArg
{
   // Unknown
};

enum class ErrorViewerState : uint32_t
{
   None
};

void
ErrEulaAppearError(nn::erreula::AppearArg *args);

void
ErrEulaCalc(nn::erreula::ControllerInfo *info);

void
ErrEulaCreate(uint8_t *unk1, nn::erreula::RegionType region, nn::erreula::LangType language, FSClient *fsClient);

void
ErrEulaDestroy();

void
ErrEulaDisappearError();

void
ErrEulaDrawDRC();

void
ErrEulaDrawTV();

ErrorViewerState
ErrEulaGetStateErrorViewer();

BOOL
ErrEulaIsDecideSelectButtonError();

BOOL
ErrEulaIsDecideSelectLeftButtonError();

BOOL
ErrEulaIsDecideSelectRightButtonError();

void
ErrEulaSetControllerRemo(nn::erreula::ControllerType type);

void
ErrEulaAppearHomeNixSign(nn::erreula::HomeNixSignArg *arg);

BOOL
ErrEulaIsAppearHomeNixSign();

void
ErrEulaDisappearHomeNixSign();

void
ErrEulaChangeLang(nn::erreula::LangType language);

BOOL
ErrEulaIsSelectCursorActive();

nn::erreula::ResultType
ErrEulaGetResultType();

uint32_t
ErrEulaGetResultCode();

uint32_t
ErrEulaGetSelectButtonNumError();

void
ErrEulaSetVersion(int version);

void
ErrEulaPlayAppearSE(bool value);

bool
ErrEulaJump(char const *, unsigned int);

} // namespace erreula

} // namespace nn
