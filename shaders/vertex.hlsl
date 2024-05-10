/// Vertex Shader

cbuffer ObjectModelCB : register(b0)
{
    matrix ObjectModel;
}

cbuffer ModelViewProjectionCB : register(b1) 
{
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
    
    //matrix mvp = mul(mul(mul(Projection, View), Model), ObjectModel);
    //OUT.Position = mul(mvp, float4(IN.Position, 1.0f));

    matrix mvp = mul(ObjectModel, mul(Model, mul(View, Projection)));
    OUT.Position = mul(float4(IN.Position, 1.0f), mvp);

    //OUT.Position = mul(float4(IN.Position, 1.0f), mul(mul(Model, View), Projection));
    OUT.TextureCoord = IN.TextureCoord;

    return OUT;
}

