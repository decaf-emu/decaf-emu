#version 450

layout(constant_id = 0) const bool IsUntiling = true;
layout(constant_id = 1) const uint MicroTileThickness = 1;
layout(constant_id = 2) const uint MacroTileWidth = 1;
layout(constant_id = 3) const uint MacroTileHeight = 1;
layout(constant_id = 4) const bool IsMacro3X = false;
layout(constant_id = 5) const bool IsBankSwapped = false;
layout(constant_id = 6) const uint BitsPerElement = 8;
layout(constant_id = 7) const bool IsDepth = false;
#ifndef DECAF_MVK_COMPAT
layout(constant_id = 8) const uint SubGroupSize = 32;
#endif

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

// Setup some convenience information based on our constants
const bool IsMacroTiling = (MacroTileWidth > 1 || MacroTileHeight > 1);
const uint BytesPerElement = BitsPerElement / 8;
const uint MicroTileBytes = MicroTileWidth * MicroTileHeight * MicroTileThickness * BytesPerElement;
const uint MacroTileBytes = MacroTileWidth * MacroTileHeight * MicroTileBytes;

layout(push_constant) uniform Parameters {
   uint firstSliceIndex;
   uint maxTiles;

   // Micro tiling parameters
   uint numTilesPerRow;
   uint numTilesPerSlice;
   uint thinMicroTileBytes;
   uint thickSliceBytes;

   // Macro tiling parameters
   uint bankSwizzle;
   uint pipeSwizzle;
   uint bankSwapWidth;
} params;

// Set up the shader inputs
layout(std430, binding = 0) buffer tiledBuffer4 { uint tiled4[]; };
layout(std430, binding = 1) buffer untiledBuffer4 { uint untiled4[]; };

void copyElems4(uint untiledOffset, uint tiledOffset, uint numElems)
{
   if (IsUntiling) {
       for (uint i = 0; i < numElems; ++i) {
         untiled4[(untiledOffset / 4) + i] = tiled4[(tiledOffset / 4) + i];
      }
   } else {
       for (uint i = 0; i < numElems; ++i) {
         tiled4[(tiledOffset / 4) + i] = untiled4[(untiledOffset / 4) + i];
      }
   }
}

#ifndef DECAF_MVK_COMPAT
layout(std430, binding = 0) buffer tiledBuffer8 { uvec2 tiled8[]; };
layout(std430, binding = 1) buffer untiledBuffer8 { uvec2 untiled8[]; };

layout(std430, binding = 0) buffer tiledBuffer16 { uvec4 tiled16[]; };
layout(std430, binding = 1) buffer untiledBuffer16 { uvec4 untiled16[]; };

void copyElems8(uint untiledOffset, uint tiledOffset, uint numElems)
{
   if (IsUntiling) {
       for (uint i = 0; i < numElems; ++i) {
         untiled8[(untiledOffset / 8) + i] = tiled8[(tiledOffset / 8) + i];
      }
   } else {
       for (uint i = 0; i < numElems; ++i) {
         tiled8[(tiledOffset / 8) + i] = untiled8[(untiledOffset / 8) + i];
      }
   }
}

void copyElems16(uint untiledOffset, uint tiledOffset, uint numElems)
{
   if (IsUntiling) {
       for (uint i = 0; i < numElems; ++i) {
         untiled16[(untiledOffset / 16)] = tiled16[(tiledOffset / 16)];
      }
   } else {
       for (uint i = 0; i < numElems; ++i) {
         tiled16[(tiledOffset / 16)] = untiled16[(untiledOffset / 16)];
      }
   }
}
#else
void copyElems8(uint untiledOffset, uint tiledOffset, uint numElems)
{
   copyElems4(untiledOffset, tiledOffset, numElems * 2);
}

void copyElems16(uint untiledOffset, uint tiledOffset, uint numElems)
{
   copyElems4(untiledOffset, tiledOffset, numElems * 4);
}
#endif

