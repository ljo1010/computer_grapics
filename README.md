# DirectX 11 농장 3D 그래픽스 및 인터랙션 시뮬레이션 프로젝트

DirectX 11 및 C++ 기반으로 제작된 실시간 3D 그래픽스 및 게임플레이 시뮬레이션 프로젝트입니다.
고급 셰이더 기법(섀도우 매핑, 거리 안개, 절차적 파티클, 멀티 텍스처링)과 게임플레이 시스템(동물 먹이주기 퀘스트, 절차적 애니메이션, 1인칭 상호작용)이 결합되어 있습니다.

---

## 1. 프로젝트 주요 기능 목록

### [그래픽스 및 셰이더 시스템]
1. 실시간 방향성 섀도우 매핑 (Directional Shadow Mapping)
   - 2048 x 2048 D32_FLOAT 섀도우 맵 렌더 타겟을 활용한 실시간 깊이(Depth) 기록
   - 3x3 PCF (Percentage-Closer Filtering) 소프트 섀도우 적용으로 부드러운 그림자 경계선 구현
   - 섀도우 바이어스(Shadow Bias) 및 그림자 농도(Shadow Intensity) 실시간 조절

2. 실시간 태양 자전 및 일주 운동 (Real-time Sun Orbit Cycle)
   - 시간에 따라 태양의 3차원 위치와 고도를 주기적으로 회전시키는 천체 시뮬레이션
   - 태양의 이동에 따라 지형과 오브젝트의 그림자가 360도로 실시간 회전

3. 거리 기반 대기 안개 효과 (Distance Fog)
   - 지형 바닥 및 모든 3D 오브젝트에 적용된 카메라 거리 기반 선형 안개 보간 (Linear Distance Fog)
   - 안개 색상, 시작 거리(Fog Start), 최대 거리(Fog End)를 실시간으로 조절하여 깊이감 있는 대기 원근감 연출

4. 절차적 셰이더 기반 파티클 시스템 (Procedural Particle System)
   - 텍스처 에셋 없이 HLSL 픽셀 셰이더 연산만으로 구현된 십자 별빛(Cross Sparkle)과 코어 글로우(Core Glow)
   - 카메라를 항상 바라보는 빌보드(Billboard) 쿼드 및 가산 혼합(Additive Blending) 기반 반사광 표현
   - 중력, 수명, 방사형 속도 물리 시뮬레이션 및 페이드아웃 적용

5. 멀티 텍스처 터레인 스플래팅 (Terrain Multi-Texture Splatting)
   - 알파 마스크 텍스처를 기반으로 흙(Dirt) 텍스처와 던전 바닥(Dungeon) 텍스처를 자연스럽게 합성
   - 포인트 라이트와 방향성 라이트의 퐁 셰이딩 동시 적용

6. 하드웨어 인스턴싱 렌더링 (Hardware Instancing)
   - 울타리(Fence) 및 나무(Tree) 모델을 인스턴스 버퍼를 통해 단일 드로우 콜로 대량 렌더링 최적화

7. FBX 스키닝 애니메이션 (Skinned Mesh Animation)
   - 본(Bone) 행렬 팔레트 및 정점 가중치(Bone Weights)를 이용한 캐릭터 스키닝 애니메이션 재생 및 모션 전환

---

### [게임플레이 및 인터랙션 시스템]
1. 동물 먹이주기 퀘스트 시스템 (Animal Feeding Quest)
   - 총 4종의 농장 동물(말, 돼지, 닭, 염소)에 대한 배고픔/포만 상태 머신
   - 건초 더미에서 건초 줍기 (최대 5개 보유, 2.5초 자동 리스폰)
   - [F] 키로 조준점 방향 건초 발사 (물리 포물선 투사체)
   - [E] 키로 가까이 있는 동물에게 직접 먹이 주기
   - [R] 키로 퀘스트 재시작

2. 절차적 뜀뛰기 애니메이션 (Procedural Hop Animation)
   - 먹이를 먹은 동물이 기쁨을 표현하는 사인파(Sine wave) 기반의 수직 바운스 및 좌우 회전 모션
   - 먹이를 먹었을 때 머리 위로 황금빛 별빛 및 분홍빛 하트 파티클 분출

3. 인게임 퀘스트 HUD 및 Dear ImGui 디버그 컨트롤 패널
   - 화면 좌상단에 실시간 퀘스트 달성률, 건초 탄약 수량, 조작 가이드 오버레이
   - 1인칭 FPS 조작 모드 시 마우스 클릭 100% 투과(Pass-through) 처리로 조작 간섭 제거
   - [Tab] 또는 [F1] 키로 마우스 커서를 해제하고 ImGui 디버그 패널에서 모든 그래픽/조명 파라미터 실시간 제어

4. 옵저버 패턴 및 이벤트 버스 (Observer Pattern / Event-Driven Architecture)
   - 게임플레이(퀘스트) 시스템과 그래픽스(파티클 시스템) 간의 직접적인 참조 및 의존성을 완전히 제거(Decoupling)
   - 타입 세이프(Type-Safe) 제네릭 이벤트 버스(`EventBus`)를 구축하여 발행(Publish)/구독(Subscribe) 모델로 시스템 간 결합도 완화 및 단일 책임 원칙(SRP) 준수

