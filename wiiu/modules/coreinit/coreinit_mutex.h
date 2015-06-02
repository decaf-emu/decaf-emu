#pragma once
#include "systemtypes.h"
#include "coreinit_thread.h"

#pragma pack(push, 1)

struct OSMutex
{
   static const uint32_t Tag = 0x6D557458;

   be_val<uint32_t> tag;
   be_ptr<char> name;
   be_val<uint32_t> unk1;
   OSThreadQueue queue;
   be_ptr<OSThread> thread;
   be_val<int32_t> count;
   be_val<uint32_t> unk2;
   be_val<uint32_t> unk3;
};

#pragma pack(pop)

void
OSInitMutex(p32<OSMutex> pMutex);

void
OSInitMutexEx(p32<OSMutex> pMutex, p32<char> pName);
