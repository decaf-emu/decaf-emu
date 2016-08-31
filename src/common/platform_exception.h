#pragma once
#include <functional>

namespace platform
{

struct Fiber;

struct Exception
{
   enum Type
   {
      AccessViolation = 1,
      InvalidInstruction = 2,
   };

   Exception(Type type_) :
      type(type_)
   {
   }

   Type type;
};

struct AccessViolationException : Exception
{
   AccessViolationException(uint64_t address_) :
      Exception(Exception::AccessViolation),
      address(address_)
   {
   }

   uint64_t address;
};

struct InvalidInstructionException : Exception
{
   InvalidInstructionException() :
      Exception(Exception::InvalidInstruction)
   {
   }

};

typedef void (*ExceptionResumeFunc)();
using ExceptionHandler = std::function<ExceptionResumeFunc(Exception *exception)>;

// Can be returned from ExceptionHandler to indicate to resume
// execute of current fiber.
static ExceptionResumeFunc const
HandledException = reinterpret_cast<ExceptionResumeFunc>(static_cast<uintptr_t>(-1));

static ExceptionResumeFunc const
UnhandledException = reinterpret_cast<ExceptionResumeFunc>(static_cast<uintptr_t>(0));

bool
installExceptionHandler(ExceptionHandler handler);

} // namespace platform
