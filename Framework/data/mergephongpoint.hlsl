// mergephongpoint.hlsl
// Directional + up to 3 point lights

#define NUM_LIGHTS 3

cbuffer MatrixBuffer
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};

cbuffer CameraBuffer
{
    float3 cameraPosition;
    float _camPadding;
};

cbuffer DirLightBuffer
{
    float4 ambientColor;
    float4 diffuseColor;
    float3 lightDirection;
    float specularPower;
    float4 specularColor;
};

cbuffer PointLightPositionBuffer
{
    float4 lightPosition[NUM_LIGHTS];
};

cbuffer PointLightColorBuffer
{
    float4 pointDiffuse[NUM_LIGHTS];
};

cbuffer AttenuationBuffer
{
    float kc;
    float kl;
    float kq;
    float pointIntensityScale;
};

cbuffer ToggleBuffer
{
    int enableAmbient;
    int enableDiffuse;
    int enableSpecular;
    int _togglePadding;
};

Texture2D AlbedoMap : register(t0);
Texture2D MetallicMap : register(t1);
Texture2D RoughnessMap : register(t2);
SamplerState SampleType : register(s0);

struct VertexInputType
{
    float4 position : POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
};

struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 viewDir : TEXCOORD1;

    // 3 point lights
    float3 toPoint0 : TEXCOORD2;
    float3 toPoint1 : TEXCOORD3;
    float3 toPoint2 : TEXCOORD4;
};

PixelInputType LightVertexShader(VertexInputType input)
{
    PixelInputType output;
    float4 worldPos;

    input.position.w = 1.0f;
    worldPos = mul(input.position, worldMatrix);
    output.position = mul(worldPos, viewMatrix);
    output.position = mul(output.position, projectionMatrix);

    output.tex = input.tex;

    output.normal = mul(input.normal, (float3x3) worldMatrix);
    output.normal = normalize(output.normal);

    output.viewDir = normalize(cameraPosition - worldPos.xyz);

    // NUM_LIGHTS == 3 �̶� 0,1,2 ������ ���
    output.toPoint0 = lightPosition[0].xyz - worldPos.xyz;
    output.toPoint1 = lightPosition[1].xyz - worldPos.xyz;
    output.toPoint2 = lightPosition[2].xyz - worldPos.xyz;

    return output;
}

float3 safeNormalize(float3 v)
{
    float len = length(v);
    return (len > 1e-6f) ? v / len : float3(0, 0, 0);
}

void AccumulatePointLight(
    float3 N, float3 V,
    float3 toLight, float4 lColor,
    float metallic, float roughness,
    inout float3 diffAcc, inout float3 specAcc)
{
    float dist = length(toLight);
    if (dist <= 1e-6f)
        return;

    float3 L = toLight / dist;
    float NdotL = saturate(dot(N, L));

    float attDen = kc + kl * dist + kq * dist * dist;
    float att = (attDen > 1e-6f) ? (1.0f / attDen) : 1.0f;

    // �ݼ��ϼ��� diffuse ���̱�
    float diffuseScale = lerp(1.0f, 0.1f, metallic);
    float3 diffuse = lColor.rgb * NdotL * att * pointIntensityScale * diffuseScale;

    float3 H = safeNormalize(V + L);
    float NdotH = saturate(dot(N, H));

    // �����Ͻ� -> �������Ͻ� -> �Ŀ�
    float smoothness = 1.0f - roughness;
    float shininess = lerp(8.0f, 64.0f, smoothness);

    // �ݼ��̸� ������ ���� ������ ���� �ƴ϶� ������ ������ ���� �� �����ٰ� ����
    float3 specBase = lerp(specularColor.rgb, lColor.rgb, metallic);

    float3 spec = specBase * pow(NdotH, shininess) * att * pointIntensityScale;

    diffAcc += diffuse;
    specAcc += spec;
}

float4 LightPixelShader(PixelInputType input) : SV_TARGET
{
    float4 albedo = AlbedoMap.Sample(SampleType, input.tex);

    // 0~1 ������ ��Ż/���� ����(��κ� R ä�θ� ���)
    float metallic = MetallicMap.Sample(SampleType, input.tex).r;
    float roughness = RoughnessMap.Sample(SampleType, input.tex).r;
    roughness = saturate(roughness);
    metallic = saturate(metallic);

    float3 N = normalize(input.normal);
    float3 V = normalize(input.viewDir);

    float3 colorAccum = 0;
    float3 specAccum = 0;

    // Ambient
    if (enableAmbient != 0)
        colorAccum += ambientColor.rgb;

    // ���Ɽ Diffuse
    if (enableDiffuse != 0)
    {
        float3 Ld = normalize(-lightDirection);
        float NdotLd = saturate(dot(N, Ld));

        // �ݼ��ϼ��� Ȯ��ݻ� ���̱�
        float diffuseScale = lerp(1.0f, 0.1f, metallic);
        colorAccum += diffuseColor.rgb * NdotLd * diffuseScale;
    }

    // ���Ɽ Specular
    if (enableSpecular != 0)
    {
        float3 Ld = normalize(-lightDirection);
        float3 H = safeNormalize(V + Ld);
        float NdotH = saturate(dot(N, H));

        float smoothness = 1.0f - roughness;
        float shininess = lerp(8.0f, 64.0f, smoothness);

        // �ݼ��ϼ��� ������ ���� �˺��� ������ ����
        float3 specBase = lerp(specularColor.rgb, albedo.rgb, metallic);

        specAccum += specBase * pow(NdotH, shininess);
    }

    // ����Ʈ ����Ʈ
    float3 pDiff = 0;
    float3 pSpec = 0;

    if (enableDiffuse != 0 || enableSpecular != 0)
    {
        AccumulatePointLight(N, V, input.toPoint0, pointDiffuse[0],
                             metallic, roughness, pDiff, pSpec);
        AccumulatePointLight(N, V, input.toPoint1, pointDiffuse[1],
                             metallic, roughness, pDiff, pSpec);
        AccumulatePointLight(N, V, input.toPoint2, pointDiffuse[2],
                             metallic, roughness, pDiff, pSpec);

        if (enableDiffuse == 0)
            pDiff = 0;
        if (enableSpecular == 0)
            pSpec = 0;
    }

    float3 lit = colorAccum + pDiff;
    float3 outRgb = saturate(lit) * albedo.rgb;
    outRgb = saturate(outRgb + specAccum + pSpec);

    return float4(outRgb, albedo.a);
}