void retileMicro8(uint tiledOffset, uint untiledOffset, uint untiledStride)
{
   const uint tiledStride = MicroTileWidth;
   const uint rowElems = MicroTileWidth / 8;

   for (uint y = 0; y < MicroTileHeight; y += 4) {
      const uint untiledRow0 = untiledOffset + 0 * untiledStride;
      const uint untiledRow1 = untiledOffset + 1 * untiledStride;
      const uint untiledRow2 = untiledOffset + 2 * untiledStride;
      const uint untiledRow3 = untiledOffset + 3 * untiledStride;

      const uint tiledRow0 = tiledOffset + 0 * tiledStride;
      const uint tiledRow1 = tiledOffset + 1 * tiledStride;
      const uint tiledRow2 = tiledOffset + 2 * tiledStride;
      const uint tiledRow3 = tiledOffset + 3 * tiledStride;

      copyElems8(untiledRow0, tiledRow0, rowElems);
      copyElems8(untiledRow1, tiledRow2, rowElems);
      copyElems8(untiledRow2, tiledRow1, rowElems);
      copyElems8(untiledRow3, tiledRow3, rowElems);

      untiledOffset += 4 * untiledStride;
      tiledOffset += 4 * tiledStride;
   }
}

void retileMicro16(uint tiledOffset, uint untiledOffset, uint untiledStride)
{
   const uint tiledStride = MicroTileWidth * 2;
   const uint rowElems = MicroTileWidth * 2 / 16;

   for (uint y = 0; y < MicroTileHeight; ++y) {
      copyElems16(untiledOffset, tiledOffset, rowElems);

      untiledOffset += untiledStride;
      tiledOffset += tiledStride;
   }
}

void retileMicro32(uint tiledOffset, uint untiledOffset, uint untiledStride)
{
   const uint tiledStride = MicroTileWidth * 4;
   const uint groupElems = 4 * 4 / 16;

   for (uint y = 0; y < MicroTileHeight; y += 2) {
      const uint untiledRow1 = untiledOffset + 0 * untiledStride;
      const uint untiledRow2 = untiledOffset + 1 * untiledStride;

      const uint tiledRow1 = tiledOffset + 0 * tiledStride;
      const uint tiledRow2 = tiledOffset + 1 * tiledStride;

      copyElems16(untiledRow1 + 0, tiledRow1 + 0, groupElems);
      copyElems16(untiledRow1 + 16, tiledRow2 + 0, groupElems);

      copyElems16(untiledRow2 + 0, tiledRow1 + 16, groupElems);
      copyElems16(untiledRow2 + 16, tiledRow2 + 16, groupElems);

      tiledOffset += tiledStride * 2;
      untiledOffset += untiledStride * 2;
   }
}

void retileMicro64(uint tiledOffset, uint untiledOffset, uint untiledStride)
{
   const uint tiledStride = MicroTileWidth * 8;
   const uint groupElems = 2 * (64 / 8) / 16;

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

      copyElems16(untiledRow1 + 0, tiledRow1 + 0, groupElems);
      copyElems16(untiledRow2 + 0, tiledRow1 + 16, groupElems);

      copyElems16(untiledRow1 + 16, tiledRow1 + 32, groupElems);
      copyElems16(untiledRow2 + 16, tiledRow1 + 48, groupElems);

      copyElems16(untiledRow1 + 32, tiledRow2 + 0, groupElems);
      copyElems16(untiledRow2 + 32, tiledRow2 + 16, groupElems);

      copyElems16(untiledRow1 + 48, tiledRow2 + 32, groupElems);
      copyElems16(untiledRow2 + 48, tiledRow2 + 48, groupElems);

      tiledOffset += tiledStride * 2;
      untiledOffset += untiledStride * 2;
   }
}

