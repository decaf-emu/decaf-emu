#include "gx2_debug_dds.h"
#include "gx2_format.h"
#include "gx2_surface.h"

#include <fstream>
#include <vector>

namespace cafe::gx2
{

constexpr inline uint32_t
make_fourcc(uint8_t c0, uint8_t c1, uint8_t c2, uint8_t c3)
{
   return c0 | (c1 << 8) | (c2 << 16) | (c3 << 24);
}

static const uint32_t
DDS_MAGIC = make_fourcc('D', 'D', 'S', ' ');

enum DXGI_FORMAT : uint32_t
{
   DXGI_FORMAT_UNKNOWN = 0,
   DXGI_FORMAT_R32G32B32A32_TYPELESS = 1,
   DXGI_FORMAT_R32G32B32A32_FLOAT = 2,
   DXGI_FORMAT_R32G32B32A32_UINT = 3,
   DXGI_FORMAT_R32G32B32A32_SINT = 4,
   DXGI_FORMAT_R32G32B32_TYPELESS = 5,
   DXGI_FORMAT_R32G32B32_FLOAT = 6,
   DXGI_FORMAT_R32G32B32_UINT = 7,
   DXGI_FORMAT_R32G32B32_SINT = 8,
   DXGI_FORMAT_R16G16B16A16_TYPELESS = 9,
   DXGI_FORMAT_R16G16B16A16_FLOAT = 10,
   DXGI_FORMAT_R16G16B16A16_UNORM = 11,
   DXGI_FORMAT_R16G16B16A16_UINT = 12,
   DXGI_FORMAT_R16G16B16A16_SNORM = 13,
   DXGI_FORMAT_R16G16B16A16_SINT = 14,
   DXGI_FORMAT_R32G32_TYPELESS = 15,
   DXGI_FORMAT_R32G32_FLOAT = 16,
   DXGI_FORMAT_R32G32_UINT = 17,
   DXGI_FORMAT_R32G32_SINT = 18,
   DXGI_FORMAT_R32G8X24_TYPELESS = 19,
   DXGI_FORMAT_D32_FLOAT_S8X24_UINT = 20,
   DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS = 21,
   DXGI_FORMAT_X32_TYPELESS_G8X24_UINT = 22,
   DXGI_FORMAT_R10G10B10A2_TYPELESS = 23,
   DXGI_FORMAT_R10G10B10A2_UNORM = 24,
   DXGI_FORMAT_R10G10B10A2_UINT = 25,
   DXGI_FORMAT_R11G11B10_FLOAT = 26,
   DXGI_FORMAT_R8G8B8A8_TYPELESS = 27,
   DXGI_FORMAT_R8G8B8A8_UNORM = 28,
   DXGI_FORMAT_R8G8B8A8_UNORM_SRGB = 29,
   DXGI_FORMAT_R8G8B8A8_UINT = 30,
   DXGI_FORMAT_R8G8B8A8_SNORM = 31,
   DXGI_FORMAT_R8G8B8A8_SINT = 32,
   DXGI_FORMAT_R16G16_TYPELESS = 33,
   DXGI_FORMAT_R16G16_FLOAT = 34,
   DXGI_FORMAT_R16G16_UNORM = 35,
   DXGI_FORMAT_R16G16_UINT = 36,
   DXGI_FORMAT_R16G16_SNORM = 37,
   DXGI_FORMAT_R16G16_SINT = 38,
   DXGI_FORMAT_R32_TYPELESS = 39,
   DXGI_FORMAT_D32_FLOAT = 40,
   DXGI_FORMAT_R32_FLOAT = 41,
   DXGI_FORMAT_R32_UINT = 42,
   DXGI_FORMAT_R32_SINT = 43,
   DXGI_FORMAT_R24G8_TYPELESS = 44,
   DXGI_FORMAT_D24_UNORM_S8_UINT = 45,
   DXGI_FORMAT_R24_UNORM_X8_TYPELESS = 46,
   DXGI_FORMAT_X24_TYPELESS_G8_UINT = 47,
   DXGI_FORMAT_R8G8_TYPELESS = 48,
   DXGI_FORMAT_R8G8_UNORM = 49,
   DXGI_FORMAT_R8G8_UINT = 50,
   DXGI_FORMAT_R8G8_SNORM = 51,
   DXGI_FORMAT_R8G8_SINT = 52,
   DXGI_FORMAT_R16_TYPELESS = 53,
   DXGI_FORMAT_R16_FLOAT = 54,
   DXGI_FORMAT_D16_UNORM = 55,
   DXGI_FORMAT_R16_UNORM = 56,
   DXGI_FORMAT_R16_UINT = 57,
   DXGI_FORMAT_R16_SNORM = 58,
   DXGI_FORMAT_R16_SINT = 59,
   DXGI_FORMAT_R8_TYPELESS = 60,
   DXGI_FORMAT_R8_UNORM = 61,
   DXGI_FORMAT_R8_UINT = 62,
   DXGI_FORMAT_R8_SNORM = 63,
   DXGI_FORMAT_R8_SINT = 64,
   DXGI_FORMAT_A8_UNORM = 65,
   DXGI_FORMAT_R1_UNORM = 66,
   DXGI_FORMAT_R9G9B9E5_SHAREDEXP = 67,
   DXGI_FORMAT_R8G8_B8G8_UNORM = 68,
   DXGI_FORMAT_G8R8_G8B8_UNORM = 69,
   DXGI_FORMAT_BC1_TYPELESS = 70,
   DXGI_FORMAT_BC1_UNORM = 71,
   DXGI_FORMAT_BC1_UNORM_SRGB = 72,
   DXGI_FORMAT_BC2_TYPELESS = 73,
   DXGI_FORMAT_BC2_UNORM = 74,
   DXGI_FORMAT_BC2_UNORM_SRGB = 75,
   DXGI_FORMAT_BC3_TYPELESS = 76,
   DXGI_FORMAT_BC3_UNORM = 77,
   DXGI_FORMAT_BC3_UNORM_SRGB = 78,
   DXGI_FORMAT_BC4_TYPELESS = 79,
   DXGI_FORMAT_BC4_UNORM = 80,
   DXGI_FORMAT_BC4_SNORM = 81,
   DXGI_FORMAT_BC5_TYPELESS = 82,
   DXGI_FORMAT_BC5_UNORM = 83,
   DXGI_FORMAT_BC5_SNORM = 84,
   DXGI_FORMAT_B5G6R5_UNORM = 85,
   DXGI_FORMAT_B5G5R5A1_UNORM = 86,
   DXGI_FORMAT_B8G8R8A8_UNORM = 87,
   DXGI_FORMAT_B8G8R8X8_UNORM = 88,
   DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM = 89,
   DXGI_FORMAT_B8G8R8A8_TYPELESS = 90,
   DXGI_FORMAT_B8G8R8A8_UNORM_SRGB = 91,
   DXGI_FORMAT_B8G8R8X8_TYPELESS = 92,
   DXGI_FORMAT_B8G8R8X8_UNORM_SRGB = 93,
   DXGI_FORMAT_BC6H_TYPELESS = 94,
   DXGI_FORMAT_BC6H_UF16 = 95,
   DXGI_FORMAT_BC6H_SF16 = 96,
   DXGI_FORMAT_BC7_TYPELESS = 97,
   DXGI_FORMAT_BC7_UNORM = 98,
   DXGI_FORMAT_BC7_UNORM_SRGB = 99,
   DXGI_FORMAT_AYUV = 100,
   DXGI_FORMAT_Y410 = 101,
   DXGI_FORMAT_Y416 = 102,
   DXGI_FORMAT_NV12 = 103,
   DXGI_FORMAT_P010 = 104,
   DXGI_FORMAT_P016 = 105,
   DXGI_FORMAT_420_OPAQUE = 106,
   DXGI_FORMAT_YUY2 = 107,
   DXGI_FORMAT_Y210 = 108,
   DXGI_FORMAT_Y216 = 109,
   DXGI_FORMAT_NV11 = 110,
   DXGI_FORMAT_AI44 = 111,
   DXGI_FORMAT_IA44 = 112,
   DXGI_FORMAT_P8 = 113,
   DXGI_FORMAT_A8P8 = 114,
   DXGI_FORMAT_B4G4R4A4_UNORM = 115,
   DXGI_FORMAT_P208 = 130,
   DXGI_FORMAT_V208 = 131,
   DXGI_FORMAT_V408 = 132,
   DXGI_FORMAT_FORCE_UINT = 0xffffffff
};

enum D3D10_RESOURCE_DIMENSION
{
   D3D10_RESOURCE_DIMENSION_UNKNOWN = 0,
   D3D10_RESOURCE_DIMENSION_BUFFER = 1,
   D3D10_RESOURCE_DIMENSION_TEXTURE1D = 2,
   D3D10_RESOURCE_DIMENSION_TEXTURE2D = 3,
   D3D10_RESOURCE_DIMENSION_TEXTURE3D = 4
};

enum D3D10_RESOURCE_MISC_FLAG
{
   D3D10_RESOURCE_MISC_GENERATE_MIPS = 0x1L,
   D3D10_RESOURCE_MISC_SHARED = 0x2L,
   D3D10_RESOURCE_MISC_TEXTURECUBE = 0x4L,
   D3D10_RESOURCE_MISC_SHARED_KEYEDMUTEX = 0x10L,
   D3D10_RESOURCE_MISC_GDI_COMPATIBLE = 0x20L
};

#pragma pack(push, 1)

struct DdsPixelFormat
{
   uint32_t	dwSize;
   uint32_t	dwFlags;
   uint32_t	dwFourCC;
   uint32_t	dwRGBBitCount;
   uint32_t	dwRBitMask;
   uint32_t	dwGBitMask;
   uint32_t	dwBBitMask;
   uint32_t	dwABitMask;
};

struct DdsHeader
{
   uint32_t	dwSize;
   uint32_t	dwFlags;
   uint32_t	dwHeight;
   uint32_t	dwWidth;
   uint32_t	dwPitchOrLinearSize;
   uint32_t	dwDepth;
   uint32_t	dwMipMapCount;
   uint32_t	dwReserved1[11];
   DdsPixelFormat ddspf;
   uint32_t	dwCaps;
   uint32_t	dwCaps2;
   uint32_t	dwCaps3;
   uint32_t	dwCaps4;
   uint32_t	dwReserved2;
};

struct DdsHeaderDX10
{
   DXGI_FORMAT dxgiFormat;
   D3D10_RESOURCE_DIMENSION resourceDimension;
   uint32_t miscFlag;
   uint32_t arraySize;
   uint32_t reserved;
};

#pragma pack(pop)

#define DDSD_CAPS                               0x00000001L
#define DDSD_HEIGHT                             0x00000002L
#define DDSD_WIDTH                              0x00000004L
#define DDSD_PITCH                              0x00000008L
#define DDSD_BACKBUFFERCOUNT                    0x00000020L
#define DDSD_ZBUFFERBITDEPTH                    0x00000040L
#define DDSD_ALPHABITDEPTH                      0x00000080L
#define DDSD_LPSURFACE                          0x00000800L
#define DDSD_PIXELFORMAT                        0x00001000L
#define DDSD_CKDESTOVERLAY                      0x00002000L
#define DDSD_CKDESTBLT                          0x00004000L
#define DDSD_CKSRCOVERLAY                       0x00008000L
#define DDSD_CKSRCBLT                           0x00010000L
#define DDSD_MIPMAPCOUNT                        0x00020000L
#define DDSD_REFRESHRATE                        0x00040000L
#define DDSD_LINEARSIZE                         0x00080000L
#define DDSD_TEXTURESTAGE                       0x00100000L
#define DDSD_FVF                                0x00200000L
#define DDSD_SRCVBHANDLE                        0x00400000L
#define DDSD_DEPTH                              0x00800000L
#define DDSD_ALL                                0x007ff9eeL

#define DDSCAPS_RESERVED1                       0x00000001L
#define DDSCAPS_ALPHA                           0x00000002L
#define DDSCAPS_BACKBUFFER                      0x00000004L
#define DDSCAPS_COMPLEX                         0x00000008L
#define DDSCAPS_FLIP                            0x00000010L
#define DDSCAPS_FRONTBUFFER                     0x00000020L
#define DDSCAPS_OFFSCREENPLAIN                  0x00000040L
#define DDSCAPS_OVERLAY                         0x00000080L
#define DDSCAPS_PALETTE                         0x00000100L
#define DDSCAPS_PRIMARYSURFACE                  0x00000200L
#define DDSCAPS_RESERVED3                       0x00000400L
#define DDSCAPS_SYSTEMMEMORY                    0x00000800L
#define DDSCAPS_TEXTURE                         0x00001000L
#define DDSCAPS_3DDEVICE                        0x00002000L
#define DDSCAPS_VIDEOMEMORY                     0x00004000L
#define DDSCAPS_VISIBLE                         0x00008000L
#define DDSCAPS_WRITEONLY                       0x00010000L
#define DDSCAPS_ZBUFFER                         0x00020000L
#define DDSCAPS_OWNDC                           0x00040000L
#define DDSCAPS_LIVEVIDEO                       0x00080000L
#define DDSCAPS_HWCODEC                         0x00100000L
#define DDSCAPS_MODEX                           0x00200000L
#define DDSCAPS_MIPMAP                          0x00400000L
#define DDSCAPS_RESERVED2                       0x00800000L
#define DDSCAPS_ALLOCONLOAD                     0x04000000L
#define DDSCAPS_VIDEOPORT                       0x08000000L
#define DDSCAPS_LOCALVIDMEM                     0x10000000L
#define DDSCAPS_NONLOCALVIDMEM                  0x20000000L
#define DDSCAPS_STANDARDVGAMODE                 0x40000000L
#define DDSCAPS_OPTIMIZED                       0x80000000L

#define DDSCAPS2_HARDWAREDEINTERLACE            0x00000002L
#define DDSCAPS2_HINTDYNAMIC                    0x00000004L
#define DDSCAPS2_HINTSTATIC                     0x00000008L
#define DDSCAPS2_TEXTUREMANAGE                  0x00000010L
#define DDSCAPS2_RESERVED1                      0x00000020L
#define DDSCAPS2_RESERVED2                      0x00000040L
#define DDSCAPS2_HINTANTIALIASING               0x00000100L
#define DDSCAPS2_CUBEMAP                        0x00000200L
#define DDSCAPS2_CUBEMAP_POSITIVEX              0x00000400L
#define DDSCAPS2_CUBEMAP_NEGATIVEX              0x00000800L
#define DDSCAPS2_CUBEMAP_POSITIVEY              0x00001000L
#define DDSCAPS2_CUBEMAP_NEGATIVEY              0x00002000L
#define DDSCAPS2_CUBEMAP_POSITIVEZ              0x00004000L
#define DDSCAPS2_CUBEMAP_NEGATIVEZ              0x00008000L
#define DDSCAPS2_CUBEMAP_ALLFACES ( DDSCAPS2_CUBEMAP_POSITIVEX |\
                                    DDSCAPS2_CUBEMAP_NEGATIVEX |\
                                    DDSCAPS2_CUBEMAP_POSITIVEY |\
                                    DDSCAPS2_CUBEMAP_NEGATIVEY |\
                                    DDSCAPS2_CUBEMAP_POSITIVEZ |\
                                    DDSCAPS2_CUBEMAP_NEGATIVEZ )
