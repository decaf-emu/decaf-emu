#version 450

layout(constant_id = 0) const bool IsUntiling = true;
layout(constant_id = 1) const uint MicroTileThickness = 1;
layout(constant_id = 2) const uint MacroTileWidth = 1;
layout(constant_id = 3) const uint MacroTileHeight = 1;
layout(constant_id = 4) const bool IsMacro3D = false;
layout(constant_id = 5) const bool IsBankSwapped = false;
layout(constant_id = 6) const uint BitsPerElement = 8;
layout(constant_id = 7) const bool IsDepth = false;
layout(constant_id = 8) const uint SubGroupSize = 32;

// Specify our grouping setup
layout(local_size_x_id = 8, local_size_y = 1, local_size_z = 1) in;

// Information about the GPU tiling itself.
const uint MicroTileWidth = 8;
const uint MicroTileHeight = 8;
const uint NumPipes = 2;
const uint NumBanks = 4;

const uint PipeInterleaveBytes = 256;
const uint NumGroupBits = 8;
const uint NumPipeBits = 1;
const uint NumBankBits = 2;
const uint GroupMask = ((1 << NumGroupBits) - 1);

const uint RowSize = 2048;
const uint SwapSize = 256;
const uint SplitSize = 2048;
const uint BankSwapBytes = 256;

// Setup some convenience information based on our constants
const bool IsMacroTiling = (MacroTileWidth > 1 || MacroTileHeight > 1);
const uint BytesPerElement = BitsPerElement / 8;
const uint MicroTileBytes = MicroTileWidth * MicroTileHeight * MicroTileThickness * BytesPerElement;
const uint MacroTileBytes = MacroTileWidth * MacroTileHeight * MicroTileBytes;

layout(push_constant) uniform Parameters {
   // Micro tiling parameters
   uint untiledStride;
   uint numTilesPerRow;
   uint numTileRows;
   uint tiledSliceIndex;
   uint sliceIndex;
   uint numSlices;
   uint sliceBytes;

   // Macro tiling parameters
   uint bankSwizzle;
   uint pipeSwizzle;
   uint bankSwapWidth;
} params;

// Set up the shader inputs
layout(std430, binding = 0) buffer tiledBuffer4 { uint tiled4[]; };
layout(std430, binding = 1) buffer untiledBuffer4 { uint untiled4[]; };
void copyBytes4(uint untiledOffset, uint tiledOffset, uint numBytes)
{
   if (IsUntiling) {
       for (uint i = 0; i < numBytes / 4; ++i) {
         untiled4[(untiledOffset / 4) + i] = tiled4[(tiledOffset / 4) + i];
      }
   } else {
       for (uint i = 0; i < numBytes / 4; ++i) {
         tiled4[(tiledOffset / 4) + i] = untiled4[(untiledOffset / 4) + i];
      }
   }
}

layout(std430, binding = 0) buffer tiledBuffer8 { uvec2 tiled8[]; };
layout(std430, binding = 1) buffer untiledBuffer8 { uvec2 untiled8[]; };
void copyBytes8(uint untiledOffset, uint tiledOffset, uint numBytes)
{
   if (IsUntiling) {
       for (uint i = 0; i < numBytes / 8; ++i) {
         untiled8[(untiledOffset / 8) + i] = tiled8[(tiledOffset / 8) + i];
      }
   } else {
       for (uint i = 0; i < numBytes / 8; ++i) {
         tiled8[(tiledOffset / 8) + i] = untiled8[(untiledOffset / 8) + i];
      }
   }
}

layout(std430, binding = 0) buffer tiledBuffer16 { uvec4 tiled16[]; };
layout(std430, binding = 1) buffer untiledBuffer16 { uvec4 untiled16[]; };
void copyBytes16(uint untiledOffset, uint tiledOffset, uint numBytes)
{
   if (IsUntiling) {
       for (uint i = 0; i < numBytes / 16; ++i) {
         untiled16[(untiledOffset / 16)] = tiled16[(tiledOffset / 16)];
      }
   } else {
       for (uint i = 0; i < numBytes / 16; ++i) {
         tiled16[(tiledOffset / 16)] = untiled16[(untiledOffset / 16)];
      }
   }
}

