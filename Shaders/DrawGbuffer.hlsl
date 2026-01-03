//***************************************************************************************
// DrawGbuffer.hlsl by ZSD (Based on Frank Luna's style)
//***************************************************************************************

#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 0
#endif

#ifndef NUM_POINT_LIGHTS
    #define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
    #define NUM_SPOT_LIGHTS 0
#endif

#include "Common.hlsl"

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TexCOORD;
    float3 TangentU : TANGENT;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION_WORLD; // 世界空间位置
    float3 NormalW : NORMAL;
    float3 TangentW : TANGENT;
    float2 TexC : TexCOORD;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut) 0.0f;

    // Fetch the material data.
    MaterialData matData = gMaterialData[gMaterialIndex];
	
    // Transform to world space.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz; // Save world position

    // Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
    vout.NormalW = mul(vin.NormalL, (float3x3) gWorld);
    vout.TangentW = mul(vin.TangentU, (float3x3) gWorld);

    // Transform to homogeneous clip space.
    vout.PosH = mul(posW, gViewProj);
	
	// Output vertex attributes for interpolation across triangle.
    float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
    vout.TexC = mul(texC, matData.MatTransform).xy;
	
    return vout;
}

// 定义一个结构体，包含所有 G-Buffer 输出
struct GBufferOutput
{
    float4 Position : SV_Target0; // 世界坐标
    float4 Normal : SV_Target1; // 视图空间法线
    float4 Albedo : SV_Target2; // 漫反射颜色
    float4 Material : SV_Target3; // 材质属性 (FresnelR0, Shininess/Roughness)
};

// 使用结构体作为返回类型
GBufferOutput PS(VertexOut pin)
{
    // Fetch the material data.
    MaterialData matData = gMaterialData[gMaterialIndex];

    // Position Pass - 输出世界坐标
    GBufferOutput output;
    output.Position = float4(pin.PosW, 1.0f); // 或者使用 pin.PosW / gFarZ 来归一化深度值
   
    // Normal Pass - 输出视图空间法线
    float3 normalV = mul(normalize(pin.NormalW), (float3x3) gView);
    output.Normal = float4(normalV, 0.0f);

    // Albedo Pass - 输出漫反射颜色
    float4 diffuseAlbedo = matData.DiffuseAlbedo;
    uint diffuseMapIndex = matData.DiffuseMapIndex;
    float4 sampledAlbedo = gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.TexC);

#ifdef ALPHA_TEST
    clip(sampledAlbedo.a - 0.1f);
#endif

    output.Albedo = diffuseAlbedo * sampledAlbedo;

    // Material Pass - 输出 Fresnel R0 和 Shininess/Roughness
    float3 fresnelR0 = matData.FresnelR0;
    float shininess = matData.Roughness * 256.0f; // 将粗糙度转换为高光度
    float normalizedShininess = shininess / 256.0f; // 归一化至 [0, 1] 范围

    fresnelR0 = saturate(fresnelR0);
    normalizedShininess = saturate(normalizedShininess);

    output.Material = float4(fresnelR0, normalizedShininess);

    return output;
}