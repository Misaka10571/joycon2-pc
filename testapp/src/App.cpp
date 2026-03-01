// App.cpp - Main entry point for JoyCon2 Connector
// DirectX 11 + Dear ImGui — Material Design 3, frameless window, DPI-aware
#define NOMINMAX
#include <Windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <dwmapi.h>
#include <mmsystem.h>
#include <tchar.h>
#include <string>
#include <algorithm>

#include <winrt/Windows.Foundation.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

#include "UI_Theme.h"
#include "UI_Pages.h"
#include "ConfigManager.h"
#include "ViGEmManager.h"
#include "PlayerManager.h"
#include "i18n.h"

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "winmm.lib")

// D3D11 globals
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;
static bool                     g_resizeRequested = false;
static UINT                     g_resizeW = 0, g_resizeH = 0;

// Window globals
static HWND  g_hwnd = nullptr;
static float g_dpiScale = 1.0f;
static bool  g_fontRebuildNeeded = false;
static float g_pendingDpiScale = 1.0f;
static const float TITLE_BAR_HEIGHT_DP = 40.0f; // logical dp

// Forward declarations
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
void LoadFonts(ImGuiIO& io, float dpiScale);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Helper: scaled value — delegates to UITheme which is set from g_dpiScale
// (S() is already defined in UI_Pages.h via UITheme::S)

// Active page
static int g_activePage = 0;  // 0=Dashboard, 1=AddDevice, 2=LayoutMgr, 3=MouseSettings

// Sidebar navigation items
struct NavItem {
    const char* labelKey;
};

static NavItem g_navItems[] = {
    { "nav_dashboard" },
    { "nav_add_device" },
    { "nav_layout_mgr" },
    { "nav_mouse_settings" },
};
static const int NAV_COUNT = 4;

// Font path relative to exe
static const char* FONT_PATH = "resources/NotoSansSC-Regular.otf";

