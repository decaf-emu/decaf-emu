#pragma once
#include <string>
#include "ppcinvokeargs.h"
#include "ppcinvokeresult.h"
#include "ppcinvokelog.h"
#include "common/log.h"
#include "common/type_list.h"
#include "cpu/trace.h"

namespace ppctypes
{

struct _argumentsState
{
   LogState log;
   cpu::Core *thread;
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

// Apply arguments
template<typename Head, typename... Tail>
inline void
applyArguments2(_argumentsState& state, Head head, Tail... tail)
{
   setArgument<Head>(state.thread, state.r, state.f, head);
   applyArguments2(state, tail...);
}

// Apply last argument
template<typename Last>
inline void
applyArguments2(_argumentsState& state, Last last)
{
   setArgument<Last>(state.thread, state.r, state.f, last);
}

// Apply host function call arguments to PowerPC registers
template<typename... Args>
inline size_t
applyArguments(cpu::Core *state, Args&&... args)
{
   _argumentsState argstate;
   argstate.thread = state;
   argstate.r = 3;
   argstate.f = 1;
   applyArguments2(argstate, std::forward<Args>(args)...);
   return argstate.r - 3;
}

typedef void LogFunc(const std::string&);

// Static function process normal arguments
template<LogFunc LogFn, typename FnReturnType, typename... FnArgs, typename Head, typename... Tail, typename... Args>
inline void
invoke2(_argumentsState& state, FnReturnType func(FnArgs...), type_list<Head, Tail...>, Args... values)
{
   auto value = getArgument<Head>(state.thread, state.r, state.f);

   if (LogFn != nullptr) {
      logArgument(state.log, value);
   }

   invoke2<LogFn>(state, func, type_list<Tail...>{}, values..., value);
}

// Static function process variable arguments
template<LogFunc LogFn, typename FnReturnType, typename... FnArgs, typename... Args>
inline void
invoke2(_argumentsState& state, FnReturnType func(FnArgs...), type_list<VarList&>, Args... values)
{
   VarList vargs(state);

   if (LogFn != nullptr) {
      logArgumentVargs(state.log);
   }

   invoke2<LogFn>(state, func, type_list<>{}, values..., vargs);
}

// Call a static function with return value
template<LogFunc LogFn, typename FnReturnType, typename... FnArgs, typename... Args>
inline void
invoke2(_argumentsState& state, FnReturnType func(FnArgs...), type_list<>, Args... args)
{
   if (LogFn != nullptr) {
      LogFn(logCallEnd(state.log));
   }

   auto result = func(args...);
   setResult<FnReturnType>(state.thread, result);
}

// Call a void static function
template<LogFunc LogFn, typename... FnArgs, typename... Args>
inline void
invoke2(_argumentsState& state, void func(FnArgs...), type_list<>, Args... args)
{
   if (LogFn != nullptr) {
      LogFn(logCallEnd(state.log));
   }

   func(args...);
}

// Call a static function from PPC
template<LogFunc LogFn, typename ReturnType, typename... Args>
inline void
invoke(cpu::Core *state, ReturnType (*func)(Args...), const std::string &name = "")
{
   _argumentsState argstate;
   argstate.thread = state;
   argstate.r = 3;
   argstate.f = 1;

   if (LogFn != nullptr) {
      logCall(argstate.log, state->lr, name);
   }

   invoke2<LogFn>(argstate, func, type_list<Args...> {});
}

// Member function process normal arguments
template<LogFunc LogFn, typename ObjectType, typename FnReturnType, typename... FnArgs, typename Head, typename... Tail, typename... Args>
inline void
invokeMemberFn2(_argumentsState& state, FnReturnType(ObjectType::*func)(FnArgs...), type_list<Head, Tail...>, Args... values)
{
   auto value = getArgument<Head>(state.thread, state.r, state.f);

   if (LogFn != nullptr) {
      logArgument(state.log, value);
   }

   invokeMemberFn2<LogFn>(state, func, type_list<Tail...>{}, values..., value);
}

// Member function process variable arguments
template<LogFunc LogFn, typename ObjectType, typename FnReturnType, typename... FnArgs, typename... Args>
inline void
invokeMemberFn2(_argumentsState& state, FnReturnType (ObjectType::*func)(FnArgs...), type_list<VarList&>, Args... values)
{
   VarList vargs(state);

   if (LogFn != nullptr) {
      logArgumentVargs(state.log);
   }

   invokeMemberFn2<LogFn>(state, func, type_list<>{}, values..., vargs);
}

// Call member function with return value
template<LogFunc LogFn, typename ObjectType, typename FnReturnType, typename... FnArgs, typename... Args>
inline void
invokeMemberFn2(_argumentsState& state, FnReturnType (ObjectType::*func)(FnArgs...), type_list<>, Args... args)
{
   if (LogFn != nullptr) {
      LogFn(logCallEnd(state.log));
   }

   auto object = reinterpret_cast<ObjectType *>(mem::translate(state.thread->gpr[3]));
   auto result = (object->*func)(args...);
   setResult<FnReturnType>(state.thread, result);
}

// Call void member function
template<LogFunc LogFn, typename ObjectType, typename... FnArgs, typename... Args>
inline void
invokeMemberFn2(_argumentsState& state, void (ObjectType::*func)(FnArgs...), type_list<>, Args... args)
{
   if (LogFn != nullptr) {
      LogFn(logCallEnd(state.log));
   }

   auto object = reinterpret_cast<ObjectType *>(mem::translate(state.thread->gpr[3]));
   (object->*func)(args...);
}

// Call a member function from PPC
template<LogFunc LogFn, typename ObjectType, typename ReturnType, typename... Args>
inline void
invokeMemberFn(cpu::Core *state, ReturnType (ObjectType::*func)(Args...), const std::string &name = "")
{
   // Start arguments from r4, as r3=this
   _argumentsState argstate;
   argstate.thread = state;
   argstate.r = 4;
   argstate.f = 1;

   if (LogFn != nullptr) {
      logCall(argstate.log, state->lr, name);
   }

   invokeMemberFn2<LogFn>(argstate, func, type_list<Args...> {});
}

} // namespace ppctypes
