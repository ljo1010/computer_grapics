// ===============================================================
// GLOBALS
// ===============================================================

// matrix buffer : b0
cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};

// light buffer : b1
cbuffer LightBuffer : register(b1)
{
    float4 diffuseColor;
    float3 lightDirection;
    float padding; // align for 16-byte boundary
};

// textures : t0(diffuse), t1(normalMap)
Texture2D shaderTextures[2] : register(t0);
SamplerState SampleType : register(s0);


// ===============================================================
// TYPEDEFS
// ===============================================================
struct VertexInputType
{
    float4 position : POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 binormal : BINORMAL;
};

struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 binormal : BINORMAL;
};


// ===============================================================
// Vertex Shader
// ===============================================================
PixelInputType BumpMapVertexShader(VertexInputType input)
{
    PixelInputType output;

    input.position.w = 1.0f;

    // world¡æview¡æproj
    output.position = mul(input.position, worldMatrix);
    output.position = mul(output.position, viewMatrix);
    output.position = mul(output.position, projectionMatrix);

    // pass UV
    output.tex = input.tex;

    // transform normal/tangent/binormal to world space
    float3x3 matW = (float3x3) worldMatrix;

    output.normal = normalize(mul(input.normal, matW));
    output.tangent = normalize(mul(input.tangent, matW));
    output.binormal = normalize(mul(input.binormal, matW));

    return output;
}


// ===============================================================
// Pixel Shader
// ===============================================================
float4 BumpMapPixelShader(PixelInputType input) : SV_TARGET
{
    float4 textureColor = shaderTextures[0].Sample(SampleType, input.tex);
    float4 bumpMap = shaderTextures[1].Sample(SampleType, input.tex);

    // Remap normal [0,1] ¡æ [-1,+1]
    bumpMap = bumpMap * 2.0f - 1.0f;

    // Create world-space normal from tangent space components
    float3 bumpNormal =
        bumpMap.x * input.tangent +
        bumpMap.y * input.binormal +
        bumpMap.z * input.normal;

    bumpNormal = normalize(bumpNormal);

    // Flip light direction for dot product
    float3 L = normalize(-lightDirection);

    // Diffuse lighting
    float lightIntensity = saturate(dot(bumpNormal, L));
    float4 lighting = diffuseColor * lightIntensity;

    return saturate(lighting * textureColor);
}
