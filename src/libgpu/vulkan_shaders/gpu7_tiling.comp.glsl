#version 450

// Our specialization constants for controlling the type of shader
layout(constant_id = 0) const uint BitsPerElement = 8;
layout(constant_id = 1) const uint SubGroupSize = 32;
layout(constant_id = 2) const uint MacroTileWidth = 1;
layout(constant_id = 3) const uint MacroTileHeight = 1;
layout(constant_id = 4) const bool IsDepth = false;
layout(constant_id = 5) const bool IsUntiling = true;

// Specify our grouping setup
layout(local_size_x_id = 1, local_size_y = 1, local_size_z = 1) in;

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
const bool IsMacroTiling = MacroTileWidth > 1 || MacroTileHeight > 1;
const uint BytesPerElement = BitsPerElement / 8;

// Set up the shader inputs
layout(std430, binding = 0) buffer tiledBuffer { uint tiled[]; };
layout(std430, binding = 1) buffer untiledBuffer { uint untiled[]; };

layout(push_constant) uniform Parameters {
    // Micro tiling parameters
    uint untiledStride;
    uint microTileBytes;
    uint numTilesPerRow;
    uint numTileRows;
    uint tiledSliceIndex;
    uint sliceIndex;
    uint numSlices;
    uint sliceBytes;
    uint microTileThickness;

    // Macro tiling parameters
    uint sampleOffset; // NOT YET SUPPORTED
    uint tileMode;
    uint macroTileBytes;
    uint bankSwizzle;
    uint pipeSwizzle;
    uint bankSwapWidth;
} params;

