struct VSInput
{
   float2 position : POSITION;
   float2 uv : TEXCOORD;
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

   result.position = float4(input.position.xy, 0, 1);
   result.uv = input.uv;

   return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
   return g_texture.Sample(g_sampler, input.uv);
}