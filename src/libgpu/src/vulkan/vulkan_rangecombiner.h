#pragma once
#ifdef DECAF_VULKAN

namespace vulkan
{

template<typename _ObjType, typename _OffsetType, typename _SizeType>
class RangeCombiner
{
   typedef std::function<void(_ObjType object, _OffsetType offset, _SizeType size)> functor_type;

public:
   RangeCombiner(functor_type functor)
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
   functor_type _functor = nullptr;
   _ObjType _object = {};
   _OffsetType _offset = {};
   _SizeType _size = {};
};

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