// ---------- Title Bar rendering (frameless window) ----------
void RenderTitleBar() {
    float titleH = S(TITLE_BAR_HEIGHT_DP);
    float windowW = ImGui::GetWindowWidth();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 wPos = ImGui::GetWindowPos();

    // Background
    dl->AddRectFilled(wPos, ImVec2(wPos.x + windowW, wPos.y + titleH),
        ImGui::GetColorU32(UITheme::Sidebar));
    // Bottom separator line
    dl->AddLine(ImVec2(wPos.x, wPos.y + titleH - 1),
        ImVec2(wPos.x + windowW, wPos.y + titleH - 1),
        ImGui::GetColorU32(UITheme::Divider));

    // App icon (colored rounded square)
    float iconSz = S(14);
    float iconY = (titleH - iconSz) * 0.5f;
    dl->AddRectFilled(
        ImVec2(wPos.x + S(14), wPos.y + iconY),
        ImVec2(wPos.x + S(14) + iconSz, wPos.y + iconY + iconSz),
        ImGui::GetColorU32(UITheme::Primary), S(3));

    // Title text
    const char* title = "JoyCon2 Connector";
    float textY = wPos.y + (titleH - ImGui::GetTextLineHeight()) * 0.5f;
    dl->AddText(ImVec2(wPos.x + S(36), textY),
        ImGui::GetColorU32(UITheme::TextSecondary), title);

    // Window control buttons (right side)
    float btnW = S(46);
    float btnH = titleH;
    float btnStartX = wPos.x + windowW - btnW * 3;

    // --- Minimize ---
    {
        ImVec2 bMin(btnStartX, wPos.y);
        ImVec2 bMax(btnStartX + btnW, wPos.y + btnH);
        bool hovered = ImGui::IsMouseHoveringRect(bMin, bMax);
        if (hovered)
            dl->AddRectFilled(bMin, bMax, IM_COL32(0, 0, 0, 26));
        float cy = wPos.y + btnH * 0.5f;
        float cx = btnStartX + btnW * 0.5f;
        dl->AddLine(ImVec2(cx - S(5), cy), ImVec2(cx + S(5), cy),
            ImGui::GetColorU32(UITheme::TextPrimary), S(1.0f));
        ImGui::SetCursorScreenPos(bMin);
        if (ImGui::InvisibleButton("##titlemin", ImVec2(btnW, btnH)))
            ShowWindow(g_hwnd, SW_MINIMIZE);
    }

    // --- Maximize / Restore ---
    {
        float bx = btnStartX + btnW;
        ImVec2 bMin(bx, wPos.y);
        ImVec2 bMax(bx + btnW, wPos.y + btnH);
        bool hovered = ImGui::IsMouseHoveringRect(bMin, bMax);
        if (hovered)
            dl->AddRectFilled(bMin, bMax, IM_COL32(0, 0, 0, 26));
        float cx = bx + btnW * 0.5f;
        float cy = wPos.y + btnH * 0.5f;
        float sz = S(4.5f);
        bool maximized = IsZoomed(g_hwnd);
        if (maximized) {
            float off = S(1.5f);
            dl->AddRect(ImVec2(cx - sz + off, cy - sz - off), ImVec2(cx + sz + off, cy + sz - off),
                ImGui::GetColorU32(UITheme::TextPrimary), 0, 0, S(1.0f));
            dl->AddRectFilled(ImVec2(cx - sz - off, cy - sz + off), ImVec2(cx + sz - off, cy + sz + off),
                ImGui::GetColorU32(UITheme::Sidebar));
            dl->AddRect(ImVec2(cx - sz - off, cy - sz + off), ImVec2(cx + sz - off, cy + sz + off),
                ImGui::GetColorU32(UITheme::TextPrimary), 0, 0, S(1.0f));
        } else {
            dl->AddRect(ImVec2(cx - sz, cy - sz), ImVec2(cx + sz, cy + sz),
                ImGui::GetColorU32(UITheme::TextPrimary), 0, 0, S(1.0f));
        }
        ImGui::SetCursorScreenPos(bMin);
        if (ImGui::InvisibleButton("##titlemax", ImVec2(btnW, btnH)))
            ShowWindow(g_hwnd, maximized ? SW_RESTORE : SW_MAXIMIZE);
    }

    // --- Close ---
    {
        float bx = btnStartX + btnW * 2;
        ImVec2 bMin(bx, wPos.y);
        ImVec2 bMax(bx + btnW, wPos.y + btnH);
        bool hovered = ImGui::IsMouseHoveringRect(bMin, bMax);
        if (hovered)
            dl->AddRectFilled(bMin, bMax, IM_COL32(0xC4, 0x2B, 0x1C, 0xFF));
        float cx = bx + btnW * 0.5f;
        float cy = wPos.y + btnH * 0.5f;
        float xsz = S(5);
        ImU32 xCol = hovered ? IM_COL32(255, 255, 255, 255) : ImGui::GetColorU32(UITheme::TextPrimary);
        dl->AddLine(ImVec2(cx - xsz, cy - xsz), ImVec2(cx + xsz, cy + xsz), xCol, S(1.0f));
        dl->AddLine(ImVec2(cx + xsz, cy - xsz), ImVec2(cx - xsz, cy + xsz), xCol, S(1.0f));
        ImGui::SetCursorScreenPos(bMin);
        if (ImGui::InvisibleButton("##titleclose", ImVec2(btnW, btnH)))
            PostMessage(g_hwnd, WM_CLOSE, 0, 0);
    }
}

