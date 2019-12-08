#pragma once
#include "cafe_ppc_interface_params.h"

#include <libcpu/cpu_control.h>

namespace cafe
{

namespace detail
{

// Because of the way the host invocations work, some of our functions
// are no-return but they run through here anyways, causing the compiler
// to become confused.  We disable those warnings for this section.
#ifdef _MSC_VER
#   pragma warning(push)
#   pragma warning(disable: 4702)
#endif

template<typename HostFunctionType, HostFunctionType HostFunc, typename FunctionTraitsType, std::size_t... I>
inline cpu::Core *
invoke_host_impl(cpu::Core *core,
                 FunctionTraitsType &&,
                 std::index_sequence<I...>)
{
   auto param_info = typename FunctionTraitsType::param_info { };

   if constexpr (FunctionTraitsType::has_return_value) {
      auto return_info = typename FunctionTraitsType::return_info { };

      if constexpr (FunctionTraitsType::is_member_function) {
         auto obj = readParam(core, typename FunctionTraitsType::object_info { });
         auto result = (obj.getRawPointer()->*HostFunc)(readParam(core, std::get<I>(param_info))...);
         core = cpu::this_core::state();
         writeParam(core, return_info, result);
      } else {
         auto result = HostFunc(readParam(core, std::get<I>(param_info))...);
         core = cpu::this_core::state();
         writeParam(core, return_info, result);
      }

      return core;
   } else {
      if constexpr (FunctionTraitsType::is_member_function) {
         auto obj = readParam(core, typename FunctionTraitsType::object_info { });
         (obj.getRawPointer()->*HostFunc)(readParam(core, std::get<I>(param_info))...);
      } else {
         HostFunc(readParam(core, std::get<I>(param_info))...);
      }

      // We must refresh our Core* as it may have changed during the kernel call
      return cpu::this_core::state();
   }
}

#ifdef _MSC_VER
#   pragma warning(pop)
#endif

} // namespace detail

// Invoke a host function from a guest context
template<typename FunctionType, FunctionType Func>
[[nodiscard]]
inline cpu::Core *
invoke(cpu::Core *core)
{
   using func_traits = detail::function_traits<FunctionType>;
   return detail::invoke_host_impl<FunctionType, Func>(core,
                                                       func_traits { },
                                                       std::make_index_sequence<func_traits::num_args> {});
}

} // namespace cafe