#define DDSCAPS2_MIPMAPSUBLEVEL                 0x00010000L
#define DDSCAPS2_D3DTEXTUREMANAGE               0x00020000L
#define DDSCAPS2_DONOTPERSIST                   0x00040000L
#define DDSCAPS2_STEREOSURFACELEFT              0x00080000L
#define DDSCAPS2_VOLUME                         0x00200000L

#define DDPF_ALPHAPIXELS                        0x00000001L
#define DDPF_ALPHA                              0x00000002L
#define DDPF_FOURCC                             0x00000004L
#define DDPF_PALETTEINDEXED4                    0x00000008L
#define DDPF_PALETTEINDEXEDTO8                  0x00000010L
#define DDPF_PALETTEINDEXED8                    0x00000020L
#define DDPF_RGB                                0x00000040L
#define DDPF_COMPRESSED                         0x00000080L
#define DDPF_RGBTOYUV                           0x00000100L
#define DDPF_YUV                                0x00000200L
#define DDPF_ZBUFFER                            0x00000400L
#define DDPF_PALETTEINDEXED1                    0x00000800L
#define DDPF_PALETTEINDEXED2                    0x00001000L
#define DDPF_ZPIXELS                            0x00002000L
#define DDPF_STENCILBUFFER                      0x00004000L
#define DDPF_ALPHAPREMULT                       0x00008000L
#define DDPF_LUMINANCE                          0x00020000L
#define DDPF_BUMPLUMINANCE                      0x00040000L
#define DDPF_BUMPDUDV                           0x00080000L

