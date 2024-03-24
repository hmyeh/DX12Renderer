/// Vertex Shader

cbuffer ModelViewProjectionCB : register(b0) 
{
    //matrix MVP;
    matrix Model;
    matrix View;
    matrix Projection;
}

struct Vertex
{
    float3 Position : POSITION;
    float2 TextureCoord : TEXTURE_COORD;
};

struct VertexShaderOutput
{
    float2 TextureCoord : TEXTURE_COORD;
    float4 Position : SV_Position;
};

VertexShaderOutput main(Vertex IN)
{
    VertexShaderOutput OUT;
    
    OUT.Position = mul(float4(IN.Position, 1.0f), mul(mul(Model, View), Projection));
    OUT.TextureCoord = IN.TextureCoord;

    return OUT;
}

