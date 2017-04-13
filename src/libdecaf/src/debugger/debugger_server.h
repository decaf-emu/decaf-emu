#pragma once

namespace debugger
{

class DebuggerServer
{
public:
   virtual ~DebuggerServer() = default;

   virtual bool start(int port) = 0;
   virtual void process() = 0;
};

} // namespace debugger
