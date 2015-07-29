#pragma once
#include "ppcinvokeargs.h"
#include "ppcinvokeresult.h"
#include "ppcinvokelog.h"
#include "util.h"

namespace ppctypes
{

struct _argumentsState
{
   LogState log;
   ThreadState *thread;
   size_t r;
   size_t f;
};

template<typename Head, typename... Tail>
static inline void
applyArguments2(_argumentsState& state, Head head, Tail... tail)
{
   setArgument<Head>(state.thread, state.r, state.f, head);
   applyArguments2(state, tail...);
}

template<typename Last>
static inline void
applyArguments2(_argumentsState& state, Last last)
{
   setArgument<Last>(state.thread, state.r, state.f, last);
}

template<typename... Args>
static inline void
applyArguments(ThreadState *state, Args&&... args)
{
   _argumentsState argstate;
   argstate.thread = state;
   argstate.r = 3;
   argstate.f = 1;
   applyArguments2(argstate, std::forward<Args>(args)...);
}

template<typename FnReturnType, typename... FnArgs, typename Head, typename... Tail, typename... Args>
static inline void
invoke2(_argumentsState& state, FnReturnType func(FnArgs...), type_list<Head, Tail...>, Args... values)
{
   auto value = getArgument<Head>(state.thread, state.r, state.f);
   logArgument(state.log, value);
   invoke2(state, func, type_list<Tail...>{}, values..., value);
}

template<typename FnReturnType, typename... FnArgs, typename... Args>
static inline void
invoke2(_argumentsState& state, FnReturnType func(FnArgs...), type_list<>, Args... args)
{
   gLog->trace(logCallEnd(state.log));
   auto result = func(args...);
   setResult<FnReturnType>(state.thread, result);
}

template<typename... FnArgs, typename... Args>
static inline void
invoke2(_argumentsState& state, void func(FnArgs...), type_list<>, Args... args)
{
   gLog->trace(logCallEnd(state.log));
   func(args...);
}

template<typename ReturnType, typename... Args>
static inline void
invoke(ThreadState *state, ReturnType func(Args...), const char *name = nullptr)
{
   _argumentsState argstate;
   argstate.thread = state;
   argstate.r = 3;
   argstate.f = 1;
   logCall(argstate.log, state->lr, name);
   invoke2(argstate, func, type_list<Args...> {});
}

class VarList {
public:
   VarList(_argumentsState& state) : mState(state) { }

   template<typename Type>
   Type next() {
      return getArgument<Type>(mState.thread, mState.r, mState.f);
   }

protected:
   _argumentsState& mState;

};

template<typename ReturnType>
static inline void
invokeVariadic2(_argumentsState& state, ReturnType func(VarList& args))
{
   VarList args(state);
   auto res = func(args);
   setResult<FnReturnType>(state.thread, result);
}

template<>
static inline void
invokeVariadic2(_argumentsState& state, void func(VarList& args))
{
   VarList args(state);
   func(args);
}

template<typename ReturnType>
static inline void
invoke(ThreadState *state, ReturnType func(VarList& args), const char *name = nullptr)
{
   _argumentsState argstate;
   argstate.thread = state;
   argstate.r = 3;
   argstate.f = 1;
   logCall(argstate.log, state->lr, name);
   gLog->trace(logCallEnd(log));
   invokeVariadic2(argstate, func);
}

}
