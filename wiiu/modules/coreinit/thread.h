#pragma once

struct OSThread
{
   char unk[1024];
};

OSThread* OSGetCurrentThread();
