////////////////////////////////////////////////////////////////////////////////
// Filename: water.hlsl
// Description: Stylized / Realistic Procedural Water Surface Shader
// Features: Dual Normal UV Scrolling, Vertex Wave Displacement,
//           Schlick Fresnel, Blinn-Phong Specular Glitter, Distance Fog
////////////////////////////////////////////////////////////////////////////////

cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};

cbuffer WaterBuffer : register(b1)
{
    float4 deepColor;       // 깊은 물 색상 (에메랄드/남색)
    float4 shallowColor;    // 얕은/반사 물 색상 (하늘빛/민트)
    float3 cameraPosition;
    float  gameTime;
    float  waveSpeed;
    float  waveHeight;
    float  waveFrequency;
    float  waterAlpha;
};

cbuffer LightBuffer : register(b2)
{
    float4 lightDiffuseColor;
    float3 lightDirection;
    float  specularPower;
    float4 specularColor;
    float4 fogColor;
    float  fogStart;
    float  fogEnd;
    int    fogEnabled;
    float  padding;
};

Texture2D normalTexture : register(t0);
SamplerState sampleType : register(s0);

struct VertexInputType
{
    float4 position : POSITION;
    float2 tex      : TEXCOORD0;
    float3 normal   : NORMAL;
};

struct PixelInputType
{
    float4 position     : SV_POSITION;
    float2 tex0         : TEXCOORD0;
    float2 tex1         : TEXCOORD1;
    float3 worldPos     : TEXCOORD2;
    float3 viewDir      : TEXCOORD3;
    float2 baseUV       : TEXCOORD4;
};

////////////////////////////////////////////////////////////////////////////////
// Vertex Shader
////////////////////////////////////////////////////////////////////////////////
PixelInputType WaterVertexShader(VertexInputType input)
{
    PixelInputType output;

    input.position.w = 1.0f;

    // 1. 절차적 정점 파도 변위 (Vertex Wave Displacement)
    float wave = sin(input.position.x * waveFrequency + gameTime * waveSpeed) * 
                 cos(input.position.z * waveFrequency * 0.8f + gameTime * waveSpeed * 0.7f);
    input.position.y += wave * waveHeight;

    // 2. 월드 변환 및 투영 행렬 계산
    float4 worldPos = mul(input.position, worldMatrix);
    output.worldPos = worldPos.xyz;

    output.position = mul(worldPos, viewMatrix);
    output.position = mul(output.position, projectionMatrix);

    // 3. 듀얼 UV 스크롤링
    output.tex0 = input.tex * 2.0f + float2(0.03f, 0.02f) * (gameTime * waveSpeed);
    output.tex1 = input.tex * 3.2f - float2(0.02f, 0.035f) * (gameTime * waveSpeed);

    // 4. 원형 마스킹용 기본 UV (0 ~ 1)
    output.baseUV = input.tex;

    // 5. 시선 벡터 (카메라 방향)
    output.viewDir = normalize(cameraPosition - worldPos.xyz);

    return output;
}

////////////////////////////////////////////////////////////////////////////////
// Pixel Shader
////////////////////////////////////////////////////////////////////////////////
float4 WaterPixelShader(PixelInputType input) : SV_TARGET
{
    // [원형 연못 마스킹 및 가장자리 부드러운 페이드]
    // 사각형 메쉬를 자연스러운 원형/타원형 연못으로 변환
    float2 centerOffset = input.baseUV - float2(0.5f, 0.5f);
    float radialDist = length(centerOffset) * 2.0f; // 중심 0.0 ~ 외곽 1.0

    if (radialDist > 1.0f)
    {
        discard; // 사각형 외곽 영역은 잘라냄
    }

    // 외곽으로 갈수록 알파가 부드럽게 빠져 흙바닥과 자연스럽게 블렌딩
    float edgeFade = saturate((1.0f - radialDist) * 3.5f);

    // 1. 듀얼 노멀맵 샘플링 및 언팩 ([0, 1] -> [-1, 1])
    float4 normalMap0 = normalTexture.Sample(sampleType, input.tex0);
    float4 normalMap1 = normalTexture.Sample(sampleType, input.tex1);

    float3 normal0 = normalMap0.xyz * 2.0f - 1.0f;
    float3 normal1 = normalMap1.xyz * 2.0f - 1.0f;

    // 잔물결 합성 (평면 수면이므로 Y축이 법선 주축)
    float3 blendedNormal = normalize(float3(normal0.x + normal1.x,
                                            normal0.z + normal1.z + 1.0f,
                                            normal0.y + normal1.y));

    float3 V = normalize(input.viewDir);
    float3 N = blendedNormal;

    // 2. 슈릭(Schlick) 프레넬 반사율 연산
    float R0 = 0.05f;
    float NdotV = saturate(dot(N, V));
    float fresnel = R0 + (1.0f - R0) * pow(1.0f - NdotV, 4.0f);

    // 프레넬에 따라 짙은 에메랄드 물빛과 얕은 반사빛 보간
    float3 waterColor = lerp(deepColor.rgb, shallowColor.rgb, fresnel);

    // 3. 태양광 반사 (블린-퐁 스페큘러 글리터)
    float3 L = -normalize(lightDirection);
    float3 H = normalize(L + V);
    float NdotH = saturate(dot(N, H));

    float spec = pow(NdotH, specularPower);
    float3 specular = spec * specularColor.rgb * lightDiffuseColor.rgb * 1.5f;

    // 최종 물 표면 색상 합성
    float3 finalColor = waterColor + specular;

    // 4. 거리 기반 대기 안개 (Distance Fog) 융합
    if (fogEnabled != 0)
    {
        float dist = distance(cameraPosition, input.worldPos);
        float fogFactor = saturate((dist - fogStart) / (fogEnd - fogStart));
        finalColor = lerp(finalColor, fogColor.rgb, fogFactor);
    }

    return float4(finalColor, waterAlpha * edgeFade);
}