struct VSInput
{
   float4 position : POSITION;
   float4 uv : TEXCOORD;
};

struct PSInput
{
   float4 position : SV_POSITION;
   float2 uv : TEXCOORD;
};

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

PSInput VSMain(VSInput input)
{
   PSInput result;

   result.position = input.position;
   result.uv = input.uv;

   return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
   return g_texture.Sample(g_sampler, input.uv);
}