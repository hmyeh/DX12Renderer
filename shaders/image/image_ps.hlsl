// Pixel Shader

Texture2D screenMap : register(t0);

SamplerState sampleWrap : register(s0);
SamplerState sampleClamp : register(s1);

struct PSInput {
	float2 TextureCoord : TEXTURE_COORD;
};

float4 main(PSInput IN) : SV_Target
{
	// for now constants here but should be passed along
	float gamma = 2.2;
	float inv_gamma = 1.0 / gamma;
	float exposure = 1.0;

    float4 color = screenMap.Sample(sampleWrap, IN.TextureCoord);
	// Simple exposure Tone mapping HDR
	float3 mapped = float3(1.0, 1.0, 1.0) - exp(-color.rgb * exposure);
	// Gamma correction
	mapped = pow(mapped, float3(inv_gamma, inv_gamma, inv_gamma));
	return float4(mapped, 1.0);
}