/*
   8 bits per element:
    0:   0,  1,  2,  3,  4,  5,  6,  7,
    8:  16, 17, 18, 19, 20, 21, 22, 23,
   16:   8,  9, 10, 11, 12, 13, 14, 15,
   24:  24, 25, 26, 27, 28, 29, 30, 31,

   32:  32, 33, 34, 35, 36, 37, 38, 39,
   40:  48, 49, 50, 51, 52, 53, 54, 55,
   48:  40, 41, 42, 43, 44, 45, 46, 47,
   56:  56, 57, 58, 59, 60, 61, 62, 63,
*/
void untileMicroTiling8(uint tiledOffset, uint untiledOffset, uint untiledStride)
{
   const uint tiledStride = MicroTileWidth;
   const uint rowSize = MicroTileWidth;

   for (uint y = 0; y < MicroTileHeight; y += 4) {
      const uint untiledRow0 = untiledOffset + 0 * untiledStride;
      const uint untiledRow1 = untiledOffset + 1 * untiledStride;
      const uint untiledRow2 = untiledOffset + 2 * untiledStride;
      const uint untiledRow3 = untiledOffset + 3 * untiledStride;

      const uint tiledRow0 = tiledOffset + 0 * tiledStride;
      const uint tiledRow1 = tiledOffset + 1 * tiledStride;
      const uint tiledRow2 = tiledOffset + 2 * tiledStride;
      const uint tiledRow3 = tiledOffset + 3 * tiledStride;

      copyBytes8(untiledRow0, tiledRow0, rowSize);
      copyBytes8(untiledRow1, tiledRow2, rowSize);
      copyBytes8(untiledRow2, tiledRow1, rowSize);
      copyBytes8(untiledRow3, tiledRow3, rowSize);

      untiledOffset += 4 * untiledStride;
      tiledOffset += 4 * tiledStride;
   }
}

/*
   16 bits per element:
    0:   0,  1,  2,  3,  4,  5,  6,  7,
    8:   8,  9, 10, 11, 12, 13, 14, 15,
   16:  16, 17, 18, 19, 20, 21, 22, 23,
   24:  24, 25, 26, 27, 28, 29, 30, 31,
   32:  32, 33, 34, 35, 36, 37, 38, 39,
   40:  40, 41, 42, 43, 44, 45, 46, 47,
   48:  48, 49, 50, 51, 52, 53, 54, 55,
   56:  56, 57, 58, 59, 60, 61, 62, 63,
*/
void untileMicroTiling16(uint tiledOffset, uint untiledOffset, uint untiledStride)
{
   const uint tiledStride = MicroTileWidth * 2;
   const uint rowSize = MicroTileWidth * 2;

   for (uint y = 0; y < MicroTileHeight; ++y) {
      copyBytes16(untiledOffset, tiledOffset, rowSize);

      untiledOffset += untiledStride;
      tiledOffset += tiledStride;
   }
}

/*
   32 bits per element:
    0:   0,  1,  2,  3,    8,  9, 10, 11,
    8:   4,  5,  6,  7,   12, 13, 14, 15,

   16:  16, 17, 18, 19,   24, 25, 26, 27,
   24:  20, 21, 22, 23,   28, 29, 30, 31,

   32:  32, 33, 34, 35,   40, 41, 42, 43,
   40:  36, 37, 38, 39,   44, 45, 46, 47,

   48:  48, 49, 50, 51,   56, 57, 58, 59,
   56:  52, 53, 54, 55,   60, 61, 62, 63,
*/
void untileMicroTiling32(uint tiledOffset, uint untiledOffset, uint untiledStride)
{
   const uint tiledStride = MicroTileWidth * 4;
   const uint groupSize = 4 * 4;

   for (uint y = 0; y < MicroTileHeight; y += 2) {
      const uint untiledRow1 = untiledOffset + 0 * untiledStride;
      const uint untiledRow2 = untiledOffset + 1 * untiledStride;

      const uint tiledRow1 = tiledOffset + 0 * tiledStride;
      const uint tiledRow2 = tiledOffset + 1 * tiledStride;

      copyBytes16(untiledRow1 + 0, tiledRow1 + 0, groupSize);
      copyBytes16(untiledRow1 + 16, tiledRow2 + 0, groupSize);

      copyBytes16(untiledRow2 + 0, tiledRow1 + 16, groupSize);
      copyBytes16(untiledRow2 + 16, tiledRow2 + 16, groupSize);

      tiledOffset += tiledStride * 2;
      untiledOffset += untiledStride * 2;
   }
}

