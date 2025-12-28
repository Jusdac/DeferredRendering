//***************************************************************************************
// deferredDefault.hlsl by ZSD (Based on Frank Luna's style)
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
    float4 ShadowPosH : POSITION0;
    float4 SsaoPosH : POSITION1;
    float3 PosW : POSITION2;
    float3 NormalW : NORMAL;
    float3 TangentW : TANGENT;
    float2 TexC : TEXCOORD;
    float2 ScreenTexC : TEXCOORD1; // 新增：屏幕坐标（用于采样GBuffer，不影响原有结构）
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut) 0.0f;
    
    MaterialData matData = gMaterialData[gMaterialIndex];
	
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;
    
    vout.NormalW = mul(vin.NormalL, (float3x3) gWorld);
    vout.TangentW = mul(vin.TangentU, (float3x3) gWorld);

    vout.PosH = mul(posW, gViewProj);

    vout.SsaoPosH = mul(posW, gViewProjTex);
    vout.ShadowPosH = mul(posW, gShadowTransform);
    float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
    vout.TexC = mul(texC, matData.MatTransform).xy;
    
    vout.ScreenTexC = (vout.PosH.xy / vout.PosH.w) * 0.5f + 0.5f;
    vout.ScreenTexC.y = 1.0f - vout.ScreenTexC.y; // 

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    float4 positionSample = gPositionMap.Sample(gsamLinearClamp, pin.ScreenTexC);
    float4 normalSample = gNormalMap.Sample(gsamLinearClamp, pin.ScreenTexC);
    float4 albedoSample = gAlbedoMap.Sample(gsamLinearClamp, pin.ScreenTexC);
    float4 materialSample = gMaterialMap.Sample(gsamLinearClamp, pin.ScreenTexC);
   
    float3 posW = positionSample.xyz; // 世界空间位置
    float3 normalV = normalSample.rgb; // 直接读取视图空间法线，无[0,1]映射
    float3 normalW = mul(normalV, (float3x3) gInvView); 
    normalW = normalize(normalW); // 确保归一化
    float4 diffuseAlbedo = albedoSample; // 漫反射反照率
    float3 fresnelR0 = materialSample.rgb; 
    float shininess = materialSample.a; 
  
    // 3. 无效像素剔除
    if (positionSample.a < 0.5f)
        discard;

    // 4. 视角方向计算
    float3 toEyeW = normalize(gEyePosW - posW);

    // 5. SSAO环境光系数
    pin.SsaoPosH /= pin.SsaoPosH.w;
    float ambientAccess = gSsaoMap.Sample(gsamLinearClamp, pin.SsaoPosH.xy).r;
    float4 ambient = ambientAccess * gAmbientLight * diffuseAlbedo;

    // 6. 阴影因子
    float3 shadowFactor = float3(1.0f, 1.0f, 1.0f);
    shadowFactor[0] = CalcShadowFactor(pin.ShadowPosH);

    Material mat;
    mat.DiffuseAlbedo = diffuseAlbedo;
    mat.FresnelR0 = fresnelR0;
    mat.Shininess = shininess;

    float4 directLight = ComputeLighting(gLights, mat, posW, normalW, toEyeW, shadowFactor);

    float3 r = reflect(-toEyeW, normalW);
    float4 reflectionColor = gCubeMap.Sample(gsamLinearWrap, r);
    float3 fresnelFactor = SchlickFresnel(fresnelR0, normalW, r);
    float3 reflection = (1.0f - (shininess / 256.0f)) * fresnelFactor * reflectionColor.rgb;

    float4 litColor = ambient + directLight;
    litColor.rgb += reflection;
    litColor.a = diffuseAlbedo.a;

    return litColor;
}