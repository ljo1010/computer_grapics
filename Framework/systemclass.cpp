// ===== systemclass.cpp =====
#include "systemclass.h"
#include <windows.h>
#include <dinput.h>

// Dear ImGui 헤더 및 Win32 메시지 처리 프로시저 선언
#include "imgui.h"
#include "imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// 마우스 커서를 게임 창 내부에 가두고 숨깁니다 (1인칭 조작 모드)
static void LockCursorToClient(HWND h) {
    RECT rc; GetClientRect(h, &rc);
    POINT tl{ rc.left, rc.top }, br{ rc.right, rc.bottom };
    ClientToScreen(h, &tl); ClientToScreen(h, &br);
    RECT scr{ tl.x, tl.y, br.x, br.y };
    ClipCursor(&scr);
    while (ShowCursor(FALSE) >= 0); // 마우스 커서를 완전히 숨김
}

// 마우스 커서 제한을 해제하고 커서를 다시 표시합니다 (ImGui UI 모드)
static void UnlockCursor() {
    ClipCursor(nullptr);
    while (ShowCursor(TRUE) < 0);   // 마우스 커서를 완전히 표시
    SetCursor(LoadCursor(NULL, IDC_ARROW));
}

SystemClass::SystemClass() : m_Input(0), m_Graphics(0) {}
SystemClass::SystemClass(const SystemClass& other) { (void)other; }
SystemClass::~SystemClass() {}

bool SystemClass::Initialize()
{
    int  screenWidth = 0;
    int  screenHeight = 0;

    InitializeWindows(screenWidth, screenHeight);

    // Input
    m_Input = new InputClass;
    if (!m_Input) return false;
    m_Input->Initialize();

    // CPU
    m_Cpu = new CpuClass;
    if (!m_Cpu) return false;
    m_Cpu->Initialize();


    if (!m_Input->InitializeDirectInput(m_hinstance, m_hwnd, screenWidth, screenHeight))
        return false;

    //  ּ Ǯ 
    //if (m_Input->InitializeSound(m_hwnd, L"./data/Lofi-bgm.wav"))
        //m_Input->PlayBGM(true);

    // Graphics
    m_Graphics = new GraphicsClass;
    if (!m_Graphics) return false;
    if (!m_Graphics->Initialize(screenWidth, screenHeight, m_hwnd))
        return false;

    // 시작 시 기본적으로 커서를 잠금(1인칭 모드)
    m_isCursorLocked = true;
    LockCursorToClient(m_hwnd);
    if (m_Graphics) m_Graphics->SetCursorLocked(m_isCursorLocked);

    return true;
}

void SystemClass::Shutdown()
{
    UnlockCursor();

    if (m_Graphics) {
        m_Graphics->Shutdown();
        delete m_Graphics;
        m_Graphics = 0;
    }
    if (m_Input) {
        m_Input->ShutdownSound();
        m_Input->ShutdownDirectInput();
        delete m_Input;
        m_Input = 0;
    }
    if (m_Cpu) {
        m_Cpu->Shutdown();
        delete m_Cpu;
        m_Cpu = 0;
    }
    ShutdownWindows();
}

void SystemClass::Run()
{
    MSG  msg{};
    bool done = false;

    while (!done)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (msg.message == WM_QUIT) {
            done = true;
        }
        else {
            if (!Frame()) done = true;
        }
    }
}

// 마우스 커서 잠금 토글 (Tab / F1 / ~ 키 입력 시 호출)
void SystemClass::ToggleCursorLock()
{
    m_isCursorLocked = !m_isCursorLocked;
    if (m_isCursorLocked)
    {
        LockCursorToClient(m_hwnd);
    }
    else
    {
        UnlockCursor();
    }

    // ImGui 마우스 커서 렌더링 상태 동기화
    if (m_Graphics)
    {
        m_Graphics->SetCursorLocked(m_isCursorLocked);
    }
}

static float GetDeltaSeconds()
{
    static LARGE_INTEGER freq = {};
    static LARGE_INTEGER last = {};
    if (freq.QuadPart == 0) {
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&last);
        return 0.016f;
    }
    LARGE_INTEGER now; QueryPerformanceCounter(&now);
    double dt = double(now.QuadPart - last.QuadPart) / double(freq.QuadPart);
    last = now;
    if (dt < 0.0) dt = 0.0;
    if (dt > 0.1) dt = 0.1;
    return static_cast<float>(dt);
}