// ---------- Sidebar rendering ----------
void RenderSidebar() {
    float titleH = S(TITLE_BAR_HEIGHT_DP);
    float sidebarW = S(220);
    ImGui::SetCursorPos(ImVec2(0, titleH));

    ImGui::PushStyleColor(ImGuiCol_ChildBg, UITheme::Sidebar);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::BeginChild("Sidebar", ImVec2(sidebarW, ImGui::GetWindowHeight() - titleH), ImGuiChildFlags_None);

    // App title
    ImGui::SetCursorPos(ImVec2(S(20), S(16)));
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts.Size > 1 ? ImGui::GetIO().Fonts->Fonts[1] : nullptr);
    ImGui::TextColored(UITheme::Primary, "JoyCon2");
    if (ImGui::GetIO().Fonts->Fonts.Size > 1) ImGui::PopFont();
    ImGui::SetCursorPosX(S(20));
    ImGui::TextColored(UITheme::TextTertiary, "Connector");

    ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();

    // Navigation items
    for (int i = 0; i < NAV_COUNT; ++i) {
        ImGui::PushID(i);
        bool selected = (g_activePage == i);

        float itemH = S(44);
        float padLeft = S(12);
        float padRight = S(12);
        float totalW = sidebarW - padLeft - padRight;

        ImGui::SetCursorPosX(padLeft);
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();

        // Background
        ImVec4 bgColor = selected ? UITheme::SidebarActive : ImVec4(0, 0, 0, 0);
        if (!selected && ImGui::IsMouseHoveringRect(cursorPos, ImVec2(cursorPos.x + totalW, cursorPos.y + itemH)))
            bgColor = UITheme::SidebarHover;

        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(
            ImVec2(cursorPos.x, cursorPos.y),
            ImVec2(cursorPos.x + totalW, cursorPos.y + itemH),
            ImGui::GetColorU32(bgColor), S(22));

        // Active indicator bar
        if (selected) {
            dl->AddRectFilled(
                ImVec2(cursorPos.x, cursorPos.y + S(8)),
                ImVec2(cursorPos.x + S(4), cursorPos.y + itemH - S(8)),
                ImGui::GetColorU32(UITheme::Primary), S(2));
        }

        // Text
        ImVec4 textColor = selected ? UITheme::Primary : UITheme::SidebarText;
        float textY = cursorPos.y + (itemH - ImGui::GetTextLineHeight()) * 0.5f;
        dl->AddText(ImVec2(cursorPos.x + S(20), textY), ImGui::GetColorU32(textColor), T(g_navItems[i].labelKey));

        // Click
        ImGui::SetCursorPosX(padLeft);
        if (ImGui::InvisibleButton("navbtn", ImVec2(totalW, itemH))) {
            g_activePage = i;
            if (i == 1) g_wizard.Reset();
        }
        ImGui::PopID();
    }

    // Bottom: Language toggle
    float bottomY = ImGui::GetWindowHeight() - S(60);
    ImGui::SetCursorPos(ImVec2(S(20), bottomY));
    ImGui::TextColored(UITheme::TextTertiary, "%s", T("nav_language"));
    ImGui::SetCursorPosX(S(20));

    ImGui::PushStyleColor(ImGuiCol_Button, UITheme::ButtonSecondary);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, UITheme::ButtonSecondaryHov);
    ImGui::PushStyleColor(ImGuiCol_Text, UITheme::TextPrimary);
    if (ImGui::Button(g_currentLang == Lang::EN ? "English" : u8"\u4E2D\u6587", ImVec2(S(80), S(32)))) {
        g_currentLang = (g_currentLang == Lang::EN) ? Lang::ZH : Lang::EN;
    }
    ImGui::PopStyleColor(3);

    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

