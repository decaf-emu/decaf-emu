#pragma once
#include <libcpu/be2_struct.h>
#include <string_view>

namespace cafe::loader
{

struct RPL_STARTINFO;
struct LOADED_RPL;

namespace internal
{

int32_t
initialiseSharedHeaps();

virt_ptr<LOADED_RPL>
findLoadedSharedModule(std::string_view moduleName);

int32_t
LiInitSharedForProcess(virt_ptr<RPL_STARTINFO> initData);

} // namespace internal

} // namespace cafe::loader
