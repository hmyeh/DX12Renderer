// Vertex Shader

struct VSInput {
	float2 Position : POSITION;
	float2 TextureCoord : TEXTURE_COORD;
};

struct VSOutput
{
	float2 TextureCoord : TEXTURE_COORD;
	float4 Position : SV_Position;
};

VSOutput main(VSInput IN)
{
	VSOutput OUT;

	OUT.TextureCoord = IN.TextureCoord;
	OUT.Position = float4(IN.Position, 0.0, 1.0);

	return OUT;
}