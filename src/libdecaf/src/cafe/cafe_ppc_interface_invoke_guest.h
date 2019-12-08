#pragma once
#include "cafe_ppc_interface_params.h"

#include <libcpu/cpu_control.h>

namespace cafe
{

namespace detail
{

template<typename FunctionTraitsType, std::size_t... I, typename... ArgTypes>
inline decltype(auto)
invoke_guest_impl(cpu::Core *core,
                  cpu::VirtualAddress address,
                  FunctionTraitsType &&,
                  std::index_sequence<I...>,
                  const ArgTypes &... args)
{
   if constexpr (FunctionTraitsType::num_args > 0) {
      // Write arguments to registers
      auto param_info = typename FunctionTraitsType::param_info { };
      (writeParam(core, std::get<I>(param_info), args), ...);
   }

   // Write stack frame back chain
   auto frame = virt_cast<uint32_t *>(virt_addr{ core->gpr[1] - 8 });
   frame[0] = core->systemCallStackHead;
   frame[1] = core->lr;
   core->gpr[1] -= 8;

   // Save state
   auto cia = core->cia;
   auto nia = core->nia;

   // Set state to our function to call
   core->cia = 0;
   core->nia = address.getAddress();

   // Start executing!
   cpu::this_core::executeSub();

   // Grab the most recent core state as it may have changed.
   core = cpu::this_core::state();

   // Restore state
   core->cia = cia;
   core->nia = nia;

   // Restore stack
   core->gpr[1] += 8;

   // Return the result
   if constexpr (FunctionTraitsType::has_return_value) {
      return readParam(core, typename FunctionTraitsType::return_info { });
   }
}

} // namespace detail

// Invoke a guest function from a host context
template<typename FunctionType, typename... ArgTypes>
inline decltype(auto)
invoke(cpu::Core *core,
       cpu::FunctionPointer<cpu::VirtualAddress, FunctionType> fn,
       const ArgTypes &... args)
{
   using func_traits = detail::function_traits<FunctionType>;
   return detail::invoke_guest_impl(core,
                                    fn.getAddress(),
                                    func_traits { },
                                    std::make_index_sequence<func_traits::num_args> {},
                                    args...);
}

// Invoke a be2_val guest function from a host context
template<typename FunctionType, typename... ArgTypes>
inline decltype(auto)
invoke(cpu::Core *core,
       be2_val<cpu::FunctionPointer<cpu::VirtualAddress, FunctionType>> fn,
       const ArgTypes &... args)
{
   using func_traits = detail::function_traits<FunctionType>;
   return detail::invoke_guest_impl(core,
                                    fn.getAddress(),
                                    func_traits { },
                                    std::make_index_sequence<func_traits::num_args> {},
                                    args...);
}

} // namespace cafe
