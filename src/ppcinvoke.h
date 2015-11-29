#pragma once
#include <string>
#include "config.h"
#include "ppcinvokeargs.h"
#include "ppcinvokeresult.h"
#include "ppcinvokelog.h"
#include "utils/log.h"
#include "utils/type_list.h"
#include "cpu/trace.h"

namespace ppctypes
{

struct _argumentsState
{
   LogState log;
   ThreadState *thread;
   size_t r;
   size_t f;
};

class VarList
{
public:
   VarList(_argumentsState& state) :
      mState(state)
   {
   }

   template<typename Type>
   Type next()
   {
      return getArgument<Type>(mState.thread, mState.r, mState.f);
   }

protected:
   _argumentsState& mState;
};

template<typename Head, typename... Tail>
inline void
applyArguments2(_argumentsState& state, Head head, Tail... tail)
{
   setArgument<Head>(state.thread, state.r, state.f, head);
   applyArguments2(state, tail...);
}

template<typename Last>
inline void
applyArguments2(_argumentsState& state, Last last)
{
   setArgument<Last>(state.thread, state.r, state.f, last);
}

template<typename... Args>
inline void
applyArguments(ThreadState *state, Args&&... args)
{
   _argumentsState argstate;
   argstate.thread = state;
   argstate.r = 3;
   argstate.f = 1;
   applyArguments2(argstate, std::forward<Args>(args)...);
}

template<typename FnReturnType, typename... FnArgs, typename Head, typename... Tail, typename... Args>
inline void
invoke2(_argumentsState& state, FnReturnType func(FnArgs...), type_list<Head, Tail...>, Args... values)
{
   auto value = getArgument<Head>(state.thread, state.r, state.f);

   if (config::log::kernel_trace) {
      logArgument(state.log, value);
   }

   invoke2(state, func, type_list<Tail...>{}, values..., value);
}

template<typename FnReturnType, typename... FnArgs, typename... Args>
inline void
invoke2(_argumentsState& state, FnReturnType func(FnArgs...), type_list<VarList&>, Args... values)
{
   VarList vargs(state);

   if (config::log::kernel_trace) {
      logArgumentVargs(state.log);
   }

   invoke2(state, func, type_list<>{}, values..., vargs);
}

inline void
invokeEndLog(_argumentsState& state)
{
   auto logData = logCallEnd(state.log);
   traceLogSyscall(logData);
   gLog->debug(logData);
}

template<typename FnReturnType, typename... FnArgs, typename... Args>
inline void
invoke2(_argumentsState& state, FnReturnType func(FnArgs...), type_list<>, Args... args)
{
   if (config::log::kernel_trace) {
      invokeEndLog(state);
   }

   auto result = func(args...);
   setResult<FnReturnType>(state.thread, result);
}

template<typename... FnArgs, typename... Args>
inline void
invoke2(_argumentsState& state, void func(FnArgs...), type_list<>, Args... args)
{
   if (config::log::kernel_trace) {
      invokeEndLog(state);
   }

   func(args...);
}

template<typename ReturnType, typename... Args>
inline void
invoke(ThreadState *state, ReturnType func(Args...), const std::string &name = "")
{
   _argumentsState argstate;
   argstate.thread = state;
   argstate.r = 3;
   argstate.f = 1;

   if (config::log::kernel_trace) {
      logCall(argstate.log, state->lr, name);
   }

   invoke2(argstate, func, type_list<Args...> {});
}

} // namespace ppctypes