enum D3DFORMAT
{
   D3DFMT_UNKNOWN           =  0,

   D3DFMT_R8G8B8            = 20,
   D3DFMT_A8R8G8B8          = 21,
   D3DFMT_X8R8G8B8          = 22,
   D3DFMT_R5G6B5            = 23,
   D3DFMT_X1R5G5B5          = 24,
   D3DFMT_A1R5G5B5          = 25,
   D3DFMT_A4R4G4B4          = 26,
   D3DFMT_R3G3B2            = 27,
   D3DFMT_A8                = 28,
   D3DFMT_A8R3G3B2          = 29,
   D3DFMT_X4R4G4B4          = 30,
   D3DFMT_A2B10G10R10       = 31,
   D3DFMT_A8B8G8R8          = 32,
   D3DFMT_X8B8G8R8          = 33,
   D3DFMT_G16R16            = 34,
   D3DFMT_A2R10G10B10       = 35,
   D3DFMT_A16B16G16R16      = 36,

   D3DFMT_A8P8              = 40,
   D3DFMT_P8                = 41,

   D3DFMT_L8                = 50,
   D3DFMT_A8L8              = 51,
   D3DFMT_A4L4              = 52,

   D3DFMT_V8U8              = 60,
   D3DFMT_L6V5U5            = 61,
   D3DFMT_X8L8V8U8          = 62,
   D3DFMT_Q8W8V8U8          = 63,
   D3DFMT_V16U16            = 64,
   D3DFMT_A2W10V10U10       = 67,