bool SystemClass::Frame()
{
    if (!m_Input->Frame()) return false;
    if (m_Input->IsEscapePressed() || m_Input->IsKeyDown(VK_ESCAPE)) return false;

    // [Tab], [F1], [~] 키로 마우스 커서 잠금/해제(디버그 UI 조작 모드) 토글
    static bool prevToggleState = false;
    bool currToggleState = m_Input->IsDIKeyDown(DIK_TAB) || m_Input->IsKeyDown(VK_TAB) ||
                           m_Input->IsDIKeyDown(DIK_F1)  || m_Input->IsKeyDown(VK_F1) ||
                           m_Input->IsDIKeyDown(DIK_GRAVE) || m_Input->IsKeyDown(VK_OEM_3);
    if (currToggleState && !prevToggleState)
    {
        ToggleCursorLock();
    }
    prevToggleState = currToggleState;

    float forward = 0, right = 0, up = 0;
    // 커서가 잠겨 있을 때(게임 플레이 모드) 또는 UI 조작 중에도 이동 허용
    if (m_Input->IsDIKeyDown(DIK_W)) forward += 3.0f;
    if (m_Input->IsDIKeyDown(DIK_S)) forward -= 3.0f;
    if (m_Input->IsDIKeyDown(DIK_D)) right += 3.0f;
    if (m_Input->IsDIKeyDown(DIK_A)) right -= 3.0f;
    if (m_Input->IsDIKeyDown(DIK_E)) up += 3.0f;
    if (m_Input->IsDIKeyDown(DIK_Q)) up -= 3.0f;

    float dt = GetDeltaSeconds();
    if (m_Input->IsDIKeyDown(DIK_LSHIFT) || m_Input->IsDIKeyDown(DIK_RSHIFT)) dt *= 4.0f;

    // --- FPS  (1 ) ---
    {
        static float accTime = 0.0f;
        static int   accFrames = 0;

        accTime += dt;
        accFrames += 1;

        if (accTime >= 1.0f) {
            m_fps = accFrames;
            accFrames = 0; 
            accTime -= 1.0f;
        }
    }

    // --- CPU   ---
    m_Cpu->Frame();                      // ο 1  
    m_cpu = m_Cpu->GetCpuPercentage();   // 0~100 


    // 마우스 회전값: 커서가 잠겨 있을 때(1인칭 모드)만 카메라를 회전시킵니다.
    int mdx = 0, mdy = 0;
    if (m_isCursorLocked)
    {
        m_Input->GetMouseDelta(mdx, mdy);   
    }

    m_Graphics->SetCameraMove(forward, right, up, dt);
    // Frame Ÿ ѱٰ . (Ʒ 3) )
    m_Graphics->SetPerformance(m_fps, m_cpu);
    if (!m_Graphics->Frame(mdx, mdy)) return false;

    return true;
}


LRESULT CALLBACK SystemClass::MessageHandler(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
    switch (umsg)
    {
    case WM_KEYDOWN: m_Input->KeyDown((unsigned int)wparam); return 0;
    case WM_KEYUP:   m_Input->KeyUp((unsigned int)wparam);   return 0;

    // 최소 창 크기 제한 (너무 작아져서 렌더 버퍼 생성이 실패하거나 최소화되는 버그 방지)
    case WM_GETMINMAXINFO:
    {
        MINMAXINFO* pInfo = (MINMAXINFO*)lparam;
        pInfo->ptMinTrackSize.x = 640;
        pInfo->ptMinTrackSize.y = 480;
        return 0;
    }

    case WM_SETFOCUS:
        if (m_isCursorLocked) LockCursorToClient(hwnd);
        return 0;

    case WM_KILLFOCUS:       
        UnlockCursor();
        return 0;

    case WM_ACTIVATE:       
        if (LOWORD(wparam) == WA_ACTIVE || LOWORD(wparam) == WA_CLICKACTIVE)
        {
            if (m_isCursorLocked) LockCursorToClient(hwnd);
        }
        else if (LOWORD(wparam) == WA_INACTIVE)
        {
            UnlockCursor();
        }
        return 0;

    // 창 크기 조절/이동 시작 시 마우스 커서 클리핑을 즉시 해제 (리사이즈 충돌 방지)
    case WM_ENTERSIZEMOVE:
        UnlockCursor();
        return 0;

    // 창 크기 조절/이동 완료 시 해상도 갱신 및 커서 상태 복구
    case WM_EXITSIZEMOVE:
    {
        RECT rc;
        if (GetClientRect(hwnd, &rc) && m_Graphics)
        {
            int w = rc.right - rc.left;
            int h = rc.bottom - rc.top;
            if (w > 0 && h > 0)
            {
                m_Graphics->OnResize(w, h);
            }
        }
        if (GetFocus() == hwnd && m_isCursorLocked) LockCursorToClient(hwnd);
        return 0;
    }

    // 마우스 커서 표시 모드일 때 Windows 기본 화살표 커서 보장
    case WM_SETCURSOR:
        if (!m_isCursorLocked)
        {
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return TRUE;
        }
        return DefWindowProc(hwnd, umsg, wparam, lparam);

    // 창 크기 변경 시 실시간 해상도 및 렌더 타깃 갱신
    case WM_SIZE:
        if (wparam != SIZE_MINIMIZED && m_Graphics)
        {
            int newWidth = LOWORD(lparam);
            int newHeight = HIWORD(lparam);
            if (newWidth > 0 && newHeight > 0)
            {
                m_Graphics->OnResize(newWidth, newHeight);
            }
        }
        return DefWindowProc(hwnd, umsg, wparam, lparam);

    default:
        return DefWindowProc(hwnd, umsg, wparam, lparam);
    }
}

