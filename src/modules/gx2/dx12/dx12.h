#pragma once

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <wrl.h>
#include "d3dx12.h"

using namespace DirectX;
using namespace Microsoft::WRL;

inline void ThrowIfFailed(HRESULT hr)
{
   if (FAILED(hr))
   {
      throw;
   }
}
