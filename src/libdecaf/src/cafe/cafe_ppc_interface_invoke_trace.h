#pragma once
#include "cafe_ppc_interface_invoke.h"

#include <common/log.h>
#include <fmt/format.h>

namespace cafe
{

namespace detail
{

template<typename ArgType, RegisterType regType, auto regIndex>
inline void
logParam(fmt::MemoryWriter &message,
         cpu::Core *core,
         param_info_t<ArgType, regType, regIndex> paramInfo)
{
   if constexpr (regType == RegisterType::VarArgs) {
      message.write("...");
   } else {
      auto value = readParam(core, paramInfo);
      if constexpr ((regType == RegisterType::Gpr32 && regIndex == 3) ||
                    (regType == RegisterType::Gpr64 && regIndex == 3) ||
                    (regType == RegisterType::Fpr && regIndex == 0)) {
         message.write("{}", value);
      } else {
         message.write(", {}", value);
      }
   }
}

template<typename HostFunctionType, typename FunctionTraitsType, std::size_t... I>
constexpr void
invoke_trace_host_impl(cpu::Core *core,
                       HostFunctionType &&,
                       const char *name,
                       FunctionTraitsType &&,
                       std::index_sequence<I...>)
{
   fmt::MemoryWriter message;
   auto param_info = typename FunctionTraitsType::param_info { };
   message.write("{}(", name);

   if constexpr (FunctionTraitsType::is_member_function) {
      message.write("this = {}, ", readParam(core, typename FunctionTraitsType::object_info { }));
   }

   if constexpr (FunctionTraitsType::num_args > 0) {
      (logParam(message, core, std::get<I>(param_info)), ...);
   }

   message.write(") from 0x{:08X}", core->lr);
   gLog->debug(message.str());
}

} // namespace detail

//! Trace log a host function call from a guest context
template<typename FunctionType>
void
invoke_trace(cpu::Core *core,
             FunctionType fn,
             const char *name)
{
   using func_traits = detail::function_traits<FunctionType>;
   invoke_trace_host_impl(core,
                          fn, name,
                          func_traits { },
                          std::make_index_sequence<func_traits::num_args> {});
}

} // namespace cafe
