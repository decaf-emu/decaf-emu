struct VSInput
{
   float4 position : POSITION;
};

struct PSInput
{
   float4 position : SV_POSITION;
};

PSInput VSMain(VSInput input)
{
   PSInput result;

   result.position = input.position;

   return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
   return float4(1, 1, 1, 1);
}