/*
   64 bits per element:
    0:   0,  1,    4,  5,    8,  9,   12, 13,
    8:   2,  3,    6,  7,   10, 11,   14, 15,

   16:  16, 17,   20, 21,   24, 25,   28, 29,
   24:  18, 19,   22, 23,   26, 27,   30, 31,

   32:  32, 33,   36, 37,   40, 41,   44, 45,
   40:  34, 35,   38, 39,   42, 43,   46, 47,

   48:  48, 49,   52, 53,   56, 57,   60, 61,
   56:  50, 51,   54, 55,   58, 59,   62, 63,
*/
void untileMicroTiling64(uint tiledOffset, uint untiledOffset, uint untiledStride)
{
   const uint tiledStride = MicroTileWidth * 8;
   const uint groupBytes = 2 * 8;

   // This will automatically DCE'd by compiler if the below ifdef is not used.
   const uint nextGroupOffset = tiledOffset + (0x100 << (NumBankBits + NumPipeBits));

   for (uint y = 0; y < MicroTileHeight; y += 2) {
      if (IsMacroTiling && y == 4) {
         tiledOffset = nextGroupOffset;
      }

      const uint untiledRow1 = untiledOffset + 0 * untiledStride;
      const uint untiledRow2 = untiledOffset + 1 * untiledStride;

      const uint tiledRow1 = tiledOffset + 0 * tiledStride;
      const uint tiledRow2 = tiledOffset + 1 * tiledStride;

      copyBytes16(untiledRow1 + 0, tiledRow1 + 0, groupBytes);
      copyBytes16(untiledRow2 + 0, tiledRow1 + 16, groupBytes);

      copyBytes16(untiledRow1 + 16, tiledRow1 + 32, groupBytes);
      copyBytes16(untiledRow2 + 16, tiledRow1 + 48, groupBytes);

      copyBytes16(untiledRow1 + 32, tiledRow2 + 0, groupBytes);
      copyBytes16(untiledRow2 + 32, tiledRow2 + 16, groupBytes);

      copyBytes16(untiledRow1 + 48, tiledRow2 + 32, groupBytes);
      copyBytes16(untiledRow2 + 48, tiledRow2 + 48, groupBytes);

      tiledOffset += tiledStride * 2;
      untiledOffset += untiledStride * 2;
   }
}

