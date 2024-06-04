//// Pixel Shader

#include "scenebuffer.hlsli"

Texture2D depthMap : register(t0);
Texture2D diffuseMap : register(t1);

SamplerState sampleWrap : register(s0);
SamplerState sampleClamp : register(s1);

cbuffer MaterialCB : register(b1) 
{
    float metallic;
    float roughness;
}

struct PixelShaderInput
{
    float4 WorldPosition : WORLD_POSITION;
    float2 TextureCoord : TEXTURE_COORD;
	float3 Normal : NORMAL;
	float4 Position : SV_Position;
};


static const float PI = 3.14159265359;


// Shadow functions

float DirLightShadowCalculation(float4 worldPos)
{
    float4 fragPosLightSpace = mul(worldPos, DirLight.LightSpaceMatrix);
    // perform perspective divide
    fragPosLightSpace.xyz /= fragPosLightSpace.w;
    // transform to NDC
    float2 shadowTexCoords = fragPosLightSpace.xy * 0.5 + 0.5;
    shadowTexCoords.y = 1.0 - shadowTexCoords.y;

    float bias = 0.005;
    float currentDepth = fragPosLightSpace.z - bias;
    
    // Simple Percentage-Closer Filtering 
    float shadow = 0.0;
    float w;
    float h;
    depthMap.GetDimensions(w, h);
    float2 texelSize = float2(1.0 / w, 1.0 / h);
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float pcfDepth = depthMap.Sample(sampleClamp, shadowTexCoords + float2(x, y) * texelSize).x;
            shadow += currentDepth > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    if (fragPosLightSpace.z > 1.0)
        shadow = 0.0;

    return shadow;
}

// PBR functions

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float3 fresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    float inv_roughness = 1.0 - roughness;
    return F0 + (max(float3(inv_roughness, inv_roughness, inv_roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}


// Cook-terrance BRDF (expect normalized vectors)
float3 computeBRDF(float3 F0, float3 albedo, float roughness, float metallic, float3 L, float3 V, float3 N) {
    float3 H = normalize(V + L);
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    float3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    float3 kS = F;
    float3 kD = float3(1.0, 1.0, 1.0) - kS;
    kD *= 1.0 - metallic;

    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    float3 specular = numerator / denominator;
    return kD * albedo / PI + specular;
}

float4 main(PixelShaderInput IN) : SV_TARGET
{
	float3 N = normalize(IN.Normal);
	float3 V = normalize(CameraPosition.xyz - IN.WorldPosition);
	float3 R = reflect(-V, N);
	float3 albedo = diffuseMap.Sample(sampleWrap, IN.TextureCoord).xyz;

    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, albedo, metallic);

	// Compute directional light reflectance
	float3 lightDir = normalize(-DirLight.Direction.xyz);
    float3 lightColor = DirLight.Color.xyz;
    float3 Lo = computeBRDF(F0, albedo, roughness, metallic, lightDir, V, N) * lightColor * max(dot(N, lightDir), 0.0);

    float3 ambient = float3(0.03, 0.03, 0.03) * albedo * 1.0;
    float shadow = DirLightShadowCalculation(IN.WorldPosition);

    float3 color = ambient + (1.0 - shadow) * Lo;

	return float4(color, 1.0);
}
