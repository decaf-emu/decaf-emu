#pragma once
#include "kernel/kernel_hlemodule.h"

/**
 * \defgroup gx2 gx2
 *
 * The graphics driver API for the latte chip in the Wii U.
 */

namespace gx2
{

class Module : public kernel::HleModuleImpl<Module>
{
public:
   virtual void initialise() override;

public:
   static void RegisterFunctions();

   static void RegisterGX2RResourceFunctions();
   static void RegisterVsyncFunctions();
};

} // namespace gx2
