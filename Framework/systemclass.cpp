// ===== systemclass.cpp (���溻) =====
#include "systemclass.h"
#include <windows.h>
#include <dinput.h>

static void LockCursorToClient(HWND h) {
    RECT rc; GetClientRect(h, &rc);
    POINT tl{ rc.left, rc.top }, br{ rc.right, rc.bottom };
    ClientToScreen(h, &tl); ClientToScreen(h, &br);
    RECT scr{ tl.x, tl.y, br.x, br.y };
    ClipCursor(&scr);
    ShowCursor(FALSE);
}
static void UnlockCursor() {
    ClipCursor(nullptr);
    ShowCursor(TRUE);
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

    //������ ���� �ּ� Ǯ��� ��
    //if (m_Input->InitializeSound(m_hwnd, L"./data/Lofi-bgm.wav"))
        //m_Input->PlayBGM(true);

    // Graphics
    m_Graphics = new GraphicsClass;
    if (!m_Graphics) return false;
    if (!m_Graphics->Initialize(screenWidth, screenHeight, m_hwnd))
        return false;

    LockCursorToClient(m_hwnd);

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

    float forward = 0, right = 0, up = 0;
    if (m_Input->IsDIKeyDown(DIK_W)) forward += 3.0f;
    if (m_Input->IsDIKeyDown(DIK_S)) forward -= 3.0f;
    if (m_Input->IsDIKeyDown(DIK_D)) right += 3.0f;
    if (m_Input->IsDIKeyDown(DIK_A)) right -= 3.0f;
    if (m_Input->IsDIKeyDown(DIK_E)) up += 3.0f;
    if (m_Input->IsDIKeyDown(DIK_Q)) up -= 3.0f;

    float dt = GetDeltaSeconds();
    if (m_Input->IsDIKeyDown(DIK_LSHIFT) || m_Input->IsDIKeyDown(DIK_RSHIFT)) dt *= 4.0f;

    // --- FPS ��� (1�� ���) ---
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

    // --- CPU ���� ���� ---
    m_Cpu->Frame();                      // ���ο��� 1�� ������ ���
    m_cpu = m_Cpu->GetCpuPercentage();   // 0~100 ��


    int mdx = 0, mdy = 0;
    m_Input->GetMouseDelta(mdx, mdy);   

    m_Graphics->SetCameraMove(forward, right, up, dt);
    // Frame�� ����Ÿ���� �ѱ�ٰ� ����. (�Ʒ� 3) ����)
    m_Graphics->SetPerformance(m_fps, m_cpu);
    if (!m_Graphics->Frame(mdx, mdy)) return false;

    return true;
}


LRESULT CALLBACK SystemClass::MessageHandler(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
    switch (umsg)
    {
        // Ű���� �޽����� ����
    case WM_KEYDOWN: m_Input->KeyDown((unsigned int)wparam); return 0;
    case WM_KEYUP:   m_Input->KeyUp((unsigned int)wparam);   return 0;

    case WM_SETFOCUS:
        LockCursorToClient(hwnd);
        return 0;

    case WM_KILLFOCUS:       
        UnlockCursor();
        return 0;

    case WM_ACTIVATE:       
        if (LOWORD(wparam) == WA_ACTIVE || LOWORD(wparam) == WA_CLICKACTIVE)
            LockCursorToClient(hwnd);
        else if (LOWORD(wparam) == WA_INACTIVE)
            UnlockCursor();
        return 0;

    case WM_EXITSIZEMOVE:
    case WM_SIZE:
        if (GetFocus() == hwnd) LockCursorToClient(hwnd);
        return 0;

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
    }
    else
    {
        screenWidth = 1000;
        screenHeight = 600;
        posX = (GetSystemMetrics(SM_CXSCREEN) - screenWidth) / 2;
        posY = (GetSystemMetrics(SM_CYSCREEN) - screenHeight) / 2;
    }

    m_hwnd = CreateWindowEx(
        WS_EX_APPWINDOW,
        m_applicationName, m_applicationName,
        WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP,  
        posX, posY, screenWidth, screenHeight,
        NULL, NULL, m_hinstance, NULL);

    ShowWindow(m_hwnd, SW_SHOW);
    SetForegroundWindow(m_hwnd);
    SetFocus(m_hwnd);

    ShowCursor(FALSE);                         // ���� �� ����
}

void SystemClass::ShutdownWindows()
{
    UnlockCursor();                             // ���� ����

    if (FULL_SCREEN) ChangeDisplaySettings(NULL, 0);

    DestroyWindow(m_hwnd); m_hwnd = NULL;
    UnregisterClass(m_applicationName, m_hinstance); m_hinstance = NULL;
    ApplicationHandle = NULL;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam)
{
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
