////////////////////////////////////////////////////////////////////////////////
// Filename: postprocess.hlsl
// Description: Advanced Post-Processing Suite
// Features: Full-Screen Triangle VS, Brightness Extract, Separable Gaussian Blur,
//           Screen-Space Volumetric God Rays (Radial Sun Shafts),
//           Bloom Composite, ACES Film Tone Mapping, Cinematic Vignette
////////////////////////////////////////////////////////////////////////////////

Texture2D sourceTexture    : register(t0);
Texture2D bloomTexture     : register(t1);
Texture2D rayTexture       : register(t2);
SamplerState sampleLinearClamp : register(s0);

cbuffer PostProcessBuffer : register(b0)
{
    float2 screenSize;          // 화면 해상도 (width, height)
    float  bloomThreshold;      // 블룸 추출 임계값
    float  bloomIntensity;      // 블룸 합성 강도
    
    int    enableBloom;         // 블룸 활성화 플래그
    int    enableTonemap;       // ACES 톤매핑 활성화 플래그
    float  vignetteIntensity;   // 비네팅 강도
    float  exposure;            // 노출도
};

cbuffer GodRaysBuffer : register(b1)
{
    float2 sunScreenPos;        // 태양의 2D 스크린 UV 좌표 (0~1)
    float  sunVisibility;       // 태양이 화면 전방에 있는지 여부 (0.0~1.0)
    float  rayIntensity;        // 빛줄기 강도 (기본 약 1.0)
    
    float  rayDecay;            // 빛줄기 감쇠율 (기본 약 0.96)
    float  rayDensity;          // 빛줄기 밀도 / 확산 반경 (기본 약 0.85)
    float  rayWeight;           // 샘플당 가중치 (기본 약 0.25)
    int    enableGodRays;       // 갓 레이 활성화 플래그
    
    float4 rayColor;            // 빛줄기 색상 (따스한 황금빛 오렌지 틴트)
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

////////////////////////////////////////////////////////////////////////////////
// Vertex Shader: 정점 버퍼 없이 SV_VertexID를 이용한 풀스크린 트라이앵글
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

    float lum = dot(color.rgb, float3(0.2126f, 0.7152f, 0.0722f));

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
// Pixel Shader 4: 태양 오클루전 마스크 추출 (Sun Occlusion Mask)
// 태양 위치 주변의 밝은 하늘빛은 통과시키고, 풍차/건물/나무 등 전경 물체는 검게 차폐
////////////////////////////////////////////////////////////////////////////////
float4 SunOcclusionPS(VS_OUTPUT input) : SV_TARGET
{
    float3 sceneCol = sourceTexture.Sample(sampleLinearClamp, input.texcoord).rgb;
    float lum = dot(sceneCol, float3(0.2126f, 0.7152f, 0.0722f));

    // 태양 중심으로부터의 거리
    float distToSun = length(input.texcoord - sunScreenPos);
    
    // 태양 광원 디스크 (Sun Disk Corona)
    float corona = saturate((0.35f - distToSun) / 0.35f);
    corona = pow(corona, 2.0f);

    // 어두운 전경 물체(건물, 풍차, 나무 실루엣)는 차폐하고, 밝은 하늘빛 영역만 빛줄기 발광체로 사용
    float lightPass = smoothstep(0.45f, 0.85f, lum);
    float3 lightSource = (sceneCol + corona * float3(1.2f, 1.0f, 0.7f)) * lightPass;

    return float4(lightSource, 1.0f);
}

////////////////////////////////////////////////////////////////////////////////
// Pixel Shader 5: 40-Tap 스크린 스페이스 방사형 블러 (Radial Blur Light Shafts)
// 태양 스크린 중심을 향해 픽셀을 샘플링하여 부챗살처럼 뻗어나가는 빛줄기 생성
////////////////////////////////////////////////////////////////////////////////
float4 RadialBlurRayPS(VS_OUTPUT input) : SV_TARGET
{
    // 태양 중심을 향하는 방향 벡터
    float2 deltaTexCoord = (input.texcoord - sunScreenPos);
    
    // 샘플링 스텝 크기 계산
    deltaTexCoord *= (1.0f / 36.0f) * rayDensity;
    
    float2 coord = input.texcoord;
    float illuminationDecay = 1.0f;
    float3 rayAccum = 0.0f;

    // 36회 방사형 샘플링 누적
    [unroll(36)]
    for (int i = 0; i < 36; i++)
    {
        coord -= deltaTexCoord;
        float3 sampleCol = sourceTexture.Sample(sampleLinearClamp, coord).rgb;
        sampleCol *= illuminationDecay * rayWeight;
        rayAccum += sampleCol;
        illuminationDecay *= rayDecay;
    }

    return float4(rayAccum, 1.0f);
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
// Pixel Shader 6: 메인 씬 + 블룸 + 갓 레이 + 비네팅 최종 합성
////////////////////////////////////////////////////////////////////////////////
float4 CompositePS(VS_OUTPUT input) : SV_TARGET
{
    float3 sceneColor = sourceTexture.Sample(sampleLinearClamp, input.texcoord).rgb;

    // 1. 갓 레이 (God Rays / Volumetric Sun Shafts) 합성
    if (enableGodRays != 0 && sunVisibility > 0.01f)
    {
        float3 godRays = rayTexture.Sample(sampleLinearClamp, input.texcoord).rgb;
        sceneColor += godRays * rayColor.rgb * (rayIntensity * sunVisibility);
    }

    // 2. 블룸 합성 (태양, 수면, 파티클의 뽀얀 글로우)
    if (enableBloom != 0)
    {
        float3 bloomColor = bloomTexture.Sample(sampleLinearClamp, input.texcoord).rgb;
        sceneColor += bloomColor * bloomIntensity;
    }

    // 3. 노출도 적용
    sceneColor *= exposure;

    // 4. ACES 필름 톤매핑
    if (enableTonemap != 0)
    {
        sceneColor = ACESFilm(sceneColor);
    }

    // 5. 시네마틱 비네팅 (Vignette)
    if (vignetteIntensity > 0.001f)
    {
        float2 uv = input.texcoord - float2(0.5f, 0.5f);
        float dist = length(uv);
        float vig = smoothstep(0.8f, 0.2f, dist * vignetteIntensity * 1.5f);
        sceneColor *= vig;
    }

    return float4(sceneColor, 1.0f);
}