void copyBytes(uint untiledOffset, uint tiledOffset, uint numBytes)
{
   if (IsUntiling) {
      for (int i = 0; i < numBytes / 4; ++i) {
         untiled[(untiledOffset / 4) + i] = tiled[(tiledOffset / 4) + i];
      }
   } else {
      for (int i = 0; i < numBytes / 4; ++i) {
         tiled[(tiledOffset / 4) + i] = untiled[(untiledOffset / 4) + i];
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

   for (int y = 0; y < MicroTileHeight; y += 4) {
      uint untiledRow0 = untiledOffset + 0 * untiledStride;
      uint untiledRow1 = untiledOffset + 1 * untiledStride;
      uint untiledRow2 = untiledOffset + 2 * untiledStride;
      uint untiledRow3 = untiledOffset + 3 * untiledStride;

      uint tiledRow0 = tiledOffset + 0 * tiledStride;
      uint tiledRow1 = tiledOffset + 1 * tiledStride;
      uint tiledRow2 = tiledOffset + 2 * tiledStride;
      uint tiledRow3 = tiledOffset + 3 * tiledStride;

      copyBytes(untiledRow0, tiledRow0, rowSize);
      copyBytes(untiledRow1, tiledRow2, rowSize);
      copyBytes(untiledRow2, tiledRow1, rowSize);
      copyBytes(untiledRow3, tiledRow3, rowSize);

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

   for (int y = 0; y < MicroTileHeight; ++y) {
      copyBytes(untiledOffset, tiledOffset, rowSize);

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

   for (int y = 0; y < MicroTileHeight; y += 2) {
      uint untiledRow1 = untiledOffset + 0 * untiledStride;
      uint untiledRow2 = untiledOffset + 1 * untiledStride;

      uint tiledRow1 = tiledOffset + 0 * tiledStride;
      uint tiledRow2 = tiledOffset + 1 * tiledStride;

      copyBytes(untiledRow1 + 0, tiledRow1 + 0, groupSize);
      copyBytes(untiledRow1 + 16, tiledRow2 + 0, groupSize);

      copyBytes(untiledRow2 + 0, tiledRow1 + 16, groupSize);
      copyBytes(untiledRow2 + 16, tiledRow2 + 16, groupSize);

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
   uint nextGroupOffset = tiledOffset + (0x100 << (NumBankBits + NumPipeBits));

   for (int y = 0; y < MicroTileHeight; y += 2) {
      if (IsMacroTiling && y == 4) {
         tiledOffset = nextGroupOffset;
      }

      uint untiledRow1 = untiledOffset + 0 * untiledStride;
      uint untiledRow2 = untiledOffset + 1 * untiledStride;

      uint tiledRow1 = tiledOffset + 0 * tiledStride;
      uint tiledRow2 = tiledOffset + 1 * tiledStride;

      copyBytes(untiledRow1 + 0, tiledRow1 + 0, groupBytes);
      copyBytes(untiledRow2 + 0, tiledRow1 + 16, groupBytes);

      copyBytes(untiledRow1 + 16, tiledRow1 + 32, groupBytes);
      copyBytes(untiledRow2 + 16, tiledRow1 + 48, groupBytes);

      copyBytes(untiledRow1 + 32, tiledRow2 + 0, groupBytes);
      copyBytes(untiledRow2 + 32, tiledRow2 + 16, groupBytes);

      copyBytes(untiledRow1 + 48, tiledRow2 + 32, groupBytes);
      copyBytes(untiledRow2 + 48, tiledRow2 + 48, groupBytes);

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

   for (int y = 0; y < MicroTileHeight; y += 2) {
      uint untiledRow1 = untiledOffset + 0 * untiledStride;
      uint untiledRow2 = untiledOffset + 1 * untiledStride;

      uint tiledRow1 = tiledOffset + 0 * tiledStride;
      uint tiledRow2 = tiledOffset + 1 * tiledStride;

      copyBytes(untiledRow1 + 0 * elemBytes, tiledRow1 + 0 * elemBytes, elemBytes);
      copyBytes(untiledRow1 + 1 * elemBytes, tiledRow1 + 2 * elemBytes, elemBytes);
      copyBytes(untiledRow2 + 0 * elemBytes, tiledRow1 + 1 * elemBytes, elemBytes);
      copyBytes(untiledRow2 + 1 * elemBytes, tiledRow1 + 3 * elemBytes, elemBytes);

      copyBytes(untiledRow1 + 2 * elemBytes, tiledRow1 + 4 * elemBytes, elemBytes);
      copyBytes(untiledRow1 + 3 * elemBytes, tiledRow1 + 6 * elemBytes, elemBytes);
      copyBytes(untiledRow2 + 2 * elemBytes, tiledRow1 + 5 * elemBytes, elemBytes);
      copyBytes(untiledRow2 + 3 * elemBytes, tiledRow1 + 7 * elemBytes, elemBytes);

      copyBytes(untiledRow1 + 4 * elemBytes, tiledRow2 + 0 * elemBytes, elemBytes);
      copyBytes(untiledRow1 + 5 * elemBytes, tiledRow2 + 2 * elemBytes, elemBytes);
      copyBytes(untiledRow2 + 4 * elemBytes, tiledRow2 + 1 * elemBytes, elemBytes);
      copyBytes(untiledRow2 + 5 * elemBytes, tiledRow2 + 3 * elemBytes, elemBytes);

      copyBytes(untiledRow1 + 6 * elemBytes, tiledRow2 + 4 * elemBytes, elemBytes);
      copyBytes(untiledRow1 + 7 * elemBytes, tiledRow2 + 6 * elemBytes, elemBytes);
      copyBytes(untiledRow2 + 6 * elemBytes, tiledRow2 + 5 * elemBytes, elemBytes);
      copyBytes(untiledRow2 + 7 * elemBytes, tiledRow2 + 7 * elemBytes, elemBytes);

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
   copyBytes(untiledOffset + uY * untiledStride + uX * groupBytes, tiledOffset + tY * tiledStride + tX * groupBytes, groupBytes);
}
void untileMicroTilingDepth(uint tiledOffset, uint untiledOffset, uint untiledStride)
{
   const uint tiledStride = MicroTileWidth * BytesPerElement;
   const uint groupBytes = 2 * BytesPerElement;

   for (int y = 0; y < 8; y += 4) {
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
   uint bytesPerElement = BytesPerElement;
   uint microTilesNumRows = params.numTileRows;
   uint microTilesPerRow = params.numTilesPerRow;
   uint untiledStride = params.untiledStride;
   uint microTileBytes = params.microTileBytes;
   uint sliceBytes = params.sliceBytes;
   uint tiledSliceIndex = params.tiledSliceIndex;
   uint baseSliceIndex = params.sliceIndex;
   uint microTileThickness = params.microTileThickness;

   uint tilesPerSlice = microTilesNumRows * microTilesPerRow;
   uint globalTileIndex = gl_GlobalInvocationID.x;

   uint dispatchSliceIndex = globalTileIndex / tilesPerSlice;
   uint sliceTileIndex = globalTileIndex % tilesPerSlice;

   uint microTileY = sliceTileIndex / microTilesPerRow;
   uint microTileX = sliceTileIndex % microTilesPerRow;

   // Check that our tiles are inside the bounds described.  Note that additional
   // warps will actually overflow into the next slice...
   if (dispatchSliceIndex >= params.numSlices) {
      return;
   }

   uint sliceIndex = baseSliceIndex + dispatchSliceIndex;

   uint microTileIndexZ = sliceIndex / microTileThickness;
   uint sliceOffset = microTileIndexZ * sliceBytes;

   // Calculate offset within thick slices for current slice
   uint thickSliceOffset = 0;
   if (microTileThickness > 1 && sliceIndex > 0) {
      thickSliceOffset = (sliceIndex % microTileThickness) * (microTileBytes / microTileThickness);
   }

   uint pixelX = microTileX * MicroTileWidth;
   uint pixelY = microTileY * MicroTileHeight;

   uint sampleOffset = 0;
   uint sampleSliceOffset = sliceOffset + sampleOffset + thickSliceOffset;

   uint tiledOffset = sampleSliceOffset + (microTileX + (microTileY * microTilesPerRow)) * microTileBytes;
   uint untiledOffset = (sliceIndex * sliceBytes / microTileThickness) + (pixelX * bytesPerElement) + (pixelY * untiledStride);

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
   uint macroTilesNumRows = params.numTileRows;
   uint macroTilesPerRow = params.numTilesPerRow;
   uint untiledStride = params.untiledStride;
   uint microTileBytes = params.microTileBytes;
   uint sliceBytes = params.sliceBytes;
   uint tiledSliceIndex = params.tiledSliceIndex;
   uint baseSliceIndex = params.sliceIndex;
   uint microTileThickness = params.microTileThickness;
   uint tileMode = params.tileMode;
   uint macroTileBytes = params.macroTileBytes;
   uint bankSwizzle = params.bankSwizzle;
   uint pipeSwizzle = params.pipeSwizzle;
   uint bankSwapWidth = params.bankSwapWidth;

   // We do not currently support samples.
   uint sampleIndex = 0;


   // uint macroTileWidth
   // const uint microTilesPerMacro
   uint microTilesPerMacroRow = microTilesPerMacro * macroTilesPerRow;
   uint microTilesPerSlice = microTilesPerMacroRow * macroTilesNumRows;
   uint globalTileIndex = gl_GlobalInvocationID.x;

   uint dispatchSliceIndex = globalTileIndex / microTilesPerSlice;
   uint macroMicroTileIndex = (globalTileIndex % microTilesPerSlice);

   uint macroTileY = macroMicroTileIndex / microTilesPerMacroRow;
   uint macroRowTileIndex = macroMicroTileIndex % microTilesPerMacroRow;

   uint macroTileX = macroRowTileIndex / microTilesPerMacro;
   uint microTileIndex = macroRowTileIndex % microTilesPerMacro;

   uint microTileY = microTileIndex / macroTileWidth;
   uint microTileX = microTileIndex % macroTileWidth;

   // Check that our tiles are inside the bounds described.  Note that additional
   // warps will actually overflow into the next slice...
   if (dispatchSliceIndex >= params.numSlices) {
      return;
   }

   uint sliceIndex = baseSliceIndex + dispatchSliceIndex;

   // Calculate offset within thick slices for current slice
   uint thickSliceOffset = 0;
   if (microTileThickness > 1 && sliceIndex > 0) {
      thickSliceOffset = (sliceIndex % microTileThickness) * (microTileBytes / microTileThickness);
   }

   // Calculate our bank/pipe/sample rotations and swaps
   uint bankSliceRotation = 0;
   uint sampleSliceRotation = 0;
   uint pipeSliceRotation = 0;

   if (tileMode >= 4 && tileMode <= 11) {
      // 2_ format
      bankSliceRotation = ((NumBanks >> 1) - 1) * (sliceIndex / microTileThickness);
      sampleSliceRotation = ((NumBanks >> 1) + 1) * sampleIndex;
   } else if (tileMode >= 12 && tileMode <= 15) {
      // 3_ format
      bankSliceRotation = (sliceIndex / microTileThickness) / NumPipes;
      pipeSliceRotation = (sliceIndex / microTileThickness);
   }

   const uint macroTilesPerSlice = macroTilesPerRow * macroTilesNumRows;
   uint sliceOffset = (sliceIndex / microTileThickness) * macroTilesPerSlice * macroTileBytes;

   uint macroTileIndex = (macroTileY * macroTilesPerRow) + macroTileX;
   uint macroTileOffset = macroTileIndex * macroTileBytes;

   const uint sampleOffset = 0;
   const uint totalOffset =
      ((sliceOffset + macroTileOffset) >> (NumBankBits + NumPipeBits))
      + sampleOffset + thickSliceOffset;
   uint offsetHigh = (totalOffset & ~GroupMask) << (NumBankBits + NumPipeBits);
   uint offsetLow = totalOffset & GroupMask;

   uint bankSwapRotation = 0;
   if (bankSwapWidth > 0) {
      const uint bankSwapOrder[] = { 0, 1, 3, 2 };
      const uint swapIndex = ((macroTileX * macroTileWidth * MicroTileWidth) / bankSwapWidth);
      bankSwapRotation = bankSwapOrder[swapIndex % NumBanks];
   }

   uint pixelX = (macroTileX * macroTileWidth + microTileX) * MicroTileWidth;
   uint pixelY = (macroTileY * macroTileHeight + microTileY) * MicroTileHeight;
   uint untiledOffset = (pixelX * bytesPerElement) + (pixelY * untiledStride);

   uint bank = ((pixelX >> 3) & 1) ^ ((pixelY >> 5) & 1);
   bank |= (((pixelX >> 4) & 1) ^ ((pixelY >> 4) & 1)) << 1;
   bank ^= (bankSwizzle + bankSliceRotation) & (NumBanks - 1);
   bank ^= sampleSliceRotation;
   bank ^= bankSwapRotation;

   uint pipe = ((pixelX >> 3) & 1) ^ ((pixelY >> 3) & 1);
   pipe ^= (pipeSwizzle + pipeSliceRotation) & (NumPipes - 1);

   uint microTileOffset =
      (bank << (NumGroupBits + NumPipeBits))
      + (pipe << NumGroupBits) + offsetLow + offsetHigh;

   // Shift forwards by the number of slices
   untiledOffset += sliceIndex * sliceBytes / microTileThickness;

   // Shift backwards by the base slices
   untiledOffset -= baseSliceIndex * sliceBytes / microTileThickness;
   microTileOffset -= tiledSliceIndex * sliceBytes / microTileThickness;

   if (IsDepth) {
      untileMicroTilingDepth(microTileOffset, untiledOffset, untiledStride);
   } else {
      if (BitsPerElement == 8) {
         untileMicroTiling8(microTileOffset, untiledOffset, untiledStride);
      } else if (BitsPerElement == 16) {
         untileMicroTiling16(microTileOffset, untiledOffset, untiledStride);
      } else if (BitsPerElement == 32) {
         untileMicroTiling32(microTileOffset, untiledOffset, untiledStride);
      } else if (BitsPerElement == 64) {
         untileMicroTiling64(microTileOffset, untiledOffset, untiledStride);
      } else if (BitsPerElement == 128) {
         untileMicroTiling128(microTileOffset, untiledOffset, untiledStride);
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