void retileMicro128(uint tiledOffset, uint untiledOffset, uint untiledStride)
{
   const uint tiledStride = MicroTileWidth * 16;
   const uint groupBytes = 16;
   const uint groupElems = 16 / 16;

   for (uint y = 0; y < MicroTileHeight; y += 2) {
      const uint untiledRow1 = untiledOffset + 0 * untiledStride;
      const uint untiledRow2 = untiledOffset + 1 * untiledStride;

      const uint tiledRow1 = tiledOffset + 0 * tiledStride;
      const uint tiledRow2 = tiledOffset + 1 * tiledStride;

      copyElems16(untiledRow1 + 0 * groupBytes, tiledRow1 + 0 * groupBytes, groupElems);
      copyElems16(untiledRow1 + 1 * groupBytes, tiledRow1 + 2 * groupBytes, groupElems);
      copyElems16(untiledRow2 + 0 * groupBytes, tiledRow1 + 1 * groupBytes, groupElems);
      copyElems16(untiledRow2 + 1 * groupBytes, tiledRow1 + 3 * groupBytes, groupElems);

      copyElems16(untiledRow1 + 2 * groupBytes, tiledRow1 + 4 * groupBytes, groupElems);
      copyElems16(untiledRow1 + 3 * groupBytes, tiledRow1 + 6 * groupBytes, groupElems);
      copyElems16(untiledRow2 + 2 * groupBytes, tiledRow1 + 5 * groupBytes, groupElems);
      copyElems16(untiledRow2 + 3 * groupBytes, tiledRow1 + 7 * groupBytes, groupElems);

      copyElems16(untiledRow1 + 4 * groupBytes, tiledRow2 + 0 * groupBytes, groupElems);
      copyElems16(untiledRow1 + 5 * groupBytes, tiledRow2 + 2 * groupBytes, groupElems);
      copyElems16(untiledRow2 + 4 * groupBytes, tiledRow2 + 1 * groupBytes, groupElems);
      copyElems16(untiledRow2 + 5 * groupBytes, tiledRow2 + 3 * groupBytes, groupElems);

      copyElems16(untiledRow1 + 6 * groupBytes, tiledRow2 + 4 * groupBytes, groupElems);
      copyElems16(untiledRow1 + 7 * groupBytes, tiledRow2 + 6 * groupBytes, groupElems);
      copyElems16(untiledRow2 + 6 * groupBytes, tiledRow2 + 5 * groupBytes, groupElems);
      copyElems16(untiledRow2 + 7 * groupBytes, tiledRow2 + 7 * groupBytes, groupElems);

      if (IsMacroTiling) {
         tiledOffset += 0x100 << (NumBankBits + NumPipeBits);
      } else {
         tiledOffset += tiledStride * 2;
      }

      untiledOffset += untiledStride * 2;
   }
}

void copyDepthXYGroup(uint tiledOffset, uint untiledOffset, uint untiledStride,
                      uint tX, uint tY, uint uX, uint uY)
{
   const uint groupBytes = 2 * BytesPerElement;
   const uint tiledStride = MicroTileWidth * BytesPerElement;

   copyElems4(untiledOffset + uY * untiledStride + uX * groupBytes, tiledOffset + tY * tiledStride + tX * groupBytes, groupBytes);
}
void retileMicroDepth(uint tiledOffset, uint untiledOffset, uint untiledStride)
{
   for (uint y = 0; y < MicroTileHeight; y += 4) {
      copyDepthXYGroup(tiledOffset, untiledOffset, untiledStride, 0, y + 0, 0, y + 0);
      copyDepthXYGroup(tiledOffset, untiledOffset, untiledStride, 1, y + 0, 0, y + 1);
      copyDepthXYGroup(tiledOffset, untiledOffset, untiledStride, 2, y + 0, 1, y + 0);
      copyDepthXYGroup(tiledOffset, untiledOffset, untiledStride, 3, y + 0, 1, y + 1);

      copyDepthXYGroup(tiledOffset, untiledOffset, untiledStride, 0, y + 1, 0, y + 2);
      copyDepthXYGroup(tiledOffset, untiledOffset, untiledStride, 1, y + 1, 0, y + 3);
      copyDepthXYGroup(tiledOffset, untiledOffset, untiledStride, 2, y + 1, 1, y + 2);
      copyDepthXYGroup(tiledOffset, untiledOffset, untiledStride, 3, y + 1, 1, y + 3);

      copyDepthXYGroup(tiledOffset, untiledOffset, untiledStride, 0, y + 2, 2, y + 0);
      copyDepthXYGroup(tiledOffset, untiledOffset, untiledStride, 1, y + 2, 2, y + 1);
      copyDepthXYGroup(tiledOffset, untiledOffset, untiledStride, 2, y + 2, 3, y + 0);
      copyDepthXYGroup(tiledOffset, untiledOffset, untiledStride, 3, y + 2, 3, y + 1);

      copyDepthXYGroup(tiledOffset, untiledOffset, untiledStride, 0, y + 3, 2, y + 2);
      copyDepthXYGroup(tiledOffset, untiledOffset, untiledStride, 1, y + 3, 2, y + 3);
      copyDepthXYGroup(tiledOffset, untiledOffset, untiledStride, 2, y + 3, 3, y + 2);
      copyDepthXYGroup(tiledOffset, untiledOffset, untiledStride, 3, y + 3, 3, y + 3);
   }
}

