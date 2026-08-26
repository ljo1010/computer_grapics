cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix lightViewMatrix;
    matrix lightProjectionMatrix;
};

struct VertexInputType
{
    float4 position : POSITION;
};

struct PixelInputType
{
    float4 position : SV_POSITION;
    float4 depthPosition : TEXCOORD0;
};

PixelInputType VS_Depth(VertexInputType input)
{
    PixelInputType output;
    input.position.w = 1.0f;
    output.position = mul(input.position, worldMatrix);
    output.position = mul(output.position, lightViewMatrix);
    output.position = mul(output.position, lightProjectionMatrix);
    output.depthPosition = output.position;
    return output;
}

float4 PS_Depth(PixelInputType input) : SV_TARGET
{
    float depthValue = input.depthPosition.z / input.depthPosition.w;
    return float4(depthValue, depthValue, depthValue, 1.0f);
}