// ---------- Font loading helper ----------
void LoadFonts(ImGuiIO& io, float dpiScale) {
    ImFontConfig fontConfig;
    fontConfig.OversampleH = 2;
    fontConfig.OversampleV = 1;

    // Build font path relative to exe
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string exeDir(exePath);
    exeDir = exeDir.substr(0, exeDir.find_last_of("\\/") + 1);
    std::string fontPath = exeDir + FONT_PATH;

    const char* fontPaths[] = {
        fontPath.c_str(),
        FONT_PATH,
        "testapp/resources/NotoSansSC-Regular.otf",
        "../resources/NotoSansSC-Regular.otf",
    };

    bool fontLoaded = false;
    for (const char* fp : fontPaths) {
        FILE* f = fopen(fp, "rb");
        if (f) {
            fclose(f);
            static const ImWchar ranges[] = {
                0x0020, 0x00FF, // Basic Latin
                0x2000, 0x206F, // General Punctuation
                0x3000, 0x30FF, // CJK Symbols, Hiragana, Katakana
                0x31F0, 0x31FF, // Katakana Extension
                0x4E00, 0x9FFF, // CJK Unified Ideographs
                0xFF00, 0xFFEF, // Halfwidth and Fullwidth Forms
                0x2600, 0x26FF, // Miscellaneous Symbols
                0x2700, 0x27BF, // Dingbats
                0xFE00, 0xFE0F, // Variation Selectors
                0,
            };
            io.Fonts->AddFontFromFileTTF(fp, 16.0f * dpiScale, &fontConfig, ranges);
            fontConfig.MergeMode = false;
            io.Fonts->AddFontFromFileTTF(fp, 22.0f * dpiScale, nullptr, ranges);
            fontLoaded = true;
            break;
        }
    }

    if (!fontLoaded) {
        io.Fonts->AddFontDefault();
        io.Fonts->AddFontDefault();
    }

    io.Fonts->Build();
}

// ---------- Main entry ----------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    winrt::init_apartment();

    // Enable Per-Monitor DPI Awareness V2
    ImGui_ImplWin32_EnableDpiAwareness();

    // Register window class
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
    wc.lpszClassName = L"JoyCon2ConnectorClass";
    RegisterClassExW(&wc);

    // Get initial DPI for sizing
    // Use the primary monitor DPI for initial window placement
    HDC hdc = GetDC(NULL);
    float initDpi = (float)GetDeviceCaps(hdc, LOGPIXELSX) / 96.0f;
    ReleaseDC(NULL, hdc);
    g_dpiScale = initDpi;
    UITheme::DpiScale = g_dpiScale;

    int initW = (int)(960 * g_dpiScale);
    int initH = (int)(640 * g_dpiScale);

    // Frameless window: WS_POPUP with resize border support
    HWND hwnd = CreateWindowExW(
        0, wc.lpszClassName, L"JoyCon2 Connector",
        WS_POPUP | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, initW, initH,
        NULL, NULL, hInstance, NULL);
    g_hwnd = hwnd;

    // Enable DWM shadow for frameless window
    MARGINS margins = { 1, 1, 1, 1 };
    DwmExtendFrameIntoClientArea(hwnd, &margins);

    // Update DPI from the actual window's monitor
    g_dpiScale = ImGui_ImplWin32_GetDpiScaleForHwnd(hwnd);
    UITheme::DpiScale = g_dpiScale;

    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        UnregisterClassW(wc.lpszClassName, hInstance);
        return 1;
    }

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    // ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    // Load fonts at DPI-scaled sizes
    LoadFonts(io, g_dpiScale);

    // Init platform/renderer
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Apply theme
    UITheme::Apply();

    // Increase system timer resolution to 1ms for responsive input
    timeBeginPeriod(1);

    // Init managers
    ViGEmManager::Instance().Initialize();
    ConfigManager::Instance().Load();
    ConfigManager::Instance().EnsureDefaults();

    // Clear color
    float clearColor[4] = { 0.96f, 0.94f, 0.92f, 1.0f };

    // Main loop
    bool running = true;
    MSG msg;
    while (running) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) running = false;
        }
        if (!running) break;

        // Handle DPI change — rebuild fonts
        if (g_fontRebuildNeeded) {
            g_dpiScale = g_pendingDpiScale;
            UITheme::DpiScale = g_dpiScale;

            ImGui_ImplDX11_InvalidateDeviceObjects();
            io.Fonts->Clear();
            LoadFonts(io, g_dpiScale);
            ImGui_ImplDX11_CreateDeviceObjects();

            UITheme::Apply();
            g_fontRebuildNeeded = false;
        }

        // Handle resize
        if (g_resizeRequested) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_resizeW, g_resizeH, DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
            g_resizeRequested = false;
        }

        // Start ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Full-window ImGui panel
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("MainWindow", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar);
        ImGui::PopStyleVar();

        // Custom title bar
        RenderTitleBar();

        // Layout: Sidebar + Content (below title bar)
        float titleH = S(TITLE_BAR_HEIGHT_DP);
        RenderSidebar();
        ImGui::SameLine();

        // Content area
        ImGui::SetCursorPosY(titleH);
        ImGui::BeginChild("ContentArea", ImVec2(0, ImGui::GetWindowHeight() - titleH), ImGuiChildFlags_None);
        switch (g_activePage) {
        case 0: RenderDashboard(); break;
        case 1: RenderAddDevice(g_activePage); break;
        case 2: RenderLayoutManager(); break;
        case 3: RenderMouseSettings(); break;
        }
        ImGui::EndChild();

        ImGui::End();

        // Render
        ImGui::Render();
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clearColor);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0);
    }

    // Cleanup
    PlayerManager::Instance().Shutdown();
    ViGEmManager::Instance().Shutdown();
    ConfigManager::Instance().Save();
    timeEndPeriod(1);

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, hInstance);

    return 0;
}