   D3DFMT_UYVY              = make_fourcc('U', 'Y', 'V', 'Y'),
   D3DFMT_R8G8_B8G8         = make_fourcc('R', 'G', 'B', 'G'),
   D3DFMT_YUY2              = make_fourcc('Y', 'U', 'Y', '2'),
   D3DFMT_G8R8_G8B8         = make_fourcc('G', 'R', 'G', 'B'),
   D3DFMT_DXT1              = make_fourcc('D', 'X', 'T', '1'),
   D3DFMT_DXT2              = make_fourcc('D', 'X', 'T', '2'),
   D3DFMT_DXT3              = make_fourcc('D', 'X', 'T', '3'),
   D3DFMT_DXT4              = make_fourcc('D', 'X', 'T', '4'),
   D3DFMT_DXT5              = make_fourcc('D', 'X', 'T', '5'),

   D3DFMT_ATI1              = make_fourcc('A', 'T', 'I', '1'),
   D3DFMT_AT1N              = make_fourcc('A', 'T', '1', 'N'),
   D3DFMT_ATI2              = make_fourcc('A', 'T', 'I', '2'),
   D3DFMT_AT2N              = make_fourcc('A', 'T', '2', 'N'),

   D3DFMT_ETC               = make_fourcc('E', 'T', 'C', ' '),
   D3DFMT_ETC1              = make_fourcc('E', 'T', 'C', '1'),
   D3DFMT_ATC               = make_fourcc('A', 'T', 'C', ' '),
   D3DFMT_ATCA              = make_fourcc('A', 'T', 'C', 'A'),
   D3DFMT_ATCI              = make_fourcc('A', 'T', 'C', 'I'),

   D3DFMT_POWERVR_2BPP      = make_fourcc('P', 'T', 'C', '2'),
   D3DFMT_POWERVR_4BPP      = make_fourcc('P', 'T', 'C', '4'),

   D3DFMT_D16_LOCKABLE      = 70,
   D3DFMT_D32               = 71,
   D3DFMT_D15S1             = 73,
   D3DFMT_D24S8             = 75,
   D3DFMT_D24X8             = 77,
   D3DFMT_D24X4S4           = 79,
   D3DFMT_D16               = 80,

   D3DFMT_D32F_LOCKABLE     = 82,
   D3DFMT_D24FS8            = 83,

   D3DFMT_L16               = 81,

   D3DFMT_VERTEXDATA        = 100,
   D3DFMT_INDEX16           = 101,
   D3DFMT_INDEX32           = 102,

   D3DFMT_Q16W16V16U16      = 110,

   D3DFMT_MULTI2_ARGB8      = make_fourcc('M','E','T','1'),

   D3DFMT_R16F              = 111,
   D3DFMT_G16R16F           = 112,
   D3DFMT_A16B16G16R16F     = 113,

   D3DFMT_R32F              = 114,
   D3DFMT_G32R32F           = 115,
   D3DFMT_A32B32G32R32F     = 116,