void mainMicroTiling(uint tileIndex)
{
   const uint thinSliceBytes = params.thickSliceBytes / MicroTileThickness;
   const uint untiledStride = params.numTilesPerRow * MicroTileWidth * BytesPerElement;
   const uint thickMicroTileBytes = params.thinMicroTileBytes * MicroTileThickness;

   const uint dispatchSliceIndex = tileIndex / params.numTilesPerSlice;
   const uint sliceTileIndex = tileIndex % params.numTilesPerSlice;

   // Find the global slice index we are currently at.
   const uint srcSliceIndex = params.firstSliceIndex + dispatchSliceIndex;

   // We need to identify where inside the current thick slice we are.
   const uint localSliceIndex = srcSliceIndex % MicroTileThickness;

   // Calculate the offset to our untiled data starting from the thick slice
   const uint srcTileY = sliceTileIndex / params.numTilesPerRow;
   const uint srcTileX = sliceTileIndex % params.numTilesPerRow;

   uint untiledOffset =
      (localSliceIndex * thinSliceBytes) +
      (srcTileX * MicroTileWidth * BytesPerElement) +
      (srcTileY * params.numTilesPerRow * params.thinMicroTileBytes);

   // Calculate the offset to our tiled data starting from the thick slice
   uint tiledOffset =
      (localSliceIndex * params.thinMicroTileBytes) +
      sliceTileIndex * thickMicroTileBytes;

   // In the case that we are using thick micro tiles, we need to advance our
     // offsets to the current thick slice boundary that we are at.
   const uint firstThickSliceIndex = params.firstSliceIndex / MicroTileThickness;
   const uint thickSliceIndex = srcSliceIndex / MicroTileThickness;
   const uint thickSliceOffset = (thickSliceIndex - firstThickSliceIndex) * params.thickSliceBytes;
   tiledOffset += thickSliceOffset;
   untiledOffset += thickSliceOffset;

   // The untiled pointers are offset forward by the local slice index already,
   // we need to back it up since our calculations above consider it.
   const uint firstThinSliceIndex = params.firstSliceIndex % MicroTileThickness;
   untiledOffset -= firstThinSliceIndex * thinSliceBytes;

   if (IsDepth) {
      retileMicroDepth(tiledOffset, untiledOffset, untiledStride);
   } else {
      if (BitsPerElement == 8) {
         retileMicro8(tiledOffset, untiledOffset, untiledStride);
      } else if (BitsPerElement == 16) {
         retileMicro16(tiledOffset, untiledOffset, untiledStride);
      } else if (BitsPerElement == 32) {
         retileMicro32(tiledOffset, untiledOffset, untiledStride);
      } else if (BitsPerElement == 64) {
         retileMicro64(tiledOffset, untiledOffset, untiledStride);
      } else if (BitsPerElement == 128) {
         retileMicro128(tiledOffset, untiledOffset, untiledStride);
      }
   }
}

