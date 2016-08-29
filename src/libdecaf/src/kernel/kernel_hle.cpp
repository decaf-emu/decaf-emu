#include "kernel_hle.h"
#include "kernel_internal.h"
#include "modules/camera/camera.h"
#include "modules/coreinit/coreinit.h"
#include "modules/dmae/dmae.h"
#include "modules/erreula/erreula.h"
#include "modules/gx2/gx2.h"
#include "modules/mic/mic.h"
#include "modules/nn_ac/nn_ac.h"
#include "modules/nn_aoc/nn_aoc.h"
#include "modules/nn_acp/nn_acp.h"
#include "modules/nn_act/nn_act.h"
#include "modules/nn_boss/nn_boss.h"
#include "modules/nn_fp/nn_fp.h"
#include "modules/nn_ndm/nn_ndm.h"
#include "modules/nn_nfp/nn_nfp.h"
#include "modules/nn_olv/nn_olv.h"
#include "modules/nn_save/nn_save.h"
#include "modules/nn_temp/nn_temp.h"
#include "modules/nsysnet/nsysnet.h"
#include "modules/proc_ui/proc_ui.h"
#include "modules/padscore/padscore.h"
#include "modules/snd_core/snd_core.h"
#include "modules/swkbd/swkbd.h"
#include "modules/sysapp/sysapp.h"
#include "modules/vpad/vpad.h"
#include "modules/zlib125/zlib125.h"

namespace kernel
{

static std::map<std::string, HleModule*>
gHleModules;

static std::map<uint32_t, HleFunction*>
gHleFuncs;

static void
kcstub(cpu::Core *state, void *data)
{
   auto func = static_cast<HleFunction *>(data);

   if (!func->valid) {
      gLog->info("Unimplemented kernel function {}::{} called from 0x{:08X}", func->module, func->name, state->lr);
      return;
   }

   // Grab our core pointer
   auto core = cpu::this_core::state();

   // Save our original stack pointer for the backchain
   auto backchainSp = core->gpr[1];

   // Allocate callee backchain and lr space.
   core->gpr[1] -= 2 * 4;

   // Write the backchain pointer
   mem::write(core->gpr[1], backchainSp);

   // Call our target
   func->call(state);

   // Grab the most recent core state as it may have changed.
   core = cpu::this_core::state();

   // Release callee backchain and lr space.
   core->gpr[1] += 2 * 4;
}

void
registerHleFunc(HleFunction *func)
{
   func->syscallID = cpu::registerKernelCall({ kcstub, func });
   gHleFuncs[func->syscallID] = func;
}

uint32_t
registerUnimplementedHleFunc(const std::string &module, const std::string &name)
{
   auto ppcFn = new functions::HleFunctionImpl<void>();
   ppcFn->valid = false;
   ppcFn->module = module;
   ppcFn->name = name;
   ppcFn->wrapped_function = nullptr;
   registerHleFunc(ppcFn);
   return ppcFn->syscallID;
}

HleModule *
findHleModule(const std::string &name)
{
   auto itr = gHleModules.find(name);

   if (itr == gHleModules.end()) {
      return nullptr;
   } else {
      return itr->second;
   }
}

void
registerHleModule(const std::string &name, HleModule *module)
{
   // TODO: Handle if there is a collision
   gHleModules.emplace(name, module);

   auto symbols = module->getSymbols();

   for (auto &sym : symbols) {
      if (sym->type == HleSymbol::Function) {
         registerHleFunc(reinterpret_cast<HleFunction*>(sym));
      }
   }
}

void
registerHleModuleAlias(const std::string &module, const std::string &alias)
{
   auto itr = gHleModules.find(module);

   // TODO: Error if the module is missing!
   if (itr != gHleModules.end()) {
      gHleModules.emplace(alias, itr->second);
   }
}

void
initialiseHleMmodules()
{
   camera::Module::RegisterFunctions();
   coreinit::Module::RegisterFunctions();
   dmae::Module::RegisterFunctions();
   nn::erreula::Module::RegisterFunctions();
   gx2::Module::RegisterFunctions();
   mic::Module::RegisterFunctions();
   nn::ac::Module::RegisterFunctions();
   nn::aoc::Module::RegisterFunctions();
   nn::acp::Module::RegisterFunctions();
   nn::act::Module::RegisterFunctions();
   nn::boss::Module::RegisterFunctions();
   nn::fp::Module::RegisterFunctions();
   nn::ndm::Module::RegisterFunctions();
   nn::nfp::Module::RegisterFunctions();
   nn::olv::Module::RegisterFunctions();
   nn::save::Module::RegisterFunctions();
   nn::temp::Module::RegisterFunctions();
   nsysnet::Module::RegisterFunctions();
   padscore::Module::RegisterFunctions();
   proc_ui::Module::RegisterFunctions();
   snd_core::Module::RegisterFunctions();
   nn::swkbd::Module::RegisterFunctions();
   sysapp::Module::RegisterFunctions();
   vpad::Module::RegisterFunctions();
   zlib125::Module::RegisterFunctions();

   registerHleModule("camera.rpl", new camera::Module {});
   registerHleModule("coreinit.rpl", new coreinit::Module{});
   registerHleModule("dmae.rpl", new dmae::Module{});
   registerHleModule("erreula.rpl", new nn::erreula::Module{});
   registerHleModule("gx2.rpl", new gx2::Module{});
   registerHleModule("mic.rpl", new mic::Module{});
   registerHleModule("nn_ac.rpl", new nn::ac::Module{});
   registerHleModule("nn_aoc.rpl", new nn::aoc::Module {});
   registerHleModule("nn_acp.rpl", new nn::acp::Module{});
   registerHleModule("nn_act.rpl", new nn::act::Module{});
   registerHleModule("nn_boss.rpl", new nn::boss::Module{});
   registerHleModule("nn_fp.rpl", new nn::fp::Module{});
   registerHleModule("nn_nfp.rpl", new nn::nfp::Module{});
   registerHleModule("nn_ndm.rpl", new nn::ndm::Module{});
   registerHleModule("nn_olv.rpl", new nn::olv::Module{});
   registerHleModule("nn_save.rpl", new nn::save::Module{});
   registerHleModule("nn_temp.rpl", new nn::temp::Module{});
   registerHleModule("nsysnet.rpl", new nsysnet::Module{});
   registerHleModule("padscore.rpl", new padscore::Module{});
   registerHleModule("proc_ui.rpl", new proc_ui::Module{});
   registerHleModule("snd_core.rpl", new snd_core::Module{});
   registerHleModuleAlias("snd_core.rpl", "sndcore2.rpl");
   registerHleModule("swkbd.rpl", new nn::swkbd::Module{});
   registerHleModule("sysapp.rpl", new sysapp::Module{});
   registerHleModule("vpad.rpl", new vpad::Module{});
   registerHleModule("zlib125.rpl", new zlib125::Module{});
}

} // namespace kernel
