/// Vertex Shader

cbuffer ModelViewProjectionCB : register(b0) 
{
    //matrix MVP;
    matrix Model;
    matrix View;
    matrix Projection;
}

struct VertexPosColor
{
    float3 Position : POSITION;
    float3 Color : COLOR;
};

struct VertexShaderOutput
{
    float4 Color : COLOR;
    float4 Position : SV_Position;
};

VertexShaderOutput main(VertexPosColor IN)
{
    VertexShaderOutput OUT;
    
    OUT.Position = mul(float4(IN.Position, 1.0f), mul(mul(Model, View), Projection));
    OUT.Color = float4(IN.Color, 1.0f);

    return OUT;
}

