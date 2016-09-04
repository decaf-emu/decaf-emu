#include "erreula.h"
#include "erreula_errorviewer.h"

namespace nn
{

namespace erreula
{

void
ErrEulaAppearError(nn::erreula::AppearArg *args)
{
   decaf_warn_stub();
}

void
ErrEulaCalc(nn::erreula::ControllerInfo *info)
{
   decaf_warn_stub();
}

BOOL
ErrEulaCreate(uint8_t *workMemory,
              nn::erreula::RegionType region,
              nn::erreula::LangType language,
              FSClient *fsClient)
{
   decaf_warn_stub();
   return TRUE;
}

void
ErrEulaDestroy()
{
   decaf_warn_stub();
}

void
ErrEulaDisappearError()
{
   decaf_warn_stub();
}

void
ErrEulaDrawDRC()
{
   decaf_warn_stub();
}

void
ErrEulaDrawTV()
{
   decaf_warn_stub();
}

ErrorViewerState
ErrEulaGetStateErrorViewer()
{
   decaf_warn_stub();

   return ErrorViewerState::None;
}

BOOL
ErrEulaIsDecideSelectButtonError()
{
   decaf_warn_stub();

   return FALSE;
}

BOOL
ErrEulaIsDecideSelectLeftButtonError()
{
   decaf_warn_stub();

   return FALSE;
}

BOOL
ErrEulaIsDecideSelectRightButtonError()
{
   decaf_warn_stub();

   return FALSE;
}

void
ErrEulaSetControllerRemo(nn::erreula::ControllerType type)
{
   decaf_warn_stub();
}

void
ErrEulaAppearHomeNixSign(nn::erreula::HomeNixSignArg *arg)
{
   decaf_warn_stub();
}

BOOL
ErrEulaIsAppearHomeNixSign()
{
   decaf_warn_stub();

   return FALSE;
}

void
ErrEulaDisappearHomeNixSign()
{
   decaf_warn_stub();
}

void
ErrEulaChangeLang(nn::erreula::LangType language)
{
   decaf_warn_stub();
}

BOOL
ErrEulaIsSelectCursorActive()
{
   decaf_warn_stub();

   return FALSE;
}

nn::erreula::ResultType
ErrEulaGetResultType()
{
   decaf_warn_stub();

   return static_cast<nn::erreula::ResultType>(1);
}

uint32_t
ErrEulaGetResultCode()
{
   decaf_warn_stub();

   return 0;
}

uint32_t
ErrEulaGetSelectButtonNumError()
{
   decaf_warn_stub();

   return 0;
}

void
ErrEulaSetVersion(int version)
{
   decaf_warn_stub();
}

void
ErrEulaPlayAppearSE(bool value)
{
   decaf_warn_stub();
}

bool
ErrEulaJump(char const *,
            unsigned int)
{
   decaf_warn_stub();
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
