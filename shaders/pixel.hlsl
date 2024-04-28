//// Pixel Shader

Texture2D diffuseMap : register(t0);

SamplerState sampleWrap : register(s0);
SamplerState sampleClamp : register(s1);

struct PixelShaderInput
{
    float2 TextureCoord : TEXTURE_COORD;
};

float4 main(PixelShaderInput IN) : SV_TARGET
{
	return diffuseMap.Sample(sampleWrap, IN.TextureCoord);
}
