#include "erreula.h"
#include "erreula_errorviewer.h"

namespace nn
{

namespace erreula
{

void
ErrEulaAppearError(nn::erreula::AppearArg *args)
{
}

void
ErrEulaCalc(nn::erreula::ControllerInfo *info)
{
}

void
ErrEulaCreate(uint8_t *unk1, nn::erreula::RegionType region, nn::erreula::LangType language, FSClient *fsClient)
{
}

void
ErrEulaDestroy()
{
}

void
ErrEulaDisappearError()
{
}

void
ErrEulaDrawDRC()
{
}

void
ErrEulaDrawTV()
{
}

ErrorViewerState
ErrEulaGetStateErrorViewer()
{
   return ErrorViewerState::None;
}

BOOL
ErrEulaIsDecideSelectButtonError()
{
   return FALSE;
}

BOOL
ErrEulaIsDecideSelectLeftButtonError()
{
   return FALSE;
}

BOOL
ErrEulaIsDecideSelectRightButtonError()
{
   return FALSE;
}

void
ErrEulaSetControllerRemo(nn::erreula::ControllerType type)
{
}

void
ErrEulaAppearHomeNixSign(nn::erreula::HomeNixSignArg *arg)
{
}

BOOL
ErrEulaIsAppearHomeNixSign()
{
   return FALSE;
}

void
ErrEulaDisappearHomeNixSign()
{
}

void
ErrEulaChangeLang(nn::erreula::LangType language)
{
}

BOOL
ErrEulaIsSelectCursorActive()
{
   return FALSE;
}

nn::erreula::ResultType
ErrEulaGetResultType()
{
   return static_cast<nn::erreula::ResultType>(1);
}

uint32_t
ErrEulaGetResultCode()
{
   return 0;
}

uint32_t
ErrEulaGetSelectButtonNumError()
{
   return 0;
}

void
ErrEulaSetVersion(int version)
{
}

void
ErrEulaPlayAppearSE(bool value)
{
}

bool
ErrEulaJump(char const *, unsigned int)
{
   return false;
}

void Module::registerErrorViewerFunctions()
{
   RegisterKernelFunctionName("ErrEulaAppearError__3RplFRCQ3_2nn7erreula9AppearArg", ErrEulaAppearError);
   RegisterKernelFunctionName("ErrEulaCalc__3RplFRCQ3_2nn7erreula14ControllerInfo", ErrEulaCalc);
   RegisterKernelFunctionName("ErrEulaCreate__3RplFPUcQ3_2nn7erreula10RegionTypeQ3_2nn7erreula8LangTypeP8FSClient", ErrEulaCreate);
   RegisterKernelFunctionName("ErrEulaDestroy__3RplFv", ErrEulaDestroy);
   RegisterKernelFunctionName("ErrEulaDisappearError__3RplFv", ErrEulaDisappearError);
   RegisterKernelFunctionName("ErrEulaDrawDRC__3RplFv", ErrEulaDrawDRC);
   RegisterKernelFunctionName("ErrEulaDrawTV__3RplFv", ErrEulaDrawTV);
   RegisterKernelFunctionName("ErrEulaGetStateErrorViewer__3RplFv", ErrEulaGetStateErrorViewer);
   RegisterKernelFunctionName("ErrEulaIsDecideSelectButtonError__3RplFv", ErrEulaIsDecideSelectButtonError);
   RegisterKernelFunctionName("ErrEulaIsDecideSelectLeftButtonError__3RplFv", ErrEulaIsDecideSelectLeftButtonError);
   RegisterKernelFunctionName("ErrEulaIsDecideSelectRightButtonError__3RplFv", ErrEulaIsDecideSelectRightButtonError);
   RegisterKernelFunctionName("ErrEulaSetControllerRemo__3RplFQ3_2nn7erreula14ControllerType", ErrEulaSetControllerRemo);
   RegisterKernelFunctionName("ErrEulaAppearHomeNixSign__3RplFRCQ3_2nn7erreula14HomeNixSignArg", ErrEulaAppearHomeNixSign);
   RegisterKernelFunctionName("ErrEulaIsAppearHomeNixSign__3RplFv", ErrEulaIsAppearHomeNixSign);
   RegisterKernelFunctionName("ErrEulaDisappearHomeNixSign__3RplFv", ErrEulaDisappearHomeNixSign);
   RegisterKernelFunctionName("ErrEulaChangeLang__3RplFQ3_2nn7erreula8LangType", ErrEulaChangeLang);
   RegisterKernelFunctionName("ErrEulaIsSelectCursorActive__3RplFv", ErrEulaIsSelectCursorActive);
   RegisterKernelFunctionName("ErrEulaGetResultType__3RplFv", ErrEulaGetResultType);
   RegisterKernelFunctionName("ErrEulaGetResultCode__3RplFv", ErrEulaGetResultCode);
   RegisterKernelFunctionName("ErrEulaGetSelectButtonNumError__3RplFv", ErrEulaGetSelectButtonNumError);
   RegisterKernelFunctionName("ErrEulaSetVersion__3RplFi", ErrEulaSetVersion);
   RegisterKernelFunctionName("ErrEulaPlayAppearSE__3RplFb", ErrEulaPlayAppearSE);
   RegisterKernelFunctionName("ErrEulaJump__3RplFPCcUi", ErrEulaJump);
}

} // namespace erreula

} // namespace nn
