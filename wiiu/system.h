#pragma once
#include <map>
#include <string>
#include <vector>
#include <cstdint>
#include "heapmanager.h"
#include "systemtypes.h"

class Memory;
class Thread;
class SystemModule;
struct UserModule;
struct SystemFunction;
class FileSystem;

class System
{
public:
   System();

   void initialiseModules();

   void registerModule(const char *name, SystemModule *module);
   SystemModule *findModule(const char *name) const;

   void registerModule(const char *name, UserModule *module);
   UserModule *getUserModule() const;

   void addThread(Thread *thread);
   void removeThread(Thread *thread);

   SystemFunction *getSyscall(uint32_t id);

   WHeapHandle addHeap(HeapManager *heap);
   HeapManager *getHeap(WHeapHandle handle);
   HeapManager *getHeapByAddress(uint32_t vaddr);
   void removeHeap(WHeapHandle handle);

   FileSystem *getFileSystem();
   void setFileSystem(FileSystem *fs);

   // Create a system object for the virtual address
   template<typename Type>
   inline Type *
   addSystemObject(p32<SystemObjectHeader> header)
   {
      auto object = new Type();

      // Set object header values
      header->tag = Type::Tag;
      header->object = object;

      // Add to object map
      auto sysobj = reinterpret_cast<SystemObject*>(object);
      sysobj->objectTag = Type::Tag;
      sysobj->objectVirtualAddress = static_cast<uint32_t>(header);
      mSystemObjects.insert(std::make_pair(sysobj->objectVirtualAddress, sysobj));

      return object;
   }

   // Get the system object associated with virtual address
   template<typename Type>
   inline Type *
   getSystemObject(p32<SystemObjectHeader> header)
   {
      assert(header->tag == Type::Tag);
      return reinterpret_cast<Type*>(header->object);
   }

   // Free the system object associated with virtual address
   bool freeSystemObject(p32<SystemObjectHeader> header);

   // Finds and free all system objects within a range of memory addresses
   void freeSystemObjects(uint32_t address, uint32_t size);

protected:
   void loadThunks();

private:
   UserModule *mUserModule;
   std::map<std::string, SystemModule*> mSystemModules;
   p32<void> mSystemThunks;
   std::vector<SystemFunction*> mSystemCalls;

   std::vector<HeapManager *> mHeaps;
   std::vector<Thread*> mThreads;

   std::map<uint32_t, SystemObject *> mSystemObjects;

   FileSystem *mFileSystem;
};

extern System gSystem;