   D3DFMT_CxV8U8            = 117,

   D3DFMT_DX10              = make_fourcc('D', 'X', '1', '0'),

   D3DFMT_FORCE_DWORD       = 0x7fffffff
};

static DdsHeader
encodeHeader(const GX2Surface* surface)
{
   DdsHeader dds;
   memset(&dds, 0, sizeof(DdsHeader));
   dds.dwSize = sizeof(DdsHeader);

   dds.dwWidth = surface->width;
   dds.dwHeight = surface->height;
   dds.dwMipMapCount = surface->mipLevels;
   dds.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_MIPMAPCOUNT;

   if ((surface->format >= GX2SurfaceFormat::UNORM_BC1 && surface->format <= GX2SurfaceFormat::UNORM_BC5)
    || (surface->format >= GX2SurfaceFormat::SRGB_BC1 && surface->format <= GX2SurfaceFormat::SRGB_BC3)
    || (surface->format >= GX2SurfaceFormat::SNORM_BC4 && surface->format <= GX2SurfaceFormat::SNORM_BC5)) {
      dds.dwFlags |= DDSD_LINEARSIZE;
      dds.dwPitchOrLinearSize = surface->imageSize + surface->mipmapSize;
   } else {
      dds.dwFlags |= DDSD_PITCH;
   }

   dds.ddspf.dwSize = sizeof(DdsPixelFormat);
   dds.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;

   if (surface->dim == GX2SurfaceDim::TextureCube) {
      dds.dwCaps2 |= DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_ALLFACES;
   } else if (surface->dim == GX2SurfaceDim::Texture3D) {
      dds.dwFlags |= DDSD_DEPTH;
      dds.dwDepth = surface->depth;
      dds.dwCaps2 |= DDSCAPS2_VOLUME;
   } else if (surface->dim == GX2SurfaceDim::Texture2DArray) {
      dds.dwFlags |= DDSD_DEPTH;
      dds.dwDepth = surface->depth;
   }

   return dds;
}

static DdsHeader
encodeHeader10(const GX2Surface* surface)
{
   DdsHeader dds;
   memset(&dds, 0, sizeof(DdsHeader));
   dds.dwSize = sizeof(DdsHeader);

   dds.dwWidth = surface->width;
   dds.dwHeight = surface->height;
   dds.dwMipMapCount = surface->mipLevels;
   dds.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;

   dds.ddspf.dwSize = sizeof(DdsPixelFormat);
   dds.dwCaps = DDSCAPS_TEXTURE;

   if (surface->dim == GX2SurfaceDim::TextureCube) {
      dds.dwCaps2 |= DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_ALLFACES;
   } else if (surface->dim == GX2SurfaceDim::Texture3D) {
      dds.dwFlags |= DDSD_DEPTH;
      dds.dwDepth = surface->depth;
      dds.dwCaps2 |= DDSCAPS2_VOLUME;
   } else if (surface->dim == GX2SurfaceDim::Texture2DArray) {
      dds.dwFlags |= DDSD_DEPTH;
      dds.dwDepth = surface->depth;
   }

   if (surface->mipLevels > 1) {
      dds.dwFlags |= DDSD_MIPMAPCOUNT;
      dds.dwCaps |= DDSCAPS_MIPMAP;
   }

   return dds;
}

static void
writeData(std::ofstream &file,
          DdsHeader *header,
          const void *image,
          size_t imageSize,
          const void *mipmaps,
          size_t mipmapSize)
{
   file.write(reinterpret_cast<char *>(header), sizeof(DdsHeader));

   if (imageSize > 0) {
      file.write(reinterpret_cast<const char *>(image), imageSize);
   }

   if (mipmapSize > 0) {
      file.write(reinterpret_cast<const char *>(mipmaps), mipmapSize);
   }
}

static bool
encodeFourCC(std::ofstream &file,
             const GX2Surface* surface,
             const void *imagePtr,
             const void *mipPtr,
             uint32_t fourCC,
             uint32_t flags = 0)
{
   DdsHeader header = encodeHeader(surface);
   header.ddspf.dwFourCC = fourCC;
   header.ddspf.dwFlags |= DDPF_FOURCC;
   writeData(file, &header, imagePtr, surface->imageSize, mipPtr, surface->mipmapSize);
   return true;
}

static bool
encodeFourCCWithPitch(std::ofstream &file,
                      const GX2Surface *surface,
                      const void *imagePtr,
                      const void *mipPtr,
                      uint32_t fourCC,
                      uint32_t flags = 0)
{
   DdsHeader header = encodeHeader(surface);
   header.ddspf.dwFourCC = fourCC;
   header.ddspf.dwFlags |= DDPF_FOURCC | flags;
   header.dwPitchOrLinearSize = surface->pitch * internal::getSurfaceFormatBytesPerElement(surface->format);
   writeData(file, &header, imagePtr, surface->imageSize, mipPtr, surface->mipmapSize);
   return true;
}

static bool
encodeMasked(std::ofstream &file,
             const GX2Surface *surface,
             const void *imagePtr,
             const void *mipPtr,
             uint32_t maskR,
             uint32_t maskG,
             uint32_t maskB,
             uint32_t maskA,
             uint32_t flags = 0)
{
   DdsHeader header = encodeHeader(surface);
   header.ddspf.dwFlags |= DDPF_RGB | flags;
   header.ddspf.dwRBitMask = maskR;
   header.ddspf.dwGBitMask = maskG;
   header.ddspf.dwBBitMask = maskB;
   header.ddspf.dwABitMask = maskA;
   header.ddspf.dwRGBBitCount = GX2GetSurfaceFormatBitsPerElement(surface->format);
   header.dwPitchOrLinearSize = surface->pitch * internal::getSurfaceFormatBytesPerElement(surface->format);
   writeData(file, &header, imagePtr, surface->imageSize, mipPtr, surface->mipmapSize);
   return true;
}

