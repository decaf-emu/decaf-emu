#pragma once
#include "cafe_hle_library_data.h"
#include "cafe_hle_library_function.h"
#include "cafe_hle_library_typeinfo.h"

#include <common/decaf_assert.h>
#include <libcpu/cpu.h>
#include <map>
#include <string>
#include <vector>

#define fnptr_decltype(Func) \
    std::conditional<std::is_function<decltype(Func)>::value, std::add_pointer<decltype(Func)>::type, decltype(Func)>::type

#define RegisterEntryPoint(fn) \
   registerFunctionExport<fnptr_decltype(fn), fn>("rpl_entry")

#define RegisterFunctionExport(fn) \
   registerFunctionExport<fnptr_decltype(fn), fn>(#fn)

#define RegisterFunctionExportName(name, fn) \
   registerFunctionExport<fnptr_decltype(fn), fn>(name)

#define RegisterDataExport(data) \
   registerDataExport(#data, data)

#define RegisterDataExportName(name, data) \
   registerDataExport(name, data)

#define RegisterFunctionInternal(fn, ptr) \
   registerFunctionInternal<fnptr_decltype(fn), fn>("__internal__" # fn, ptr)

#define RegisterDataInternal(data) \
   registerDataInternal("__internal__" # data, data)

namespace cafe::hle
{

enum class LibraryId
{
   avm,
   camera,
   coreinit,
   dc,
   dmae,
   drmapp,
   erreula,
   gx2,
   h264,
   lzma920,
   mic,
   nfc,
   nio_prof,
   nlibcurl,
   nlibnss2,
   nlibnss,
   nn_acp,
   nn_ac,
   nn_act,
   nn_aoc,
   nn_boss,
   nn_ccr,
   nn_cmpt,
   nn_dlp,
   nn_ec,
   nn_fp,
   nn_hai,
   nn_hpad,
   nn_idbe,
   nn_ndm,
   nn_nets2,
   nn_nfp,
   nn_nim,
   nn_olv,
   nn_pdm,
   nn_save,
   nn_sl,
   nn_spm,
   nn_temp,
   nn_uds,
   nn_vctl,
   nsysccr,
   nsyshid,
   nsyskbd,
   nsysnet,
   nsysuhs,
   nsysuvd,
   ntag,
   padscore,
   proc_ui,
   sndcore2,
   snd_core,
   snduser2,
   snd_user,
   swkbd,
   sysapp,
   tcl,
   tve,
   uac,
   uac_rpl,
   usb_mic,
   uvc,
   uvd,
   vpadbase,
   vpad,
   zlib125,
   Max,
};

class Library
{
public:
   // We have 3 default suymbols: NULL, .text, .data
   static constexpr auto BaseSymbolIndex = uint32_t { 3 };

   static cpu::Core *
   handleUnknownSystemCall(cpu::Core *state,
                           uint32_t id);

public:
   Library(LibraryId id, std::string name) :
      mID(id), mName(std::move(name))
   {
   }

   virtual ~Library() = default;

   LibraryId
   id() const
   {
      return mID;
   }

   const std::string &
   name() const
   {
      return mName;
   }

   virt_addr
   findSymbolAddress(std::string_view name) const
   {
      auto itr = mSymbolMap.find(name);
      if (itr == mSymbolMap.end()) {
         return virt_addr { 0 };
      }

      return itr->second->address;
   }

   LibrarySymbol *
   findSymbol(std::string_view name) const
   {
      auto itr = mSymbolMap.find(name);
      if (itr == mSymbolMap.end()) {
         return nullptr;
      }

      return itr->second.get();
   }

   LibrarySymbol *
   findSymbol(virt_addr addr) const
   {
      for (auto &[name, symbol] : mSymbolMap) {
         if (symbol->address == addr) {
            return symbol.get();
         }
      }

      return nullptr;
   }

   const std::vector<uint8_t> &
   getGeneratedRpl() const
   {
      return mGeneratedRpl;
   }

   void
   generate()
   {
      registerSymbols();
      registerSystemCalls();
      generateRpl();
   }

   void
   relocate(virt_addr textBaseAddress,
            virt_addr dataBaseAddress);

   void
   addUnimplementedFunctionExport(UnimplementedLibraryFunction *unimpl)
   {
      mUnimplementedFunctionExports.push_back(unimpl);
   }

   UnimplementedLibraryFunction *
   findUnimplementedFunctionExport(std::string_view name)
   {
      for (auto unimpl : mUnimplementedFunctionExports) {
         if (unimpl->name == name) {
            return unimpl;
         }
      }

      return nullptr;
   }

   const auto &
   getSymbolMap() const
   {
      return mSymbolMap;
   }

protected:
   virtual void
   registerSymbols() = 0;

   void
   registerSystemCalls();

   void
   generateRpl();

   template<typename FunctionType, FunctionType Fn>
   void
   registerFunctionInternal(const char *name,
                            virt_func_ptr<typename std::remove_pointer<FunctionType>::type> &hostPtr)
   {
      auto symbol = internal::makeLibraryFunction<FunctionType, Fn>(name);
      symbol->exported = false;
      symbol->hostPtr = reinterpret_cast<virt_ptr<void> *>(&hostPtr);
      registerSymbol(name, std::move(symbol));
   }

   template<typename FunctionType, FunctionType Fn>
   void
   registerFunctionExport(const char *name)
   {
      auto symbol = internal::makeLibraryFunction<FunctionType, Fn>(name);
      symbol->exported = true;
      registerSymbol(name, std::move(symbol));
   }

   template<typename DataType>
   void
   registerDataInternal(const char *name,
                        virt_ptr<DataType> &data)
   {
      auto symbol = std::make_unique<LibraryData>();
      symbol->exported = false;
      symbol->hostPointer = reinterpret_cast<virt_ptr<void> *>(&data);
      symbol->constructor = [](void *ptr) { new (ptr) DataType(); };
      symbol->size = sizeof(DataType);
      symbol->align = alignof(DataType);
      registerSymbol(name, std::move(symbol));
   }

   template<typename DataType>
   void
   registerDataExport(const char *name,
                      virt_ptr<DataType> &data)
   {
      auto symbol = std::make_unique<LibraryData>();
      symbol->exported = true;
      symbol->hostPointer = reinterpret_cast<virt_ptr<void> *>(&data);
      symbol->constructor = [](void *ptr) { new (ptr) DataType(); };
      symbol->size = sizeof(DataType);
      symbol->align = alignof(DataType);
      registerSymbol(name, std::move(symbol));
   }

   void
   registerLibraryDependency(const char *name)
   {
      mLibraryDependencies.push_back(name);
   }

   void
   registerSymbol(const char *name,
                  std::unique_ptr<LibrarySymbol> symbol)
   {
      decaf_check(mSymbolMap.find(name) == mSymbolMap.end());
      symbol->index = BaseSymbolIndex + static_cast<uint32_t>(mSymbolMap.size());
      symbol->name = name;
      mSymbolMap.emplace(name, std::move(symbol));
   }

   template<typename ObjectType>
   void
   registerTypeInfo(const char *typeName,
                    std::vector<const char *> &&virtualTable,
                    std::vector<const char *> &&baseTypes = {})
   {
      auto &typeInfo = mTypeInfo.emplace_back();
      typeInfo.name = typeName;
      typeInfo.virtualTable = std::move(virtualTable);
      typeInfo.baseTypes = std::move(baseTypes);
      typeInfo.hostVirtualTablePtr = &ObjectType::VirtualTable;
      typeInfo.hostTypeDescriptorPtr = &ObjectType::TypeDescriptor;
   }

private:
   LibraryId mID;
   std::string mName;
   std::vector<std::string> mLibraryDependencies;
   std::map<std::string, std::unique_ptr<LibrarySymbol>, std::less<>> mSymbolMap;
   std::vector<LibraryTypeInfo> mTypeInfo;
   std::vector<uint8_t> mGeneratedRpl;
   std::vector<UnimplementedLibraryFunction *> mUnimplementedFunctionExports;
};

virt_addr
registerUnimplementedSymbol(std::string_view module,
                            std::string_view name);

void
setUnimplementedFunctionStubMemory(virt_ptr<void> base,
                                   uint32_t size);

} // namespace cafe::hle
