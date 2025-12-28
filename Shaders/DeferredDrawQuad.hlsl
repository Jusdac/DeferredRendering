//***************************************************************************************
// deferredDrawQuad.hlsl by ZSD (Based on Frank Luna's style)
//***************************************************************************************

#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 3
#endif

#ifndef NUM_POINT_LIGHTS
    #define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
    #define NUM_SPOT_LIGHTS 0
#endif

#include "Common.hlsl"

static const float2 gTexCoords[6] =
{
    float2(0.0f, 1.0f), float2(0.0f, 0.0f), float2(1.0f, 0.0f),
    float2(0.0f, 1.0f), float2(1.0f, 0.0f), float2(1.0f, 1.0f)
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float2 TexC : TEXCOORD;
};

VertexOut VS(uint vid : SV_VertexID)
{
    VertexOut vout;
    vout.TexC = gTexCoords[vid];
    vout.PosH = float4(
        2.0f * vout.TexC.x - 1.0f,
        1.0f - 2.0f * vout.TexC.y,
        0.0f, 1.0f
    );
    return vout;
}
float4 PS(VertexOut pin) : SV_Target
{
    // 1. 采样 G-Buffer
    float4 positionSample = gPositionMap.Sample(gsamLinearClamp, pin.TexC);
    float4 normalSample = gNormalMap.Sample(gsamLinearClamp, pin.TexC);
    float4 albedoSample = gAlbedoMap.Sample(gsamLinearClamp, pin.TexC);
    float4 materialSample = gMaterialMap.Sample(gsamLinearClamp, pin.TexC);

    // 2. 恢复世界空间位置和法线
    float3 posW = positionSample.xyz;
    float3 normalV = normalSample.rgb;
    float3 normalW = normalize(mul(normalV, (float3x3) gInvView));

    // 3. 构建材质
    Material mat;
    mat.DiffuseAlbedo = albedoSample;
    mat.FresnelR0 = materialSample.rgb;
    mat.Shininess = materialSample.a;

    // 4. 视线方向
    float3 toEyeW = normalize(gEyePosW - posW);

    // 5. 计算阴影坐标（使用 G-Buffer 的 posW）
    float4 shadowPosH = mul(float4(posW, 1.0f), gShadowTransform);
    float3 shadowFactor = float3(1.0f, 1.0f, 1.0f);
    shadowFactor[0] = CalcShadowFactor(shadowPosH); // 调用你提供的函数

    // 6. 计算 SSAO 坐标并采样
    float4 ssaoPosH = mul(float4(posW, 1.0f), gViewProjTex);
    ssaoPosH /= ssaoPosH.w;
    float ambientAccess = gSsaoMap.Sample(gsamLinearClamp, ssaoPosH.xy).r;

    // 7. 环境光
    float4 ambient = ambientAccess * gAmbientLight * albedoSample;

    // 8. 直接光照（含阴影）
    float4 directLight = ComputeLighting(gLights, mat, posW, normalW, toEyeW, shadowFactor);

    // 9. 镜面反射（CubeMap）
    float3 r = reflect(-toEyeW, normalW);
    float3 fresnelFactor = SchlickFresnel(mat.FresnelR0, normalW, r);
    float4 reflectionColor = gCubeMap.Sample(gsamLinearWrap, r);

    // 10. 合成最终颜色
    float4 litColor = ambient + directLight;
    litColor.rgb += mat.Shininess * fresnelFactor * reflectionColor.rgb;
    litColor.a = albedoSample.a;

    return litColor;
}