// ---------- D3D11 Helpers ----------
bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd,
        &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr,
            createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd,
            &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK) return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED) {
            g_resizeW = (UINT)LOWORD(lParam);
            g_resizeH = (UINT)HIWORD(lParam);
            g_resizeRequested = true;
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_NCCALCSIZE: {
        // Remove the standard window frame entirely
        if (wParam == TRUE) {
            NCCALCSIZE_PARAMS* params = (NCCALCSIZE_PARAMS*)lParam;
            if (IsZoomed(hWnd)) {
                // When maximized, adjust to work area to avoid extending past screen
                HMONITOR hMon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
                MONITORINFO mi = { sizeof(mi) };
                GetMonitorInfo(hMon, &mi);
                params->rgrc[0] = mi.rcWork;
            }
            return 0;
        }
        break;
    }

    case WM_NCHITTEST: {
        // Custom hit testing for frameless window
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        ScreenToClient(hWnd, &pt);

        RECT rc;
        GetClientRect(hWnd, &rc);

        int borderW = (int)S(5);
        float titleH = S(TITLE_BAR_HEIGHT_DP);
        float btnRegionW = S(46 * 3);

        // Don't handle resize borders when maximized
        if (!IsZoomed(hWnd)) {
            if (pt.y < borderW) {
                if (pt.x < borderW) return HTTOPLEFT;
                if (pt.x > rc.right - borderW) return HTTOPRIGHT;
                return HTTOP;
            }
            if (pt.y > rc.bottom - borderW) {
                if (pt.x < borderW) return HTBOTTOMLEFT;
                if (pt.x > rc.right - borderW) return HTBOTTOMRIGHT;
                return HTBOTTOM;
            }
            if (pt.x < borderW) return HTLEFT;
            if (pt.x > rc.right - borderW) return HTRIGHT;
        }

        // Title bar area
        if (pt.y < (int)titleH) {
            // Window control buttons region → let ImGui handle
            if (pt.x > rc.right - (int)btnRegionW)
                return HTCLIENT;
            // Rest of title bar → draggable
            return HTCAPTION;
        }

        return HTCLIENT;
    }

    case WM_DPICHANGED: {
        float newDpi = HIWORD(wParam) / 96.0f;
        g_pendingDpiScale = newDpi;
        g_fontRebuildNeeded = true;
        RECT* suggested = (RECT*)lParam;
        SetWindowPos(hWnd, nullptr, suggested->left, suggested->top,
            suggested->right - suggested->left, suggested->bottom - suggested->top,
            SWP_NOZORDER | SWP_NOACTIVATE);
        return 0;
    }

    case WM_GETMINMAXINFO: {
        LPMINMAXINFO mmi = (LPMINMAXINFO)lParam;
        mmi->ptMinTrackSize.x = (LONG)S(700);
        mmi->ptMinTrackSize.y = (LONG)S(450);
        return 0;
    }
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}