static bool
encodeLuminance(std::ofstream &file,
                const GX2Surface *surface,
                const void *imagePtr,
                const void *mipPtr,
                uint32_t maskL,
                uint32_t maskA,
                uint32_t flags = 0)
{
   DdsHeader header = encodeHeader(surface);
   header.ddspf.dwFlags |= DDPF_LUMINANCE | flags;
   header.ddspf.dwRGBBitCount = GX2GetSurfaceFormatBitsPerElement(surface->format);
   header.ddspf.dwRBitMask = maskL;
   header.ddspf.dwABitMask = maskA;
   header.dwPitchOrLinearSize = surface->pitch * internal::getSurfaceFormatBytesPerElement(surface->format);
   writeData(file, &header, imagePtr, surface->imageSize, mipPtr, surface->mipmapSize);
   return true;
}

static void
swizzle565(const void *srcPtr,
           void *dstPtr,
           size_t size)
{
   auto src = reinterpret_cast<const uint16_t *>(srcPtr);
   auto dst = reinterpret_cast<uint16_t *>(dstPtr);

   for (auto i = 0u; i < (size / 2); ++i) {
      auto pixel = src[i];
      pixel = ((pixel & 0xF800) >> 11) | (pixel & 0x07E0) | ((pixel & 0x1F) << 11);
      dst[i] = pixel;
   }
}

static void
swizzle1555(const void *srcPtr,
            void *dstPtr,
            size_t size)
{
   auto src = reinterpret_cast<const uint16_t *>(srcPtr);
   auto dst = reinterpret_cast<uint16_t *>(dstPtr);

   for (auto i = 0u; i < (size / 2); ++i) {
      auto pixel = src[i];
      pixel = (pixel & 0x8000) | ((pixel & 0x7c00) >> 10) | ((pixel & 0x001f) << 10) | (pixel & 0x03e0);
      dst[i] = pixel;
   }
}

static void
swizzle4444(const void *srcPtr,
            void *dstPtr,
            size_t size)
{
   auto src = reinterpret_cast<const uint16_t *>(srcPtr);
   auto dst = reinterpret_cast<uint16_t *>(dstPtr);

   for (auto i = 0u; i < (size / 2); ++i) {
      auto pixel = src[i];
      pixel = (pixel & 0xf000) | ((pixel & 0x0f00) >> 8) | (pixel & 0x00f0) | ((pixel & 0x000f) << 8);
      dst[i] = pixel;
   }
}

static bool
encode565(std::ofstream &file,
          const GX2Surface *surface,
          const void *imagePtr,
          const void *mipPtr,
          uint32_t flags = 0)
{
   DdsHeader header = encodeHeader(surface);
   header.ddspf.dwFlags |= DDPF_RGB | flags;
   header.ddspf.dwRGBBitCount = GX2GetSurfaceFormatBitsPerElement(surface->format);
   header.dwPitchOrLinearSize = surface->pitch * internal::getSurfaceFormatBytesPerElement(surface->format);

   std::vector<uint8_t> image, mipmap;
   image.resize(surface->imageSize);
   mipmap.resize(surface->mipmapSize);

   swizzle565(imagePtr, image.data(), image.size());
   swizzle565(mipPtr, mipmap.data(), mipmap.size());

   writeData(file, &header, image.data(), image.size(), mipmap.data(), mipmap.size());
   return true;
}

static bool
encode1555(std::ofstream &file,
           const GX2Surface *surface,
           const void *imagePtr,
           const void *mipPtr,
           uint32_t flags = 0)
{
   DdsHeader header = encodeHeader(surface);
   header.ddspf.dwFlags |= DDPF_RGB | DDPF_ALPHAPIXELS | flags;
   header.ddspf.dwRGBBitCount = GX2GetSurfaceFormatBitsPerElement(surface->format);
   header.dwPitchOrLinearSize = surface->pitch * internal::getSurfaceFormatBytesPerElement(surface->format);

   header.ddspf.dwRBitMask = 0x7c00;
   header.ddspf.dwGBitMask = 0x3e0;
   header.ddspf.dwBBitMask = 0x1f;
   header.ddspf.dwABitMask = 0x8000;

   std::vector<uint8_t> image, mipmap;
   image.resize(surface->imageSize);
   mipmap.resize(surface->mipmapSize);

   swizzle1555(imagePtr, image.data(), image.size());
   swizzle1555(mipPtr, mipmap.data(), mipmap.size());

   writeData(file, &header, image.data(), image.size(), mipmap.data(), mipmap.size());
   return true;
}