/*
   128 bits per element:
      0:   0,  2,    4,  6,    8, 10,   12, 14,
      8:   1,  3,    5,  7,    9, 11,   13, 15,

     16:  16, 18,   20, 22,   24, 26,   28, 30,
     24:  17, 19,   21, 23,   25, 27,   29, 31,

     32:  32, 34,   36, 38,   40, 42,   44, 46,
     40:  33, 35,   37, 39,   41, 43,   45, 47,

     48:  48, 50,   52, 54,   56, 58,   60, 62,
     56:  49, 51,   53, 55,   57, 59,   61, 63,
*/
void untileMicroTiling128(uint tiledOffset, uint untiledOffset, uint untiledStride)
{
   const uint tiledStride = MicroTileWidth * 16;
   const uint elemBytes = 16;

   for (uint y = 0; y < MicroTileHeight; y += 2) {
      const uint untiledRow1 = untiledOffset + 0 * untiledStride;
      const uint untiledRow2 = untiledOffset + 1 * untiledStride;

      const uint tiledRow1 = tiledOffset + 0 * tiledStride;
      const uint tiledRow2 = tiledOffset + 1 * tiledStride;

      copyBytes16(untiledRow1 + 0 * elemBytes, tiledRow1 + 0 * elemBytes, elemBytes);
      copyBytes16(untiledRow1 + 1 * elemBytes, tiledRow1 + 2 * elemBytes, elemBytes);
      copyBytes16(untiledRow2 + 0 * elemBytes, tiledRow1 + 1 * elemBytes, elemBytes);
      copyBytes16(untiledRow2 + 1 * elemBytes, tiledRow1 + 3 * elemBytes, elemBytes);

      copyBytes16(untiledRow1 + 2 * elemBytes, tiledRow1 + 4 * elemBytes, elemBytes);
      copyBytes16(untiledRow1 + 3 * elemBytes, tiledRow1 + 6 * elemBytes, elemBytes);
      copyBytes16(untiledRow2 + 2 * elemBytes, tiledRow1 + 5 * elemBytes, elemBytes);
      copyBytes16(untiledRow2 + 3 * elemBytes, tiledRow1 + 7 * elemBytes, elemBytes);

      copyBytes16(untiledRow1 + 4 * elemBytes, tiledRow2 + 0 * elemBytes, elemBytes);
      copyBytes16(untiledRow1 + 5 * elemBytes, tiledRow2 + 2 * elemBytes, elemBytes);
      copyBytes16(untiledRow2 + 4 * elemBytes, tiledRow2 + 1 * elemBytes, elemBytes);
      copyBytes16(untiledRow2 + 5 * elemBytes, tiledRow2 + 3 * elemBytes, elemBytes);

      copyBytes16(untiledRow1 + 6 * elemBytes, tiledRow2 + 4 * elemBytes, elemBytes);
      copyBytes16(untiledRow1 + 7 * elemBytes, tiledRow2 + 6 * elemBytes, elemBytes);
      copyBytes16(untiledRow2 + 6 * elemBytes, tiledRow2 + 5 * elemBytes, elemBytes);
      copyBytes16(untiledRow2 + 7 * elemBytes, tiledRow2 + 7 * elemBytes, elemBytes);

      if (IsMacroTiling) {
         tiledOffset += 0x100 << (NumBankBits + NumPipeBits);
      } else {
         tiledOffset += tiledStride * 2;
      }

      untiledOffset += untiledStride * 2;
   }
}

