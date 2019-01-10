#pragma once
#include "cafe_ppc_interface_invoke.h"
#include <fmt/format.h>

namespace cafe
{

namespace detail
{

void
logTraceMessage(const fmt::memory_buffer &entry);

template<int argIndex, typename ArgType, RegisterType regType, int regIndex>
inline void
logParam(fmt::memory_buffer &message,
         cpu::Core *core,
         param_info_t<ArgType, regType, regIndex> paramInfo)
{
   if (argIndex > 0) {
      fmt::format_to(message, ", ");
   }

   if constexpr (regType == RegisterType::VarArgs) {
      fmt::format_to(message, "...");
   } else {
      fmt::format_to(message, "{}", readParam(core, paramInfo));
   }
}

template<typename HostFunctionType, typename FunctionTraitsType, std::size_t... I>
void
invoke_trace_host_impl(cpu::Core *core,
                       const char *name,
                       FunctionTraitsType &&,
                       std::index_sequence<I...>)
{
   fmt::memory_buffer message;
   auto param_info = typename FunctionTraitsType::param_info { };
   fmt::format_to(message, "{}(", name);

   if constexpr (FunctionTraitsType::is_member_function) {
      fmt::format_to(message, "this = {}, ", readParam(core, typename FunctionTraitsType::object_info { }));
   }

   if constexpr (FunctionTraitsType::num_args > 0) {
      (logParam<I>(message, core, std::get<I>(param_info)), ...);
   }

   fmt::format_to(message, ") from 0x{:08X}", core->lr);
   logTraceMessage(message);
}

} // namespace detail

//! Trace log a host function call from a guest context
template<typename FunctionType>
void
invoke_trace(cpu::Core *core,
             const char *name)
{
   using func_traits = detail::function_traits<FunctionType>;
   detail::invoke_trace_host_impl<FunctionType>(
      core, name, func_traits { }, std::make_index_sequence<func_traits::num_args> {});
}

} // namespace cafe