5. 유한 상태 머신 및 상태 패턴 (Finite State Machine / State Pattern)
   - 동물의 행동과 상태를 단순 플래그(`bool`)가 아닌 `IAnimalState` 기반 상태 클래스(`HungryState`, `HappyHopState`, `SatisfiedState`)로 캡슐화
   - 객체 스스로 상태 전이(State Transition)와 애니메이션 모션(숨쉬기, 사인파 뜀뛰기, 만족 대기)을 수행하여 개방-폐쇄 원칙(OCP) 실현

6. 모던 C++ 스마트 포인터 및 RAII 메모리 관리 (Smart Pointers & RAII)
   - `GraphicsClass`의 핵심 엔진 서브시스템(D3DClass, CameraClass, Shaders, ShadowMap, UI)에 생 포인터 대신 `std::unique_ptr` 적용
   - 수동 `delete`를 100% 제거하고 자원의 획득과 해제를 객체 생명주기에 바인딩하여 예외 안전성(Exception Safety) 및 메모리 누수 제로(Zero Leak) 달성

7. 렌더 패스 파이프라인 아키텍처 (Render Pass Pipeline Pattern)
   - 거대한 단일 렌더 함수를 상용 렌더 파이프라인 수준의 단계별 렌더 패스(Pass 1~8)로 모듈화 분리
   - `RenderContext`를 통해 행렬, 광원, 섀도우 SRV를 공유하며 Shadow -> Skybox -> Terrain -> Mesh -> Instancing -> Skinned -> Particle -> UI 순서로 체계적 실행

8. 이벤트 기반 오디오 및 사운드 시스템 (Event-Driven Audio System)
   - DirectSound 기반의 배경음악(Lofi BGM) 루프 재생 및 절차적 16비트 PCM 합성 효과음 3종 탑재
   - 옵저버 패턴 연동: 건초 발사 시 '스윙음', 먹이 적중 시 '띠링~ 벨소리', 미션 올클리어 시 '승리 팡파레' 자동 재생
   - [M] 키 및 ImGui 패널을 통한 실시간 음소거(Mute) 토글 지원

9. 스타일라이즈드 절차적 수면 렌더링 시스템 (Procedural Water Surface System)
   - 듀얼 노멀맵 시간 기반 UV 스크롤링으로 잔물결 파동 표현 및 정점 셰이더 3차원 파도 출렁임(Vertex Wave Displacement)
   - 슈릭(Schlick) 프레넬 반사율 계산으로 시선 각도에 따른 깊은 에메랄드 물빛과 얕은 반사광 보간
   - 블린-퐁 스페큘러 태양광 반짝임(Glitter) 및 거리 안개 융합, ImGui 실시간 파라미터(위치, 크기, 파도, 색상) 튜닝

---

## 2. 핵심 소스 코드 가이드 (중점적으로 보아야 할 파일)

### [셰이더 및 렌더 파이프라인]
- Framework/data/mergephongpoint.hlsl
  - 메인 3D 오브젝트(건물, 동물, 소품 등)를 렌더링하는 통합 셰이더
  - 방향성 라이트 퐁 셰이딩, 다중 포인트 라이트 감쇠, 섀도우 매핑(PCF), 거리 기반 안개(Distance Fog) 연산이 모두 구현되어 있습니다.

- Framework/data/MultiTextureShader.hlsl
  - 바닥 지형을 렌더링하는 멀티 텍스처 스플래팅 셰이더
  - 알파 마스크 기반 텍스처 블렌딩, 실시간 그림자 투영 및 거리 안개 계산이 포함되어 있습니다.

- Framework/data/depth.hlsl
  - 광원(태양) 카메라 시점에서 씬의 깊이(Depth) 값을 섀도우 맵 텍스처에 기록하는 셰이더입니다.

- Framework/data/particle.hlsl
  - 절차적으로 반짝이는 별빛 및 코어 글로우를 생성하고 빌보드 정점을 계산하는 파티클 전용 셰이더입니다.

- Framework/data/water.hlsl
  - 수면의 듀얼 UV 스크롤링, 정점 사인파 파도 변위, 프레넬 반사율, 태양광 스페큘러, 거리 안개를 연산하는 물 전용 셰이더입니다.

- Framework/WaterClass.h / .cpp, WaterShaderClass.h / .cpp
  - 40x40 세분화 그리드 버퍼를 생성하고 파라미터를 바인딩하여 3차원 출렁이는 연못 수면을 렌더링하는 클래스입니다.

- Framework/ShadowMapClass.h / .cpp
  - 2048 x 2048 해상도의 D32_FLOAT 뎁스 렌더 타겟 및 셰이더 리소스 뷰(SRV) 생성/바인딩 클래스입니다.

- Framework/DepthShaderClass.h / .cpp
  - 섀도우 패스 전용 셰이더 래퍼 클래스입니다.

