
cbuffer ObjectModelCB : register(b0)
{
    matrix Model;
}

struct DirectionalLight
{
    float4 Direction;
    float4 Color;
    float4x4 LightSpaceMatrix;
};

cbuffer SceneCB : register(b1)
{
    matrix View;
    matrix Projection;
    float4 CameraPosition;
    DirectionalLight DirLight;
}

struct Vertex
{
    float3 Position : POSITION;
    float2 TextureCoord : TEXTURE_COORD;
    float3 Normal : NORMAL;
};

float4 main(Vertex IN) : SV_POSITION
{
    matrix mvp = mul(Model, DirLight.LightSpaceMatrix);
    float4 pos = mul(float4(IN.Position, 1.0f), Model);
    pos = mul(pos, DirLight.LightSpaceMatrix);
    return pos;
}