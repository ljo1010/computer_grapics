////////////////////////////////////////////////////////////////////////////////
// Filename: water.hlsl
// Description: Advanced Stylized Water Surface Shader
// Features: Dual Normal UV Flow, Vertex Wave Displacement,
//           Schlick Fresnel, Procedural Caustics, Shoreline Foam Ring,
//           Atmospheric Sky Reflection, Specular Sun Glitter, Distance Fog
////////////////////////////////////////////////////////////////////////////////

cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};

cbuffer WaterBuffer : register(b1)
{
    float4 deepColor;       // 깊은 물 색상 (에메랄드/청록)
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

    // 1. 3중 사인파 절차적 정점 파도 변위 (Vertex Wave Displacement)
    float wave1 = sin(input.position.x * waveFrequency + gameTime * waveSpeed * 1.2f);
    float wave2 = cos(input.position.z * waveFrequency * 0.9f + gameTime * waveSpeed * 0.8f);
    float wave3 = sin((input.position.x + input.position.z) * waveFrequency * 0.6f + gameTime * waveSpeed);
    float totalWave = (wave1 * 0.45f + wave2 * 0.35f + wave3 * 0.20f) * waveHeight;
    input.position.y += totalWave;

    // 2. 월드 변환 및 투영 행렬 계산
    float4 worldPos = mul(input.position, worldMatrix);
    output.worldPos = worldPos.xyz;

    output.position = mul(worldPos, viewMatrix);
    output.position = mul(output.position, projectionMatrix);

    // 3. 듀얼 UV 스크롤링
    output.tex0 = input.tex * 2.2f + float2(0.025f, 0.015f) * (gameTime * waveSpeed);
    output.tex1 = input.tex * 3.4f - float2(0.018f, 0.028f) * (gameTime * waveSpeed);

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
    // ------------------------------------------------------------
    // [1] 원형 연못 마스킹 및 가장자리 부드러운 페이드
    // ------------------------------------------------------------
    float2 centerOffset = input.baseUV - float2(0.5f, 0.5f);
    float radialDist = length(centerOffset) * 2.0f; // 중심 0.0 ~ 외곽 1.0

    if (radialDist > 1.0f)
    {
        discard;
    }

    // ------------------------------------------------------------
    // [2] 듀얼 노멀맵 샘플링 및 미세 물결 법선 합성
    // ------------------------------------------------------------
    float4 normalMap0 = normalTexture.Sample(sampleType, input.tex0);
    float4 normalMap1 = normalTexture.Sample(sampleType, input.tex1);

    float3 normal0 = normalMap0.xyz * 2.0f - 1.0f;
    float3 normal1 = normalMap1.xyz * 2.0f - 1.0f;

    // 잔물결 합성 (평면 수면이므로 Y축이 법선 주축)
    float3 blendedNormal = normalize(float3((normal0.x + normal1.x) * 0.7f,
                                            1.0f,
                                            (normal0.y + normal1.y) * 0.7f));

    float3 V = normalize(input.viewDir);
    float3 N = blendedNormal;

    // ------------------------------------------------------------
    // [3] 슈릭(Schlick) 프레넬 반사율 연산
    // ------------------------------------------------------------
    float R0 = 0.04f;
    float NdotV = saturate(dot(N, V));
    float fresnel = R0 + (1.0f - R0) * pow(1.0f - NdotV, 3.5f);

    // ------------------------------------------------------------
    // [4] 절차적 햇빛 굴절빛 (Procedural Sunlight Caustics)
    // ------------------------------------------------------------
    float2 cUV1 = input.worldPos.xz * 0.5f + float2(gameTime * 0.025f, gameTime * 0.015f);
    float2 cUV2 = input.worldPos.xz * 0.5f - float2(gameTime * 0.02f, -gameTime * 0.03f);
    float c1 = sin(cUV1.x * 7.0f + sin(cUV1.y * 6.0f)) * 0.5f + 0.5f;
    float c2 = cos(cUV2.y * 7.0f + cos(cUV2.x * 6.0f)) * 0.5f + 0.5f;
    float causticLight = pow(c1 * c2, 1.6f) * 0.45f;

    // ------------------------------------------------------------
    // [5] 하늘 환경 반사 (Atmospheric Sky Reflection)
    // ------------------------------------------------------------
    float3 R = reflect(-V, N);
    float3 skyZenith = float3(0.32f, 0.62f, 0.92f);   // 맑은 하늘빛
    float3 skyHorizon = shallowColor.rgb * 1.15f;     // 수면 반사빛
    float3 skyReflection = lerp(skyHorizon, skyZenith, saturate(R.y));

    // 깊은 에메랄드 물빛과 하늘 반사빛 합성 + 카우스틱스 하이라이트
    float3 waterBase = lerp(deepColor.rgb, skyReflection, fresnel);
    waterBase += causticLight * float3(0.4f, 0.85f, 0.95f);

    // ------------------------------------------------------------
    // [6] 태양광 스페큘러 반짝임 (Sun Specular Glitter & Shimmer)
    // ------------------------------------------------------------
    float3 L = -normalize(lightDirection);
    float3 H = normalize(L + V);
    float NdotH = saturate(dot(N, H));

    // 넓은 주 스페큘러 + 좁고 강렬한 보석빛 스파클
    float specMain = pow(NdotH, specularPower) * 1.2f;
    float specSparkle = pow(NdotH, specularPower * 4.0f) * 2.5f;
    float3 specular = (specMain + specSparkle) * specularColor.rgb * lightDiffuseColor.rgb;

    float3 finalColor = waterBase + specular;

    // ------------------------------------------------------------
    // [7] 연안 물가 하얀 거품 링 (Shoreline Foam Ripple)
    // ------------------------------------------------------------
    float foamDistort = (normal0.x + normal1.y) * 0.05f;
    float foamCoord = radialDist + foamDistort;
    
    // 연안 경계선에 찰랑이는 물거품 띠
    float foamWave = sin(foamCoord * 35.0f - gameTime * waveSpeed * 4.0f) * 0.5f + 0.5f;
    float foamMask = smoothstep(0.78f, 0.96f, foamCoord);
    float foamFactor = saturate(foamMask * (foamWave * 0.6f + 0.5f));
    
    float3 foamColor = float3(0.92f, 0.97f, 1.0f); // 뽀얀 우윳빛 물거품
    finalColor = lerp(finalColor, foamColor, foamFactor * 0.85f);

    // ------------------------------------------------------------
    // [8] 가장자리 부드러운 알파 페이드 (Soft Edge Falloff)
    // ------------------------------------------------------------
    float edgeFade = saturate((1.0f - radialDist) * 4.0f);
    float finalAlpha = saturate(waterAlpha * edgeFade + foamFactor * 0.5f);

    // ------------------------------------------------------------
    // [9] 거리 기반 대기 안개 (Distance Fog) 융합
    // ------------------------------------------------------------
    if (fogEnabled != 0)
    {
        float dist = distance(cameraPosition, input.worldPos);
        float fogFactor = saturate((dist - fogStart) / (fogEnd - fogStart));
        finalColor = lerp(finalColor, fogColor.rgb, fogFactor);
    }

    return float4(finalColor, finalAlpha);
}