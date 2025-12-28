//***************************************************************************************
// DrawMaterial.hlsl by ZSD (Based on Frank Luna's style)
//***************************************************************************************

// Defaults for number of lights.
#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 0
#endif

#ifndef NUM_POINT_LIGHTS
    #define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
    #define NUM_SPOT_LIGHTS 0
#endif

// Include common HLSL code.
#include "Common.hlsl"

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
    float3 TangentU : TANGENT;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 NormalW : NORMAL;
    float3 TangentW : TANGENT;
    float2 TexC : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut) 0.0f;

	// Fetch the material data.
    MaterialData matData = gMaterialData[gMaterialIndex];
	
    // Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
    vout.NormalW = mul(vin.NormalL, (float3x3) gWorld);
    vout.TangentW = mul(vin.TangentU, (float3x3) gWorld);

    // Transform to homogeneous clip space.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosH = mul(posW, gViewProj);
	
	// Output vertex attributes for interpolation across triangle.
    float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
    vout.TexC = mul(texC, matData.MatTransform).xy;
	
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	// Fetch the material data.
    MaterialData matData = gMaterialData[gMaterialIndex];
	// float4 diffuseAlbedo = matData.DiffuseAlbedo; // Not used here
	// uint diffuseMapIndex = matData.DiffuseMapIndex; // Not used here
	// uint normalMapIndex = matData.NormalMapIndex; // Not used here
	
    // Interpolating normal can unnormalize it, so renormalize it.
    // pin.NormalW = normalize(pin.NormalW); // Not needed for material calculation in this PS.

    // Pack material properties into the output.
    // Assuming R: FresnelR0.x, G: FresnelR0.y, B: FresnelR0.z, A: Shininess / 256.0f
    // Ensure matData.FresnelR0 is in [0,1] range. Shininess is typically >= 0.
    float3 fresnelR0 = matData.FresnelR0;
    float shininess = matData.Roughness * 256.0f; // Convert Roughness back to Shininess, or use matData.Shininess if available
    float normalizedShininess = shininess / 256.0f; // Normalize to [0, 1] for R8G8B8A8_UNORM

    // If using R8G8B8A8_UNORM, ensure values are clamped to [0, 1]
    fresnelR0 = saturate(fresnelR0); // Clamp FresnelR0 to [0, 1]
    normalizedShininess = saturate(normalizedShininess); // Clamp normalized Shininess to [0, 1]

    // If you want to store Roughness instead of Shininess in the Alpha channel:
    // float roughness = matData.Roughness;
    // return float4(fresnelR0, roughness);

    return float4(fresnelR0, normalizedShininess);
}