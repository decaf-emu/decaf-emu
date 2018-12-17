#pragma once

// void(_ObjType object, _OffsetType offset, _SizeType size)
template<typename _ObjType, typename _OffsetType, typename _SizeType, typename _FunctorType>
class RangeCombiner
{
public:
   RangeCombiner(_FunctorType functor)
      : _functor(functor)
   {
   }

   void push(_ObjType object, _OffsetType offset, _SizeType size)
   {
      if (_size > 0 && _object == object && _offset + _size == offset) {
         _size += size;
      } else {
         flush();

         _object = object;
         _offset = offset;
         _size = size;
      }
   }

   void flush()
   {
      if (_size == 0) {
         return;
      }

      _functor(_object, _offset, _size);

      _object = {};
      _offset = {};
      _size = {};
   }

protected:
   _FunctorType _functor = nullptr;
   _ObjType _object = {};
   _OffsetType _offset = {};
   _SizeType _size = {};
};

template<typename _ObjType, typename _OffsetType, typename _SizeType, typename _FunctorType>
static inline RangeCombiner<_ObjType, _OffsetType, _SizeType, _FunctorType>
makeRangeCombiner(_FunctorType functor)
{
   return RangeCombiner<_ObjType, _OffsetType, _SizeType, _FunctorType>(functor);
}