#ifndef GFD_ENUM_H
#define GFD_ENUM_H

#include <common/enum_start.inl>

ENUM_NAMESPACE_ENTER(gfd)

ENUM_BEG(GFDBlockType, uint32_t)
   ENUM_VALUE(EndOfFile,                  1)
   ENUM_VALUE(Padding,                    2)
   ENUM_VALUE(VertexShaderHeader,         3)
   ENUM_VALUE(VertexShaderProgram,        5)
   ENUM_VALUE(PixelShaderHeader,          6)
   ENUM_VALUE(PixelShaderProgram,         7)
   ENUM_VALUE(GeometryShaderHeader,       8)
   ENUM_VALUE(GeometryShaderProgram,      9)
   ENUM_VALUE(GeometryShaderCopyProgram,  10)
   ENUM_VALUE(TextureHeader,              11)
   ENUM_VALUE(TextureImage,               12)
   ENUM_VALUE(TextureMipmap,              13)
   ENUM_VALUE(ComputeShaderHeader,        14)
   ENUM_VALUE(ComputeShaderProgram,       15)
ENUM_END(GFDBlockType)

ENUM_NAMESPACE_EXIT(gfd)

#include <common/enum_end.inl>

#endif // ifdef GFD_ENUM_H
