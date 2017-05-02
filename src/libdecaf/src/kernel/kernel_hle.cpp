#include "decaf_config.h"
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
#include "modules/nsyskbd/nsyskbd.h"
#include "modules/nsysnet/nsysnet.h"
#include "modules/proc_ui/proc_ui.h"
#include "modules/padscore/padscore.h"
#include "modules/snd_core/snd_core.h"
#include "modules/snd_user/snd_user.h"
#include "modules/swkbd/swkbd.h"
#include "modules/sysapp/sysapp.h"
#include "modules/vpad/vpad.h"
#include "modules/zlib125/zlib125.h"

namespace kernel
{

static std::map<std::string, HleModule*>
sHleModules;

static void
kcstub(cpu::Core *state, void *data)
{
   auto func = static_cast<HleFunction *>(data);

   if (!func->valid) {
      gLog->warn("Unimplemented kernel function {}::{} called from 0x{:08X}", func->module, func->name, state->lr);
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
}

uint32_t
registerUnimplementedHleFunc(const std::string &module,
                             const std::string &name)
{
   auto ppcFn = new functions::HleFunctionImpl<void>();
   ppcFn->valid = false;
   ppcFn->module = module;
   ppcFn->name = name;
   ppcFn->wrapped_function = nullptr;
   registerHleFunc(ppcFn);
   return ppcFn->syscallID;
}

void
registerHleModule(const std::string &name,
                  HleModule *module)
{
   if (std::find(decaf::config::system::lle_modules.begin(),
                 decaf::config::system::lle_modules.end(),
                 name) != decaf::config::system::lle_modules.end()) {
      gLog->warn("Replacing HLE with LLE for module {}.", name);
      return;
   }

   // Register module
   decaf_check(sHleModules.find(name) == sHleModules.end());
   sHleModules.emplace(name, module);

   // Register every function symbol
   auto &symbolMap = module->getSymbolMap();

   for (auto &pair : symbolMap) {
      auto symbol = pair.second;

      if (symbol->type == HleSymbol::Function) {
         registerHleFunc(reinterpret_cast<HleFunction *>(symbol));
      }
   }
}

void
registerHleModuleAlias(const std::string &module,
                       const std::string &alias)
{
   if (std::find(decaf::config::system::lle_modules.begin(),
                 decaf::config::system::lle_modules.end(),
                 alias) != decaf::config::system::lle_modules.end()) {
      gLog->warn("Replacing HLE with LLE for module {}.", alias);
      return;
   }

   auto itr = sHleModules.find(module);
   decaf_check(itr != sHleModules.end());
   sHleModules.emplace(alias, itr->second);
}

HleModule *
findHleModule(const std::string &name)
{
   auto itr = sHleModules.find(name);

   if (itr == sHleModules.end()) {
      return nullptr;
   } else {
      return itr->second;
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
   nsyskbd::Module::RegisterFunctions();
   nsysnet::Module::RegisterFunctions();
   padscore::Module::RegisterFunctions();
   proc_ui::Module::RegisterFunctions();
   snd_core::Module::RegisterFunctions();
   snd_user::Module::RegisterFunctions();
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
   registerHleModule("nsyskbd.rpl", new nsyskbd::Module {});
   registerHleModule("nsysnet.rpl", new nsysnet::Module{});
   registerHleModule("padscore.rpl", new padscore::Module{});
   registerHleModule("proc_ui.rpl", new proc_ui::Module{});
   registerHleModule("snd_core.rpl", new snd_core::Module{});
   registerHleModuleAlias("snd_core.rpl", "sndcore2.rpl");
   registerHleModule("snd_user.rpl", new snd_user::Module {});
   registerHleModule("swkbd.rpl", new nn::swkbd::Module{});
   registerHleModule("sysapp.rpl", new sysapp::Module{});
   registerHleModule("vpad.rpl", new vpad::Module{});
   registerHleModule("zlib125.rpl", new zlib125::Module{});
}

} // namespace kernel
