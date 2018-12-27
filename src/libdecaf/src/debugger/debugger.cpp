#include "debugger.h"
#include "debugger_server_gdb.h"
#include "decaf.h"

#include "decaf_debug_api.h"

namespace debugger
{

static GdbServer
sGdbServer { };

void
initialise()
{
   if (decaf::config::debugger::enabled
    && decaf::config::debugger::gdb_stub) {
      sGdbServer.start(decaf::config::debugger::gdb_stub_port);
   }
}

void
shutdown()
{
   // Force resume any paused cores.
   ::decaf::debug::resume();
}

void
draw(unsigned width, unsigned height)
{
   sGdbServer.process();
}

} // namespace debugger