/*
   depth elements:
       0:   0,  1,  4,  5,   16, 17, 20, 21,
       8:   2,  3,  6,  7,   18, 19, 22, 23,
      16:   8,  9, 12, 13,   24, 25, 28, 29,
      24:  10, 11, 14, 15,   26, 27, 30, 31,
      32:  32, 33, 36, 37,   48, 49, 52, 53,
      40:  34, 35, 38, 39,   50, 51, 54, 55,
      48:  40, 41, 44, 45,   56, 57, 60, 61,
      56:  42, 43, 46, 47,   58, 59, 62, 63,

       0:   0,  2,    8, 10,
       8:   1,  3,    9, 11,
      16:   4,  6,   12, 14,
      24:   5,  7,   13, 15,
*/
void copyXYGroups(uint tiledOffset, uint tiledStride, uint tX, uint tY,
                  uint untiledOffset, uint untiledStride, uint uX, uint uY,
                  uint groupBytes)
{
   copyBytes4(untiledOffset + uY * untiledStride + uX * groupBytes, tiledOffset + tY * tiledStride + tX * groupBytes, groupBytes);
}
void untileMicroTilingDepth(uint tiledOffset, uint untiledOffset, uint untiledStride)
{
   const uint tiledStride = MicroTileWidth * BytesPerElement;
   const uint groupBytes = 2 * BytesPerElement;

   for (uint y = 0; y < 8; y += 4) {
      copyXYGroups(tiledOffset, tiledStride, 0, y + 0, untiledOffset, untiledStride, 0, y + 0, groupBytes);
      copyXYGroups(tiledOffset, tiledStride, 1, y + 0, untiledOffset, untiledStride, 0, y + 1, groupBytes);
      copyXYGroups(tiledOffset, tiledStride, 2, y + 0, untiledOffset, untiledStride, 1, y + 0, groupBytes);
      copyXYGroups(tiledOffset, tiledStride, 3, y + 0, untiledOffset, untiledStride, 1, y + 1, groupBytes);

      copyXYGroups(tiledOffset, tiledStride, 0, y + 1, untiledOffset, untiledStride, 0, y + 2, groupBytes);
      copyXYGroups(tiledOffset, tiledStride, 1, y + 1, untiledOffset, untiledStride, 0, y + 3, groupBytes);
      copyXYGroups(tiledOffset, tiledStride, 2, y + 1, untiledOffset, untiledStride, 1, y + 2, groupBytes);
      copyXYGroups(tiledOffset, tiledStride, 3, y + 1, untiledOffset, untiledStride, 1, y + 3, groupBytes);

      copyXYGroups(tiledOffset, tiledStride, 0, y + 2, untiledOffset, untiledStride, 2, y + 0, groupBytes);
      copyXYGroups(tiledOffset, tiledStride, 1, y + 2, untiledOffset, untiledStride, 2, y + 1, groupBytes);
      copyXYGroups(tiledOffset, tiledStride, 2, y + 2, untiledOffset, untiledStride, 3, y + 0, groupBytes);
      copyXYGroups(tiledOffset, tiledStride, 3, y + 2, untiledOffset, untiledStride, 3, y + 1, groupBytes);

      copyXYGroups(tiledOffset, tiledStride, 0, y + 3, untiledOffset, untiledStride, 2, y + 2, groupBytes);
      copyXYGroups(tiledOffset, tiledStride, 1, y + 3, untiledOffset, untiledStride, 2, y + 3, groupBytes);
      copyXYGroups(tiledOffset, tiledStride, 2, y + 3, untiledOffset, untiledStride, 3, y + 2, groupBytes);
      copyXYGroups(tiledOffset, tiledStride, 3, y + 3, untiledOffset, untiledStride, 3, y + 3, groupBytes);
   }
}


void mainMicroTiling()
{
   const uint bytesPerElement = BytesPerElement;
   const uint microTilesNumRows = params.numTileRows;
   const uint microTilesPerRow = params.numTilesPerRow;
   const uint untiledStride = params.untiledStride;
   const uint microTileBytes = MicroTileBytes;
   const uint sliceBytes = params.sliceBytes;
   const uint tiledSliceIndex = params.tiledSliceIndex;
   const uint baseSliceIndex = params.sliceIndex;
   const uint microTileThickness = MicroTileThickness;

   // Calculate which micro tile we are working with
   const uint tilesPerSlice = microTilesNumRows * microTilesPerRow;
   const uint globalTileIndex = gl_GlobalInvocationID.x;

   const uint dispatchSliceIndex = globalTileIndex / tilesPerSlice;
   const uint sliceTileIndex = globalTileIndex % tilesPerSlice;

   const uint microTileY = sliceTileIndex / microTilesPerRow;
   const uint microTileX = sliceTileIndex % microTilesPerRow;

   // Check that our tiles are inside the bounds described.  Note that additional
   // warps will actually overflow into the next slice...
   if (dispatchSliceIndex >= params.numSlices) {
      return;
   }

   // Calculate our overall slice index
   const uint sliceIndex = baseSliceIndex + dispatchSliceIndex;

   // Figure out what our slice offset is
   const uint microTileIndexZ = sliceIndex / microTileThickness;
   const uint sliceOffset = microTileIndexZ * sliceBytes;

   // Calculate offset within thick slices for current slice
   uint thickSliceOffset = (sliceIndex % microTileThickness) * (microTileBytes / microTileThickness);

   const uint pixelX = microTileX * MicroTileWidth;
   const uint pixelY = microTileY * MicroTileHeight;

   const uint sampleOffset = 0;
   const uint sampleSliceOffset = sliceOffset + sampleOffset + thickSliceOffset;

   // Prepare to call the microtiler
   uint tiledOffset = sampleSliceOffset + (microTileX + (microTileY * microTilesPerRow)) * microTileBytes;
   uint untiledOffset = (sliceIndex * sliceBytes / microTileThickness) + (pixelX * bytesPerElement) + (pixelY * untiledStride);

   // Shift backwards by the base slices
   untiledOffset -= baseSliceIndex * sliceBytes / microTileThickness;
   tiledOffset -= tiledSliceIndex * sliceBytes / microTileThickness;

   if (IsDepth) {
      untileMicroTilingDepth(tiledOffset, untiledOffset, untiledStride);
   } else {
      if (BitsPerElement == 8) {
         untileMicroTiling8(tiledOffset, untiledOffset, untiledStride);
      } else if (BitsPerElement == 16) {
         untileMicroTiling16(tiledOffset, untiledOffset, untiledStride);
      } else if (BitsPerElement == 32) {
         untileMicroTiling32(tiledOffset, untiledOffset, untiledStride);
      } else if (BitsPerElement == 64) {
         untileMicroTiling64(tiledOffset, untiledOffset, untiledStride);
      } else if (BitsPerElement == 128) {
         untileMicroTiling128(tiledOffset, untiledOffset, untiledStride);
      }
   }
}