void SystemClass::InitializeWindows(int& screenWidth, int& screenHeight)
{
    WNDCLASSEX wc{};
    DEVMODE dmScreenSettings{};
    int posX, posY;

    ApplicationHandle = this;
    m_hinstance = GetModuleHandle(NULL);
    m_applicationName = L"Engine";

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = m_hinstance;
    wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
    wc.hIconSm = wc.hIcon;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = m_applicationName;
    RegisterClassEx(&wc);

    screenWidth = GetSystemMetrics(SM_CXSCREEN);
    screenHeight = GetSystemMetrics(SM_CYSCREEN);

    if (FULL_SCREEN)
    {
        dmScreenSettings.dmSize = sizeof(dmScreenSettings);
        dmScreenSettings.dmPelsWidth = (unsigned long)screenWidth;
        dmScreenSettings.dmPelsHeight = (unsigned long)screenHeight;
        dmScreenSettings.dmBitsPerPel = 32;
        dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
        ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN);
        posX = posY = 0;

        m_hwnd = CreateWindowEx(
            WS_EX_APPWINDOW,
            m_applicationName, L"Animal Farm 3D - Pawdy Engine",
            WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP,  
            posX, posY, screenWidth, screenHeight,
            NULL, NULL, m_hinstance, NULL);
    }
    else
    {
        // 창 모드 기본 해상도: 1280 x 720
        screenWidth = 1280;
        screenHeight = 720;

        // 클라이언트(렌더링) 영역이 1280x720이 되도록 윈도우 프레임 크기 계산
        RECT wr = { 0, 0, screenWidth, screenHeight };
        AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
        int windowWidth = wr.right - wr.left;
        int windowHeight = wr.bottom - wr.top;

        posX = (GetSystemMetrics(SM_CXSCREEN) - windowWidth) / 2;
        posY = (GetSystemMetrics(SM_CYSCREEN) - windowHeight) / 2;

        // 크기 조절이 가능한 모던 윈도우 스타일(WS_OVERLAPPEDWINDOW) 적용
        m_hwnd = CreateWindowEx(
            WS_EX_APPWINDOW,
            m_applicationName, L"Animal Farm 3D - Pawdy Engine",
            WS_OVERLAPPEDWINDOW,
            posX, posY, windowWidth, windowHeight,
            NULL, NULL, m_hinstance, NULL);
    }

    ShowWindow(m_hwnd, SW_SHOW);
    SetForegroundWindow(m_hwnd);
    SetFocus(m_hwnd);

    ShowCursor(FALSE);
}

void SystemClass::ShutdownWindows()
{
    UnlockCursor();                             //  

    if (FULL_SCREEN) ChangeDisplaySettings(NULL, 0);

    DestroyWindow(m_hwnd); m_hwnd = NULL;
    UnregisterClass(m_applicationName, m_hinstance); m_hinstance = NULL;
    ApplicationHandle = NULL;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam)
{
    // Dear ImGui 입력 메시지 우선 처리
    if (ImGui_ImplWin32_WndProcHandler(hwnd, umessage, wparam, lparam))
        return true;

    switch (umessage)
    {
    case WM_DESTROY:
    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;
    default:
        return ApplicationHandle->MessageHandler(hwnd, umessage, wparam, lparam);
    }
}
