#pragma once
#include "cafe_ppc_interface.h"

#include <libcpu/state.h>

namespace cafe
{

namespace detail
{

void
invoke_trace_host_impl(cpu::Core *core, const char *name, bool is_member_function, const RuntimeParamInfo *params, size_t numParams);

} // namespace detail

//! Trace log a host function call from a guest context
template<typename FunctionType>
void
invoke_trace(cpu::Core *core,
             const char *name)
{
   using func_traits = detail::function_traits<FunctionType>;
   invoke_trace_host_impl(core, name, func_traits::is_member_function, func_traits::runtime_param_info.data(), func_traits::runtime_param_info.size());
}

} // namespace cafe