static bool
encode4444(std::ofstream &file,
           const GX2Surface *surface,
           const void *imagePtr,
           const void *mipPtr,
           uint32_t flags = 0)
{
   DdsHeader header = encodeHeader(surface);
   header.ddspf.dwFlags |= DDPF_RGB | DDPF_ALPHAPIXELS | flags;
   header.ddspf.dwRGBBitCount = GX2GetSurfaceFormatBitsPerElement(surface->format);
   header.dwPitchOrLinearSize = surface->pitch * internal::getSurfaceFormatBytesPerElement(surface->format);

   header.ddspf.dwRBitMask = 0xf00;
   header.ddspf.dwGBitMask = 0xf0;
   header.ddspf.dwBBitMask = 0xf;
   header.ddspf.dwABitMask = 0xf000;

   std::vector<uint8_t> image, mipmap;
   image.resize(surface->imageSize);
   mipmap.resize(surface->mipmapSize);

   swizzle4444(imagePtr, image.data(), image.size());
   swizzle4444(mipPtr, mipmap.data(), mipmap.size());

   writeData(file, &header, image.data(), image.size(), mipmap.data(), mipmap.size());
   return true;
}

static bool
encodeDX10(std::ofstream &file,
           const GX2Surface *surface,
           const void *imagePtr,
           const void *mipPtr,
           DXGI_FORMAT dxgiFormat)
{
   DdsHeader header = encodeHeader10(surface);
   header.ddspf.dwFlags = DDPF_FOURCC;
   header.ddspf.dwFourCC = make_fourcc('D', 'X', '1', '0');

   switch (surface->format) {
   case GX2SurfaceFormat::UNORM_BC1:
   case GX2SurfaceFormat::SRGB_BC1:
   case GX2SurfaceFormat::UNORM_BC4:
   case GX2SurfaceFormat::SNORM_BC4:
      header.dwPitchOrLinearSize = header.dwWidth * 2;
      break;
   case GX2SurfaceFormat::UNORM_BC2:
   case GX2SurfaceFormat::UNORM_BC3:
   case GX2SurfaceFormat::UNORM_BC5:
   case GX2SurfaceFormat::SRGB_BC2:
   case GX2SurfaceFormat::SRGB_BC3:
   case GX2SurfaceFormat::SNORM_BC5:
      header.dwPitchOrLinearSize = header.dwWidth * 4;
      break;
   case GX2SurfaceFormat::FLOAT_R11_G11_B10:
      header.dwPitchOrLinearSize = header.dwWidth * 4;
      break;
   }

   file.write(reinterpret_cast<char *>(&header), sizeof(DdsHeader));

   // Setup dx10 header
   DdsHeaderDX10 headerDX10;
   memset(&headerDX10, 0, sizeof(DdsHeaderDX10));

   headerDX10.dxgiFormat = dxgiFormat;

   if (surface->dim == GX2SurfaceDim::Texture2D) {
      headerDX10.resourceDimension = D3D10_RESOURCE_DIMENSION_TEXTURE2D;
      headerDX10.miscFlag = 0;
      headerDX10.arraySize = 1;
   } else if (surface->dim == GX2SurfaceDim::TextureCube) {
      headerDX10.resourceDimension = D3D10_RESOURCE_DIMENSION_TEXTURE2D;
      headerDX10.miscFlag = D3D10_RESOURCE_MISC_TEXTURECUBE;
      headerDX10.arraySize = 6;
   } else {
      headerDX10.resourceDimension = D3D10_RESOURCE_DIMENSION_TEXTURE3D;
      headerDX10.miscFlag = 0;
      headerDX10.arraySize = surface->depth;
   }

   file.write(reinterpret_cast<char *>(&headerDX10), sizeof(DdsHeaderDX10));

   if (surface->imageSize > 0) {
      file.write(reinterpret_cast<const char *>(imagePtr), surface->imageSize);
   }

   if (surface->mipmapSize > 0) {
      file.write(reinterpret_cast<const char *>(mipPtr), surface->mipmapSize);
   }

   return true;
}

static void
encodeCubemap(const GX2Surface *surface,
              const void *imagePtr,
              const void *mipPtr,
              void *cubeData)
{
   // DDS cubemap has slices interleaved with mipmaps... fuck you.
   int minMipSize = 1;
   if (surface->format == GX2SurfaceFormat::UNORM_BC1 ||
       surface->format == GX2SurfaceFormat::SRGB_BC1) {
      minMipSize = 8;
   } else if (surface->format >= GX2SurfaceFormat::UNORM_BC2 &&
              surface->format <= GX2SurfaceFormat::SNORM_BC5) {
      minMipSize = 16;
   } else {
      minMipSize = 1;
   }

   auto dst = reinterpret_cast<uint8_t *>(cubeData);
   auto srcImage = reinterpret_cast<const uint8_t *>(imagePtr);
   auto srcMip = reinterpret_cast<const uint8_t *>(mipPtr);

   for (auto i = 0; i < 6; ++i) {
      int sliceSize = surface->imageSize / 6;
      std::memcpy(dst, srcImage + (sliceSize * i), sliceSize);
      dst += sliceSize;

      int mipOffset = 0;
      for (auto j = 0u; j < surface->mipLevels - 1; ++j) {
         int mipSize = sliceSize >> ((j + 1) * 2);
         mipSize = std::max(mipSize, minMipSize);

         memcpy(dst, srcMip + mipOffset + (mipSize * i), mipSize);
         dst += mipSize;
         mipOffset += mipSize * 6;
      }
   }
}