void mainMacroTiling()
{
   const uint bytesPerElement = BytesPerElement;
   const uint macroTileWidth = MacroTileWidth;
   const uint macroTileHeight = MacroTileHeight;
   const uint microTilesPerMacro = 8; // This is always 8 for macro tiling! 2x4, 4x2, 1x8
   const uint macroTilesNumRows = params.numTileRows;
   const uint macroTilesPerRow = params.numTilesPerRow;
   const uint untiledStride = params.untiledStride;
   const uint microTileBytes = MicroTileBytes;
   const uint sliceBytes = params.sliceBytes;
   const uint tiledSliceIndex = params.tiledSliceIndex;
   const uint baseSliceIndex = params.sliceIndex;
   const uint microTileThickness = MicroTileThickness;
   const uint macroTileBytes = MacroTileBytes;
   const uint bankSwizzle = params.bankSwizzle;
   const uint pipeSwizzle = params.pipeSwizzle;
   const uint bankSwapWidth = params.bankSwapWidth;

   // We do not currently support samples.
   const uint sampleIndex = 0;

   // Calculate which micro tile we are working with
   const uint microTilesPerMacroRow = microTilesPerMacro * macroTilesPerRow;
   const uint microTilesPerSlice = microTilesPerMacroRow * macroTilesNumRows;
   const uint globalTileIndex = gl_GlobalInvocationID.x;

   const uint dispatchSliceIndex = globalTileIndex / microTilesPerSlice;
   const uint macroMicroTileIndex = (globalTileIndex % microTilesPerSlice);

   const uint macroTileY = macroMicroTileIndex / microTilesPerMacroRow;
   const uint macroRowTileIndex = macroMicroTileIndex % microTilesPerMacroRow;

   const uint macroTileX = macroRowTileIndex / microTilesPerMacro;
   const uint microTileIndex = macroRowTileIndex % microTilesPerMacro;

   const uint microTileY = microTileIndex / macroTileWidth;
   const uint microTileX = microTileIndex % macroTileWidth;

   // Check that our tiles are inside the bounds described.  Note that additional
   // warps will actually overflow into the next slice...
   if (dispatchSliceIndex >= params.numSlices) {
      return;
   }

   // Calculate our overall slice index
   uint sliceIndex = baseSliceIndex + dispatchSliceIndex;

   // Calculate offset within thick slices for current slice
   const uint macroTilesPerSlice = macroTilesPerRow * macroTilesNumRows;
   const uint sliceOffset = (sliceIndex / microTileThickness) * macroTilesPerSlice * macroTileBytes;

   const uint macroTileIndex = (macroTileY * macroTilesPerRow) + macroTileX;
   const uint macroTileOffset = macroTileIndex * macroTileBytes;

   const uint sampleOffset = 0;
   const uint thickSliceOffset = (sliceIndex % microTileThickness) * (microTileBytes / microTileThickness);
   const uint totalOffset =
      ((sliceOffset + macroTileOffset) >> (NumBankBits + NumPipeBits))
      + sampleOffset + thickSliceOffset;

   const uint offsetHigh = (totalOffset & ~GroupMask) << (NumBankBits + NumPipeBits);
   const uint offsetLow = totalOffset & GroupMask;

   const uint pixelX = (macroTileX * macroTileWidth + microTileX) * MicroTileWidth;
   const uint pixelY = (macroTileY * macroTileHeight + microTileY) * MicroTileHeight;

   // Calculate our bank/pipe/sample rotations and swaps
   uint bankSliceRotation = 0;
   uint sampleSliceRotation = 0;
   uint pipeSliceRotation = 0;
   if (!IsMacro3D) {
      // 2_ format
      bankSliceRotation = ((NumBanks >> 1) - 1) * (sliceIndex / microTileThickness);
      sampleSliceRotation = ((NumBanks >> 1) + 1) * sampleIndex;
   } else {
      // 3_ format
      bankSliceRotation = (sliceIndex / microTileThickness) / NumPipes;
      pipeSliceRotation = (sliceIndex / microTileThickness);
   }

   uint bankSwapRotation = 0;
   if (IsBankSwapped) {
      const uint bankSwapOrder[] = { 0, 1, 3, 2 };
      const uint swapIndex = ((macroTileX * macroTileWidth * MicroTileWidth) / bankSwapWidth);
      bankSwapRotation = bankSwapOrder[swapIndex % NumBanks];
   }

   uint bank = ((pixelX >> 3) & 1) ^ ((pixelY >> 5) & 1);
   bank |= (((pixelX >> 4) & 1) ^ ((pixelY >> 4) & 1)) << 1;
   bank ^= (bankSwizzle + bankSliceRotation) & (NumBanks - 1);
   bank ^= sampleSliceRotation;
   bank ^= bankSwapRotation;

   uint pipe = ((pixelX >> 3) & 1) ^ ((pixelY >> 3) & 1);
   pipe ^= (pipeSwizzle + pipeSliceRotation) & (NumPipes - 1);

   const uint microTileOffset =
      (bank << (NumGroupBits + NumPipeBits))
      + (pipe << NumGroupBits) + offsetLow + offsetHigh;

   // Get ready to call the micro tiler
   uint untiledOffset = (pixelX * bytesPerElement) + (pixelY * untiledStride);
   uint tiledOffset = microTileOffset;

   // Shift forwards by the number of slices
   untiledOffset += sliceIndex * sliceBytes / microTileThickness;

   // Shift backwards by the base slices
   untiledOffset -= baseSliceIndex * sliceBytes / microTileThickness;
   tiledOffset -= tiledSliceIndex * sliceBytes / microTileThickness;

   if (IsDepth) {
      untileMicroTilingDepth(tiledOffset, untiledOffset, untiledStride);
   } else {
      if (BitsPerElement == 8) {
         untileMicroTiling8(tiledOffset, untiledOffset, untiledStride);
      } else if (BitsPerElement == 16) {
         untileMicroTiling16(tiledOffset, untiledOffset, untiledStride);
      } else if (BitsPerElement == 32) {
         untileMicroTiling32(tiledOffset, untiledOffset, untiledStride);
      } else if (BitsPerElement == 64) {
         untileMicroTiling64(tiledOffset, untiledOffset, untiledStride);
      } else if (BitsPerElement == 128) {
         untileMicroTiling128(tiledOffset, untiledOffset, untiledStride);
      }
   }
}

void main()
{
   if (IsMacroTiling) {
      mainMacroTiling();
   } else {
      mainMicroTiling();
   }
}