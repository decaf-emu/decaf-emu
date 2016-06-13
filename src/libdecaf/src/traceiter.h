#pragma once
#include "trace.h"

class TraceIterator
{
public:
   TraceIterator(const Tracer* tracer)
      : mTracer(tracer), mCurTrace(nullptr), mPrevTrace(nullptr)
   {
   }

   const Trace * next()
   {

   }

protected:
   const Tracer *mTracer;
   const Trace *mPrevTrace;
   const Trace *mCurTrace;
};