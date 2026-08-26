cbuffer MatrixBuffer : register(b0)
{
    matrix viewMatrix;
    matrix projectionMatrix;
};

cbuffer CameraBuffer : register(b1)
{
    float3 cameraPosition;
    float _pad0;
};

struct VS_INPUT
{
    float3 position : POSITION;
    float2 tex : TEXCOORD0;
    float4 color : COLOR0;
    float size : PSIZE;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float4 color : COLOR0;
};

PS_INPUT VS_main(VS_INPUT input)
{
    PS_INPUT output;

    float3 toCam = normalize(cameraPosition - input.position);
    float3 up = float3(0.0f, 1.0f, 0.0f);
    float3 right = normalize(cross(up, toCam));
    up = cross(toCam, right);

    float3 worldPos = input.position 
                    + right * (input.tex.x - 0.5f) * input.size 
                    + up * (0.5f - input.tex.y) * input.size;

    float4 viewPos = mul(float4(worldPos, 1.0f), viewMatrix);
    output.position = mul(viewPos, projectionMatrix);
    output.tex = input.tex;
    output.color = input.color;

    return output;
}

float4 PS_main(PS_INPUT input) : SV_TARGET
{
    float2 uv = input.tex - 0.5f;
    float r = length(uv);
    if (r > 0.5f) discard;

    float crossGlow = 0.04f / (abs(uv.x) * abs(uv.y) + 0.003f);
    crossGlow = saturate(crossGlow * 0.2f);

    float circleGlow = saturate(1.0f - (r * 2.0f));
    circleGlow = circleGlow * circleGlow;

    float shape = saturate(crossGlow + circleGlow);
    float alpha = shape * input.color.a;

    if (alpha < 0.01f) discard;

    float3 glowColor = input.color.rgb + float3(0.3f, 0.3f, 0.3f) * circleGlow;
    return float4(glowColor, alpha);
}