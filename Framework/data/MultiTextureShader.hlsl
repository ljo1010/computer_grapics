cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projMatrix;
};

cbuffer BlendBuffer : register(b1)
{
    float alphaStrength;
    float uvTile;
    float2 pad;
};

cbuffer LightBuffer : register(b2)
{
    float4 ambientColor;

    float3 pointPos0;
    float pointRange0;
    float4 pointColor0;

    float3 pointPos1;
    float pointRange1;
    float4 pointColor1;
};

cbuffer ShadowBuffer : register(b3)
{
    matrix lightViewMatrix;
    matrix lightProjectionMatrix;
    float shadowBias;
    int enableShadow;
    int enablePCF;
    float _shadowPad;
};

cbuffer DirLightBuffer : register(b4)
{
    float4 dirAmbient;
    float4 dirDiffuse;
    float3 dirDirection;
    float _dirPad;
};

Texture2D tex0 : register(t0);
Texture2D tex1 : register(t1);
Texture2D texAlpha : register(t2);
Texture2D ShadowMap : register(t3);

SamplerState SampleType : register(s0);

struct VS_INPUT
{
    float3 position : POSITION;
    float2 tex : TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
};

VS_OUTPUT VS_main(VS_INPUT input)
{
    VS_OUTPUT output;

    float4 pos = float4(input.position, 1.0f);
    float4 wpos = mul(pos, worldMatrix);
    output.worldPos = wpos.xyz;

    float4 vpos = mul(wpos, viewMatrix);
    float4 ppos = mul(vpos, projMatrix);
    output.position = ppos;

    output.tex = input.tex * uvTile;

    return output;
}

float3 ComputePointLight(
    float3 worldPos,
    float3 normal,
    float3 pointPos,
    float4 pointColor,
    float pointRange)
{
    if (pointRange <= 0.0f)
        return 0.0f;

    float3 L = pointPos - worldPos;
    float dist = length(L);
    if (dist > pointRange || dist <= 1e-6f)
        return 0.0f;

    L = normalize(L);
    float NdotL = saturate(dot(normal, L));
    float atten = saturate(1.0f - dist / pointRange);

    return pointColor.rgb * NdotL * atten;
}

float CalculateShadow(float3 worldPosition)
{
    if (enableShadow == 0)
        return 1.0f;

    float4 lightSpacePos = mul(float4(worldPosition, 1.0f), lightViewMatrix);
    lightSpacePos = mul(lightSpacePos, lightProjectionMatrix);

    float3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords.x = projCoords.x * 0.5f + 0.5f;
    projCoords.y = -projCoords.y * 0.5f + 0.5f;

    if (projCoords.x < 0.0f || projCoords.x > 1.0f ||
        projCoords.y < 0.0f || projCoords.y > 1.0f ||
        projCoords.z < 0.0f || projCoords.z > 1.0f)
    {
        return 1.0f;
    }

    float currentDepth = projCoords.z - shadowBias;
    float shadowFactor = 0.0f;

    if (enablePCF != 0)
    {
        float texelSize = 1.0f / 2048.0f;
        [unroll]
        for (int x = -1; x <= 1; ++x)
        {
            [unroll]
            for (int y = -1; y <= 1; ++y)
            {
                float pcfDepth = ShadowMap.Sample(SampleType, projCoords.xy + float2(x, y) * texelSize).r;
                shadowFactor += (currentDepth <= pcfDepth) ? 1.0f : 0.0f;
            }
        }
        shadowFactor /= 9.0f;
    }
    else
    {
        float closestDepth = ShadowMap.Sample(SampleType, projCoords.xy).r;
        shadowFactor = (currentDepth <= closestDepth) ? 1.0f : 0.0f;
    }

    return shadowFactor;
}

float4 PS_main(VS_OUTPUT i) : SV_TARGET
{
    float2 uv = i.tex * uvTile;

    float3 c0 = tex0.Sample(SampleType, uv).rgb;
    float3 c1 = tex1.Sample(SampleType, uv).rgb;
    float mask = texAlpha.Sample(SampleType, uv).r;
    mask = saturate(mask * alphaStrength);
    float3 baseColor = lerp(c0, c1, mask);

    float3 N = float3(0.0f, 1.0f, 0.0f);

    float3 diff0 = ComputePointLight(i.worldPos, N, pointPos0, pointColor0, pointRange0);
    float3 diff1 = ComputePointLight(i.worldPos, N, pointPos1, pointColor1, pointRange1);

    float3 Ld = normalize(-dirDirection);
    float NdotLd = saturate(dot(N, Ld));
    float shadow = CalculateShadow(i.worldPos);

    float3 dirLit = dirDiffuse.rgb * NdotLd * shadow;
    float3 totalAmbient = ambientColor.rgb + dirAmbient.rgb;

    float3 lit = totalAmbient + dirLit + diff0 + diff1;
    lit = saturate(lit);

    return float4(baseColor * lit, 1.0f);
}