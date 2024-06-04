/// Vertex Shader

#include "scenebuffer.hlsli"

float3x3 inverse(float3x3 m) {
    // computes the inverse of a matrix m
    float det = m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2]) -
        m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
        m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);

    float invdet = det == 0.0f ? 0.0f : 1.0f / det;

    float3x3 minv; // inverse of matrix m
    minv[0][0] = (m[1][1] * m[2][2] - m[2][1] * m[1][2]) * invdet;
    minv[0][1] = (m[0][2] * m[2][1] - m[0][1] * m[2][2]) * invdet;
    minv[0][2] = (m[0][1] * m[1][2] - m[0][2] * m[1][1]) * invdet;
    minv[1][0] = (m[1][2] * m[2][0] - m[1][0] * m[2][2]) * invdet;
    minv[1][1] = (m[0][0] * m[2][2] - m[0][2] * m[2][0]) * invdet;
    minv[1][2] = (m[1][0] * m[0][2] - m[0][0] * m[1][2]) * invdet;
    minv[2][0] = (m[1][0] * m[2][1] - m[2][0] * m[1][1]) * invdet;
    minv[2][1] = (m[2][0] * m[0][1] - m[0][0] * m[2][1]) * invdet;
    minv[2][2] = (m[0][0] * m[1][1] - m[1][0] * m[0][1]) * invdet;

    return minv;
}

cbuffer ObjectModelCB : register(b0)
{
    matrix Model;
}

struct Vertex
{
    float3 Position : POSITION;
    float2 TextureCoord : TEXTURE_COORD;
    float3 Normal : NORMAL;
};

struct VertexShaderOutput
{
    float4 WorldPosition : WORLD_POSITION;
    float2 TextureCoord : TEXTURE_COORD;
    float3 Normal : NORMAL;
    float4 Position : SV_Position;
};

VertexShaderOutput main(Vertex IN)
{
    VertexShaderOutput OUT;

    OUT.WorldPosition = mul(float4(IN.Position, 1.0f), Model);
    matrix mvp = mul(Model, mul(View, Projection));
    OUT.Position = mul(float4(IN.Position, 1.0f), mvp);
    OUT.Normal = mul(IN.Normal, transpose(inverse((float3x3) Model)));
    OUT.TextureCoord = IN.TextureCoord;

    return OUT;
}

