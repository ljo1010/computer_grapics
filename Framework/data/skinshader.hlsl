// === Constant Buffers ===
#define MAX_BONES 120

cbuffer MatrixBuffer : register(b0)
{
    float4x4 matWorld;
    float4x4 matView;
    float4x4 matProj;
};

cbuffer BoneBuffer : register(b1)
{
    float4x4 matBones[MAX_BONES];
};

// === Input / Output ===
struct VertexInputType
{
    float3 position : POSITION;
    float2 tex : TEXCOORD;
    uint4 boneIdx : BONEID;
    float4 weight : WEIGHT;
};

struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD;
};

Texture2D shaderTexture : register(t0);
SamplerState SampleType : register(s0);

// === Vertex Shader ===
PixelInputType VS(VertexInputType input)
{
    PixelInputType output;

    float4 localPos = float4(input.position, 1.0f);
    float4 skinnedPos = float4(0, 0, 0, 0);

    [unroll]
    for (int i = 0; i < 4; i++)
    {
        uint id = input.boneIdx[i];
        float w = input.weight[i];

        if (w > 0.0f && id < MAX_BONES)   // �� ���� üũ �߰�
        {
            skinnedPos += mul(localPos, matBones[id]) * w;
        }
    }

    float4 pos = mul(skinnedPos, matWorld);
    pos = mul(pos, matView);
    pos = mul(pos, matProj);

    output.position = pos;
    output.tex = input.tex;
    return output;
}

// === Pixel Shader ===
float4 PS(PixelInputType input) : SV_TARGET
{
    return shaderTexture.Sample(SampleType, input.tex);
}
