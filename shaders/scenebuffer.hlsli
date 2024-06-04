
struct DirectionalLight
{
    float4 Direction;
    float4 Color;
    float4x4 LightSpaceMatrix;
};

cbuffer SceneCB : register(b2)
{
    matrix View;
    matrix Projection;
    float4 CameraPosition;
    DirectionalLight DirLight;
}
