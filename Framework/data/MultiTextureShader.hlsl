// ===============================================================
// MultiTextureShader.hlsl
//   - POSITION(float3), TEXCOORD(float2)
//   - MatrixBuffer(b0), BlendBuffer(b1), LightBuffer(b2)
//   - �ؽ�ó 3��(t0,t1,t2) + ���÷�(s0)
//   - ���Ɽ ����, ambient + point light 2��
// ===============================================================

cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projMatrix;
};

cbuffer BlendBuffer : register(b1)
{
    float alphaStrength; // ���� ����ũ ����
    float uvTile; // Ÿ�ϸ� ��
    float2 pad; // �е�
};

cbuffer LightBuffer : register(b2)
{
    float4 ambientColor; // ��ü ȯ�汤

    float3 pointPos0;
    float pointRange0;
    float4 pointColor0;

    float3 pointPos1;
    float pointRange1;
    float4 pointColor1;
};

Texture2D tex0 : register(t0); // dirttexture.dds
Texture2D tex1 : register(t1); // dungeontextile.dds
Texture2D texAlpha : register(t2); // alphatexture.dds
SamplerState SampleType : register(s0);

// ---------------------------------------------------------------
// Vertex shader
// ---------------------------------------------------------------
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

    // ���� ��ǥ
    float4 wpos = mul(pos, worldMatrix);
    output.worldPos = wpos.xyz;

    // Ŭ�� ��ǥ
    float4 vpos = mul(wpos, viewMatrix);
    float4 ppos = mul(vpos, projMatrix);
    output.position = ppos;

    // Ÿ�ϸ� ����
    output.tex = input.tex * uvTile;

    return output;
}

// ---------------------------------------------------------------
// Pixel shader
// ---------------------------------------------------------------
float3 ComputePointLight(
    float3 worldPos,
    float3 normal,
    float3 pointPos,
    float4 pointColor,
    float pointRange)
{
    // range�� 0 �̰ų� color�� 0�̸� ��ǻ� ���� ��
    if (pointRange <= 0.0f)
        return 0.0f;

    float3 L = pointPos - worldPos;
    float dist = length(L);
    if (dist > pointRange)
        return 0.0f;

    if (L.y <= 0.0f || dist > pointRange)
        return 0.0f;

    L = normalize(L);

    float NdotL = saturate(dot(normal, L));

    // ������ ���� ���� (���ϸ� �ٲ㵵 ��)
    float atten = saturate(1.0f - dist / pointRange);

    return pointColor.rgb * NdotL * atten;
}

// ---------- Pixel Shader ----------
float4 PS_main(VS_OUTPUT i) : SV_TARGET
{
    // -----------------------------
    // 1. ��Ƽ �ؽ�ó ����� (�� ��� �״��)
    // -----------------------------
    // �ʿ��ϸ� uvTile �� ���� ������ uv = i.tex �θ� �ᵵ ��
    float2 uv = i.tex * uvTile;

    // dirt / dungeon / alpha
    float3 c0 = tex0.Sample(SampleType, uv).rgb;
    float3 c1 = tex1.Sample(SampleType, uv).rgb;
    // ���ĸ� ����ũ�� R ä�� ��� (���� �ڵ�� ����)
    float mask = texAlpha.Sample(SampleType, uv).r;

    mask = saturate(mask * alphaStrength);
    // mask=0 �� c0, mask=1 �� c1 (�� �ش����� ������ �����)
    float3 baseColor = lerp(c0, c1, mask);

    // -----------------------------
    // 2. ���� (ambient + point light 2��)
    // -----------------------------
    // �ٴ��̴ϱ� ���� ���� +Y
    float3 N = float3(0.0f, 1.0f, 0.0f);

    // ����Ʈ ����Ʈ �� ��
    float3 diff0 = ComputePointLight(i.worldPos, N, pointPos0, pointColor0, pointRange0);
    float3 diff1 = ComputePointLight(i.worldPos, N, pointPos1, pointColor1, pointRange1);

    float3 lit = ambientColor.rgb + diff0 + diff1;
    lit = saturate(lit);

    return float4(baseColor * lit, 1.0f);
}