namespace debug
{

bool
saveDDS(const std::string &filename,
        const GX2Surface *surface,
        const void *imagePtr,
        const void *mipPtr)
{
   std::ofstream fh { filename, std::ofstream::binary };

   if (!fh.is_open()) {
      return false;
   }

   fh.write(reinterpret_cast<const char *>(&DDS_MAGIC), sizeof(DDS_MAGIC));

   std::vector<uint8_t> cubeAdjustedImage;
   std::vector<uint8_t> cubeAdjustedMip;
   if (surface->dim == GX2SurfaceDim::TextureCube) {
      std::vector<uint8_t> cubeData;
      cubeData.resize(surface->imageSize + surface->mipmapSize);

      encodeCubemap(surface, imagePtr, mipPtr, cubeData.data());

      cubeAdjustedImage.resize(surface->imageSize);
      std::memcpy(cubeAdjustedImage.data(), cubeData.data(), surface->imageSize);
      imagePtr = cubeAdjustedImage.data();

      cubeAdjustedMip.resize(surface->mipmapSize);
      std::memcpy(cubeAdjustedMip.data(), cubeData.data() + surface->imageSize, surface->mipmapSize);
      mipPtr = cubeAdjustedMip.data();
   }

   bool result = false;

   switch (surface->format) {
   case GX2SurfaceFormat::UNORM_R16_G16_B16_A16:
      result = encodeFourCCWithPitch(fh, surface, imagePtr, mipPtr, D3DFMT_A16B16G16R16, DDPF_ALPHAPIXELS);
      break;
   case GX2SurfaceFormat::FLOAT_R16_G16_B16_A16:
      result = encodeFourCCWithPitch(fh, surface, imagePtr, mipPtr, D3DFMT_A16B16G16R16F, DDPF_ALPHAPIXELS);
      break;
   case GX2SurfaceFormat::FLOAT_R32_G32_B32_A32:
      result = encodeFourCCWithPitch(fh, surface, imagePtr, mipPtr, D3DFMT_A32B32G32R32F, DDPF_ALPHAPIXELS);
      break;
   case GX2SurfaceFormat::UNORM_R10_G10_B10_A2:
      result = encodeMasked(fh, surface, imagePtr, mipPtr, 0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000, DDPF_ALPHAPIXELS);
      break;
   case GX2SurfaceFormat::UNORM_R8_G8_B8_A8:
   case GX2SurfaceFormat::SRGB_R8_G8_B8_A8:
      result = encodeMasked(fh, surface, imagePtr, mipPtr, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000, DDPF_ALPHAPIXELS);
      break;
   case GX2SurfaceFormat::UNORM_R5_G6_B5:
      result = encode565(fh, surface, imagePtr, mipPtr);
      break;
   case GX2SurfaceFormat::UNORM_R5_G5_B5_A1:
      result = encode1555(fh, surface, imagePtr, mipPtr);
      break;
   case GX2SurfaceFormat::UNORM_R4_G4_B4_A4:
      result = encode4444(fh, surface, imagePtr, mipPtr);
      break;
   case GX2SurfaceFormat::UNORM_R8:
      result = encodeLuminance(fh, surface, imagePtr, mipPtr, 0xff, 0);
      break;
   case GX2SurfaceFormat::UNORM_R8_G8:
      result = encodeLuminance(fh, surface, imagePtr, mipPtr, 0xff, 0xff00, DDPF_ALPHAPIXELS);
      break;
   case GX2SurfaceFormat::UNORM_BC1:
      result = encodeFourCC(fh, surface, imagePtr, mipPtr, make_fourcc('D', 'X', 'T', '1'));
      break;
   case GX2SurfaceFormat::UNORM_BC2:
      result = encodeFourCC(fh, surface, imagePtr, mipPtr, make_fourcc('D', 'X', 'T', '3'));
      break;
   case GX2SurfaceFormat::UNORM_BC3:
      result = encodeFourCC(fh, surface, imagePtr, mipPtr, make_fourcc('D', 'X', 'T', '5'));
      break;
   case GX2SurfaceFormat::UNORM_BC4:
      result = encodeFourCC(fh, surface, imagePtr, mipPtr, make_fourcc('A', 'T', 'I', '1'));
      break;
   case GX2SurfaceFormat::SNORM_BC4:
      result = encodeFourCC(fh, surface, imagePtr, mipPtr, make_fourcc('A', 'T', 'I', '1'));
      break;
   case GX2SurfaceFormat::UNORM_BC5:
      result = encodeFourCC(fh, surface, imagePtr, mipPtr, make_fourcc('A', 'T', 'I', '2'));
      break;
   case GX2SurfaceFormat::SNORM_BC5:
      result = encodeFourCC(fh, surface, imagePtr, mipPtr, make_fourcc('A', 'T', 'I', '2'));
      break;
   case GX2SurfaceFormat::SRGB_BC1:
      result = encodeDX10(fh, surface, imagePtr, mipPtr, DXGI_FORMAT_BC1_UNORM_SRGB);
      break;
   case GX2SurfaceFormat::SRGB_BC2:
      result = encodeDX10(fh, surface, imagePtr, mipPtr, DXGI_FORMAT_BC2_UNORM_SRGB);
      break;
   case GX2SurfaceFormat::SRGB_BC3:
      result = encodeDX10(fh, surface, imagePtr, mipPtr, DXGI_FORMAT_BC3_UNORM_SRGB);
      break;
   case GX2SurfaceFormat::FLOAT_R11_G11_B10:
      result = encodeDX10(fh, surface, imagePtr, mipPtr, DXGI_FORMAT_R11G11B10_FLOAT);
      break;
   }

   return result;
}

} // namespace debug

} // namepsace cafe::gx2