- Framework/lightshaderclass.h / .cpp
  - mergephongpoint.hlsl에 모든 상수 버퍼(행렬, 조명, 그림자, 안개)를 바인딩하고 렌더링을 지휘하는 클래스입니다.

- Framework/MultiTextureShaderClass.h / .cpp
  - MultiTextureShader.hlsl에 지형 텍스처, 조명, 그림자, 안개 버퍼를 바인딩하는 클래스입니다.

- Framework/ParticleSystem.h / .cpp
  - 동적 버퍼를 활용하여 파티클의 생성, 물리 시뮬레이션(중력, 수명, 페이드), 빌보드 렌더링을 처리하는 시스템입니다.

---

### [게임플레이 및 시스템 관리자]
- Framework/Event.h / EventBus.h
  - 타입 세이프 제네릭 이벤트 버스 및 게임 내 이벤트 구조체(AnimalFedEvent, QuestResetEvent, HayImpactEvent)가 정의된 옵저버 패턴의 핵심 아키텍처입니다.

- Framework/AnimalQuestSystem.h / .cpp
  - 4마리 동물(말, 돼지, 닭, 염소)의 상태 관리, 건초 충돌 판정, 절차적 뜀뛰기 모션, 탄약 획득 및 리스폰을 담당하는 핵심 퀘스트 클래스입니다. (이벤트 발행 주체)

- Framework/AnimalState.h / .cpp
  - IAnimalState 기반 FSM(상태 패턴) 구현체 (HungryState, HappyHopState, SatisfiedState)로 각 상태의 고유 행동과 생명주기를 캡슐화한 클래스입니다.

- Framework/RenderContext.h / IRenderPass.h
  - 렌더 패스 파이프라인에서 공유되는 프레임 컨텍스트 및 렌더 패스 전략 패턴(Strategy Pattern) 인터페이스입니다.

- Framework/SoundManager.h / .cpp
  - DirectSound 기반 배경음악(Lofi BGM) 및 이벤트 버스 구독형 효과음(발사음, 먹이 적중음, 승리 팡파레) 관리자입니다.

- Framework/LightManager.h / .cpp
  - 태양의 3차원 자전 궤도(UpdateSunOrbit), 광원 뷰/투영 행렬 생성, 그림자 파라미터, 대기 안개 속성(Fog Color, Start, End)을 총괄 관리하는 클래스입니다.

- Framework/ProjectileSystem.h / .cpp
  - 플레이어가 던지는 건초 투사체의 포물선 궤적 및 수명을 계산하는 물리 투사체 시스템입니다.

- Framework/PlayerController.h / .cpp
  - 1인칭 FPS 이동(WASD), 마우스 회전(Yaw/Pitch), 울타리 충돌 검사(AABB)를 수행하는 플레이어 조작기입니다.

- Framework/graphicsclass.h / .cpp
  - 전체 렌더링 파이프라인(섀도우 패스 -> 지형 패스 -> 메쉬 패스 -> 인스턴싱 패스 -> 스키닝 패스 -> 파티클 패스 -> UI 패스)을 통합 실행하는 메인 그래픽스 클래스입니다.

---

## 3. 조작 방법 안내

### [기본 조작]
- W, A, S, D: 1인칭 카메라 이동
- 마우스 이동: 시점 회전 (Yaw / Pitch)
- F: 건초 던지기 발사 (보유 탄약 소모)
- E: 가까이 있는 동물에게 직접 먹이 주기
- R: 퀘스트 리셋 (동물들을 다시 배고픔 상태로 초기화)
- M: 오디오 음소거 토글 (BGM 및 효과음 On/Off)
- 1, 2: 농장 소녀 캐릭터 스키닝 애니메이션 전환
- Tab 또는 F1: 마우스 커서 해제 / 1인칭 모드 토글 (ImGui 조작 모드)

### [디버그 패널 (ImGui)]
- 마우스 커서 해제(Tab/F1) 상태에서 우측 디버그 창을 통해 다음 항목들을 실시간 조절 가능:
  - 퀘스트 진행 상태 및 개별 동물 상태 확인
  - 성능 모니터링 (FPS, ms/frame, CPU 점유율, 해상도)
  - 실시간 그림자 ON/OFF, 3x3 PCF 토글, 태양 실시간 자전 ON/OFF, 자전 속도, 그림자 농도, 바이어스
  - 대기 거리 안개(Distance Fog) ON/OFF, 안개 색상, 시작 거리, 최대 거리
  - 파티클 이펙트 수동 방출 테스트 및 클리어
  - 광원 방향, 주변광(Ambient), 확산광(Diffuse), 반사광(Specular), 광택 강도
  - 카메라 및 플레이어 실시간 좌표 확인

---

## 4. 빌드 환경
- OS: Windows 10 / 11 (64-bit)
- IDE: Visual Studio 2022
- 플랫폼 툴셋: v143
- 언어 표준: C++17
- 그래픽스 API: DirectX 11 (Direct3D 11)
- 종속 라이브러리: Assimp, DirectXTK, Dear ImGui, DirectXMath