void mainMacroTiling(uint tileIndex)
{
   const uint thinSliceBytes = params.thickSliceBytes / MicroTileThickness;
   const uint untiledStride = params.numTilesPerRow * MicroTileWidth * BytesPerElement;

   const uint dispatchSliceIndex = tileIndex / params.numTilesPerSlice;
   const uint sliceTileIndex = tileIndex % params.numTilesPerSlice;

   // Find the global slice index we are currently at.
   const uint srcSliceIndex = params.firstSliceIndex + dispatchSliceIndex;

   // We need to identify where inside the current thick slice we are.
   const uint localSliceIndex = srcSliceIndex % MicroTileThickness;

   // Calculate the thickSliceIndex
   const uint thickSliceIndex = srcSliceIndex / MicroTileThickness;

   // Calculate our tile positions
   const uint microTilesPerMacro = MacroTileWidth * MacroTileHeight;
   const uint macroTilesPerRow = params.numTilesPerRow / MacroTileWidth;
   const uint microTilesPerMacroRow = microTilesPerMacro * macroTilesPerRow;

   const uint srcMacroTileY = sliceTileIndex / microTilesPerMacroRow;
   const uint macroRowTileIndex = sliceTileIndex % microTilesPerMacroRow;

   const uint srcMacroTileX = macroRowTileIndex / microTilesPerMacro;
   const uint microTileIndex = macroRowTileIndex % microTilesPerMacro;

   const uint srcMicroTileY = microTileIndex / MacroTileWidth;
   const uint srcMicroTileX = microTileIndex % MacroTileWidth;

   const uint srcTileX = srcMacroTileX * MacroTileWidth + srcMicroTileX;
   const uint srcTileY = srcMacroTileY * MacroTileHeight + srcMicroTileY;

   // Figure out what our untiled offset shall be
   uint untiledOffset =
      (localSliceIndex * thinSliceBytes) +
      (srcTileX * MicroTileWidth * BytesPerElement) +
      (srcTileY * MicroTileHeight * untiledStride);

   // Calculate the offset to our untiled data starting from the thick slice
   const uint macroTileIndex = (srcMacroTileY * macroTilesPerRow) + srcMacroTileX;
   const uint macroTileOffset = macroTileIndex * MacroTileBytes;
   const uint tiledBaseOffset =
      (macroTileOffset >> (NumBankBits + NumPipeBits)) +
      (localSliceIndex * params.thinMicroTileBytes);

   const uint offsetHigh = (tiledBaseOffset & ~GroupMask) << (NumBankBits + NumPipeBits);
   const uint offsetLow = tiledBaseOffset & GroupMask;

   // Calculate our bank/pipe/sample rotations and swaps
   uint bankSliceRotation = 0;
   uint pipeSliceRotation = 0;
   if (!IsMacro3X) {
      // 2_ format
      bankSliceRotation = ((NumBanks >> 1) - 1)* thickSliceIndex;
   } else {
      // 3_ format
      bankSliceRotation = thickSliceIndex / NumPipes;
      pipeSliceRotation = thickSliceIndex;
   }

   uint bankSwapRotation = 0;
   if (IsBankSwapped) {
      const uint bankSwapOrder[] = { 0, 1, 3, 2 };
      const uint swapIndex = ((srcMacroTileX * MicroTileWidth * MacroTileWidth) / params.bankSwapWidth);
      bankSwapRotation = bankSwapOrder[swapIndex % NumBanks];
   }

   uint bank = 0;
   bank |= ((srcTileX >> 0) & 1) ^ ((srcTileY >> 2) & 1);
   bank |= (((srcTileX >> 1) & 1) ^ ((srcTileY >> 1) & 1)) << 1;
   bank ^= (params.bankSwizzle + bankSliceRotation) & (NumBanks - 1);
   bank ^= bankSwapRotation;

   uint pipe = 0;
   pipe |= ((srcTileX >> 0) & 1) ^ ((srcTileY >> 0) & 1);
   pipe ^= (params.pipeSwizzle + pipeSliceRotation) & (NumPipes - 1);

   uint tiledOffset =
      (bank << (NumGroupBits + NumPipeBits)) |
      (pipe << NumGroupBits) |
      offsetLow | offsetHigh;

   // In the case that we are using thick micro tiles, we need to advance our
   // offsets to the current thick slice boundary that we are at.
   const uint firstThickSliceIndex = params.firstSliceIndex / MicroTileThickness;
   const uint thickSliceOffset = (thickSliceIndex - firstThickSliceIndex) * params.thickSliceBytes;
   tiledOffset += thickSliceOffset;
   untiledOffset += thickSliceOffset;

   // The untiled pointers are offset forward by the local slice index already,
   // we need to back it up since our calculations above consider it.
   const uint firstThinSliceIndex = params.firstSliceIndex % MicroTileThickness;
   untiledOffset -= firstThinSliceIndex * thinSliceBytes;

   if (IsDepth) {
      retileMicroDepth(tiledOffset, untiledOffset, untiledStride);
   } else {
      if (BitsPerElement == 8) {
         retileMicro8(tiledOffset, untiledOffset, untiledStride);
      } else if (BitsPerElement == 16) {
         retileMicro16(tiledOffset, untiledOffset, untiledStride);
      } else if (BitsPerElement == 32) {
         retileMicro32(tiledOffset, untiledOffset, untiledStride);
      } else if (BitsPerElement == 64) {
         retileMicro64(tiledOffset, untiledOffset, untiledStride);
      } else if (BitsPerElement == 128) {
         retileMicro128(tiledOffset, untiledOffset, untiledStride);
      }
   }
}

void main()
{
   const uint tileIndex = gl_GlobalInvocationID.x;
   if (tileIndex >= params.maxTiles) {
      return;
   }

   if (IsMacroTiling) {
      mainMacroTiling(tileIndex);
   } else {
      mainMicroTiling(tileIndex);
   }
}