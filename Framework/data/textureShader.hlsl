// ===============================================================
// GLOBALS
// ===============================================================
cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
    matrix orthoMatrix;
};

Texture2D shaderTexture : register(t0);
SamplerState SampleType : register(s0);

// ===============================================================
// TYPEDEFS
// ===============================================================
struct VertexInputType
{
    float4 position : POSITION;
    float2 tex : TEXCOORD0;
    float3 instancePos : INSTANCEPOS;
};

struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
};



// ===============================================================
// TEXTURE MODEL SHADER (�Ϲ� �𵨿�)
// ===============================================================
PixelInputType TextureVertexShader(VertexInputType input)
{
    PixelInputType output;

    input.position.w = 1.0f;
    input.position.xyz += input.instancePos;

    output.position = mul(input.position, worldMatrix);
    output.position = mul(output.position, viewMatrix);
    output.position = mul(output.position, projectionMatrix);
    
    output.tex = input.tex;
    return output;
}

float4 TexturePixelShader(PixelInputType input) : SV_TARGET
{
    float4 textureColor = shaderTexture.Sample(SampleType, input.tex);
    return textureColor;
}

// ===============================================================
// SKYBOX SHADER (�ϴ� ť��� ����)
// ===============================================================

// ��ī�̹ڽ� ���� �Է�/��� ����
struct SkyVSOutput
{
    float4 posH : SV_POSITION;
    float3 texCoord : TEXCOORD0;
};

// Vertex Shader
SkyVSOutput SKYMAP_VS(float3 inPos : POSITION)
{
    SkyVSOutput output;

    // ī�޶� �̵� ���� ���� (ī�޶� ȸ���� ����)
    float4 pos = float4(inPos, 1.0f);
    output.posH = mul(pos, worldMatrix);
    output.posH = mul(output.posH, viewMatrix);
    output.posH = mul(output.posH, projectionMatrix);

    // ��ī�̹ڽ��� �׻� ���� �ڿ� �ֵ��� z=w ����
    output.posH = output.posH.xyww;

    // ť��� ���� ���� = �Է� ���� ��ġ
    output.texCoord = inPos;

    return output;
}

// Pixel Shader
float4 SKYMAP_PS(SkyVSOutput input) : SV_Target
{
    // ť��� ���ø�
    return shaderTexture.Sample(SampleType, normalize(input.texCoord));
}
