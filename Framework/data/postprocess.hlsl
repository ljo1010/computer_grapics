////////////////////////////////////////////////////////////////////////////////
// Filename: postprocess.hlsl
// Description: Post-Processing Shader Suite
// Features: Full-Screen Triangle VS, Brightness Extract, Separable Gaussian Blur,
//           Bloom Composite, ACES Film Tone Mapping, Vignette, Gamma Correction
////////////////////////////////////////////////////////////////////////////////

Texture2D sourceTexture : register(t0);
Texture2D bloomTexture  : register(t1);
SamplerState sampleLinearClamp : register(s0);

cbuffer PostProcessBuffer : register(b0)
{
    float2 screenSize;          // 화면 해상도 (width, height)
    float  bloomThreshold;      // 블룸 추출 임계값 (기본 약 0.8)
    float  bloomIntensity;      // 블룸 합성 강도 (기본 약 0.7)
    
    int    enableBloom;         // 블룸 활성화 플래그
    int    enableTonemap;       // ACES 톤매핑 활성화 플래그
    float  vignetteIntensity;   // 비네팅 강도 (기본 약 0.35)
    float  exposure;            // 노출도 (기본 1.0)
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

////////////////////////////////////////////////////////////////////////////////
// Vertex Shader: 정점 버퍼 없이 SV_VertexID를 이용한 풀스크린 트라이앵글 생성
////////////////////////////////////////////////////////////////////////////////
VS_OUTPUT FullScreenVS(uint vertexID : SV_VertexID)
{
    VS_OUTPUT output;
    output.texcoord = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(output.texcoord * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    return output;
}

////////////////////////////////////////////////////////////////////////////////
// Pixel Shader 1: 고휘도 영역 추출 (Bright Pass)
////////////////////////////////////////////////////////////////////////////////
float4 BrightPassPS(VS_OUTPUT input) : SV_TARGET
{
    float4 color = sourceTexture.Sample(sampleLinearClamp, input.texcoord);

    // 루미넌스(휘도) 계산 (인간 시각 인지 가중치)
    float lum = dot(color.rgb, float3(0.2126f, 0.7152f, 0.0722f));

    // Soft-knee 커브로 부드러운 임계값 클리핑
    float knee = bloomThreshold * 0.5f;
    float soft = lum - bloomThreshold + knee;
    soft = clamp(soft, 0.0f, 2.0f * knee);
    soft = (soft * soft) / (4.0f * knee + 1e-4f);

    float contribution = max(soft, lum - bloomThreshold) / max(lum, 1e-4f);
    float3 brightColor = color.rgb * max(0.0f, contribution);

    return float4(brightColor, 1.0f);
}

////////////////////////////////////////////////////////////////////////////////
// Pixel Shader 2: 9-Tap 수평 가우시안 블러 (Horizontal Blur)
////////////////////////////////////////////////////////////////////////////////
float4 BlurHorizontalPS(VS_OUTPUT input) : SV_TARGET
{
    float2 texelSize = 1.0f / screenSize;
    float3 color = 0;

    // 9-Tap Gaussian Weights (Sigma = 2.5)
    const float weights[5] = { 0.227027f, 0.1945946f, 0.1216216f, 0.054054f, 0.016216f };

    color += sourceTexture.Sample(sampleLinearClamp, input.texcoord).rgb * weights[0];

    for (int i = 1; i < 5; ++i)
    {
        float2 offset = float2(texelSize.x * i * 2.0f, 0.0f);
        color += sourceTexture.Sample(sampleLinearClamp, input.texcoord + offset).rgb * weights[i];
        color += sourceTexture.Sample(sampleLinearClamp, input.texcoord - offset).rgb * weights[i];
    }

    return float4(color, 1.0f);
}

////////////////////////////////////////////////////////////////////////////////
// Pixel Shader 3: 9-Tap 수직 가우시안 블러 (Vertical Blur)
////////////////////////////////////////////////////////////////////////////////
float4 BlurVerticalPS(VS_OUTPUT input) : SV_TARGET
{
    float2 texelSize = 1.0f / screenSize;
    float3 color = 0;

    const float weights[5] = { 0.227027f, 0.1945946f, 0.1216216f, 0.054054f, 0.016216f };

    color += sourceTexture.Sample(sampleLinearClamp, input.texcoord).rgb * weights[0];

    for (int i = 1; i < 5; ++i)
    {
        float2 offset = float2(0.0f, texelSize.y * i * 2.0f);
        color += sourceTexture.Sample(sampleLinearClamp, input.texcoord + offset).rgb * weights[i];
        color += sourceTexture.Sample(sampleLinearClamp, input.texcoord - offset).rgb * weights[i];
    }

    return float4(color, 1.0f);
}

////////////////////////////////////////////////////////////////////////////////
// ACES Film Tone Mapping Operator
////////////////////////////////////////////////////////////////////////////////
float3 ACESFilm(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

////////////////////////////////////////////////////////////////////////////////
// Pixel Shader 4: 메인 합성 + ACES 톤매핑 + 비네팅 + 감마 보정
////////////////////////////////////////////////////////////////////////////////
float4 CompositePS(VS_OUTPUT input) : SV_TARGET
{
    float3 sceneColor = sourceTexture.Sample(sampleLinearClamp, input.texcoord).rgb;

    // 1. 블룸 합성
    if (enableBloom != 0)
    {
        float3 bloomColor = bloomTexture.Sample(sampleLinearClamp, input.texcoord).rgb;
        sceneColor += bloomColor * bloomIntensity;
    }

    // 2. 노출도 적용
    sceneColor *= exposure;

    // 3. ACES 필름 톤매핑
    if (enableTonemap != 0)
    {
        sceneColor = ACESFilm(sceneColor);
    }

    // 4. 감성적인 시네마틱 비네팅 (Vignette)
    if (vignetteIntensity > 0.001f)
    {
        float2 uv = input.texcoord - float2(0.5f, 0.5f);
        float dist = length(uv);
        float vig = smoothstep(0.8f, 0.2f, dist * vignetteIntensity * 1.6f);
        sceneColor *= vig;
    }

    // 5. sRGB 감마 보정 (Linear -> Gamma 2.2)
    sceneColor = pow(max(0.0f, sceneColor), 1.0f / 2.2f);

    return float4(sceneColor, 1.0f);
}