// App.cpp - Main entry point for JoyCon2 Connector
// DirectX 11 + Dear ImGui application with Material Design 3 sidebar navigation
#include <Windows.h>
#include <d3d11.h>
#include <tchar.h>
#include <string>

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

// D3D11 globals
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;
static bool                     g_resizeRequested = false;
static UINT                     g_resizeW = 0, g_resizeH = 0;

// Forward declarations
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Active page
static int g_activePage = 0;  // 0=Dashboard, 1=AddDevice, 2=LayoutMgr, 3=MouseSettings

// Sidebar navigation items with icons (Unicode symbols from font)
struct NavItem {
    const char* labelKey;
    const char* icon;  // UTF-8 icon character
};

static NavItem g_navItems[] = {
    { "nav_dashboard",      u8"\xF0\x9F\x8E\xAE" },  // 🎮
    { "nav_add_device",     u8"\xE2\x9E\x95" },       // ➕
    { "nav_layout_mgr",     u8"\xF0\x9F\x94\xA7" },  // 🔧
    { "nav_mouse_settings", u8"\xF0\x9F\x96\xB1" },  // 🖱
};
static const int NAV_COUNT = 4;

// Font path relative to exe
static const char* FONT_PATH = "resources/NotoSansSC-Regular.otf";

// ---------- Sidebar rendering ----------
void RenderSidebar() {
    float sidebarW = 220.0f;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, UITheme::Sidebar);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::BeginChild("Sidebar", ImVec2(sidebarW, 0), ImGuiChildFlags_None);

    // App title
    ImGui::SetCursorPos(ImVec2(20, 20));
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts.Size > 1 ? ImGui::GetIO().Fonts->Fonts[1] : nullptr);
    ImGui::TextColored(UITheme::Primary, "JoyCon2");
    if (ImGui::GetIO().Fonts->Fonts.Size > 1) ImGui::PopFont();
    ImGui::SetCursorPosX(20);
    ImGui::TextColored(UITheme::TextTertiary, "Connector");

    ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();

    // Navigation items
    for (int i = 0; i < NAV_COUNT; ++i) {
        ImGui::PushID(i);
        bool selected = (g_activePage == i);

        float itemH = 44.0f;
        float padLeft = 12.0f;
        float padRight = 12.0f;
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
            ImGui::GetColorU32(bgColor), 22.0f);

        // Active indicator bar
        if (selected) {
            dl->AddRectFilled(
                ImVec2(cursorPos.x, cursorPos.y + 8),
                ImVec2(cursorPos.x + 4, cursorPos.y + itemH - 8),
                ImGui::GetColorU32(UITheme::Primary), 2.0f);
        }

        // Text
        ImVec4 textColor = selected ? UITheme::Primary : UITheme::SidebarText;
        float textY = cursorPos.y + (itemH - ImGui::GetTextLineHeight()) * 0.5f;
        dl->AddText(ImVec2(cursorPos.x + 20, textY), ImGui::GetColorU32(textColor), T(g_navItems[i].labelKey));

        // Click
        ImGui::SetCursorPosX(padLeft);
        if (ImGui::InvisibleButton("navbtn", ImVec2(totalW, itemH))) {
            g_activePage = i;
            if (i == 1) g_wizard.Reset(); // Reset wizard when navigating to Add Device
        }
        ImGui::PopID();
    }

    // Bottom: Language toggle
    float bottomY = ImGui::GetWindowHeight() - 60;
    ImGui::SetCursorPos(ImVec2(20, bottomY));
    ImGui::TextColored(UITheme::TextTertiary, "%s", T("nav_language"));
    ImGui::SetCursorPosX(20);
    
    ImGui::PushStyleColor(ImGuiCol_Button, UITheme::ButtonSecondary);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, UITheme::ButtonSecondaryHov);
    ImGui::PushStyleColor(ImGuiCol_Text, UITheme::TextPrimary);
    if (ImGui::Button(g_currentLang == Lang::EN ? "English" : u8"\u4E2D\u6587", ImVec2(80, 30))) {
        g_currentLang = (g_currentLang == Lang::EN) ? Lang::ZH : Lang::EN;
    }
    ImGui::PopStyleColor(3);

    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

// ---------- Main entry ----------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    winrt::init_apartment();

    // Register window class
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.lpszClassName = L"JoyCon2ConnectorClass";
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(
        0, wc.lpszClassName, L"JoyCon2 \u8FDE\u63A5\u5668 - JoyCon2 Connector",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 960, 640,
        NULL, NULL, hInstance, NULL);

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
    io.IniFilename = nullptr; // Don't save imgui.ini

    // Fonts
    ImFontConfig fontConfig;
    fontConfig.OversampleH = 2;
    fontConfig.OversampleV = 1;

    // Try to load CJK font
    ImFont* mainFont = nullptr;
    ImFont* titleFont = nullptr;

    // Build font path relative to exe
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string exeDir(exePath);
    exeDir = exeDir.substr(0, exeDir.find_last_of("\\/") + 1);
    std::string fontPath = exeDir + FONT_PATH;

    // Also try relative to working directory
    bool fontLoaded = false;
    const char* fontPaths[] = {
        fontPath.c_str(),
        FONT_PATH,
        "testapp/resources/NotoSansSC-Regular.otf",
        "../resources/NotoSansSC-Regular.otf",
    };

    for (const char* fp : fontPaths) {
        FILE* f = fopen(fp, "rb");
        if (f) {
            fclose(f);
            // Main font (16px)
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
            mainFont = io.Fonts->AddFontFromFileTTF(fp, 16.0f, &fontConfig, ranges);
            fontConfig.MergeMode = false;
            titleFont = io.Fonts->AddFontFromFileTTF(fp, 22.0f, nullptr, ranges);
            fontLoaded = true;
            break;
        }
    }

    if (!fontLoaded) {
        mainFont = io.Fonts->AddFontDefault();
        titleFont = io.Fonts->AddFontDefault();
    }

    io.Fonts->Build();

    // Init platform/renderer
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Apply theme
    UITheme::Apply();

    // Init managers
    ViGEmManager::Instance().Initialize();
    ConfigManager::Instance().Load();
    ConfigManager::Instance().EnsureDefaults();

    // Clear color
    float clearColor[4] = { 0.96f, 0.94f, 0.92f, 1.0f }; // Match Surface color

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

        // Layout: Sidebar + Content
        RenderSidebar();
        ImGui::SameLine();

        // Content area
        ImGui::BeginChild("ContentArea", ImVec2(0, 0), ImGuiChildFlags_None);
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

        g_pSwapChain->Present(1, 0); // VSync
    }

    // Cleanup
    PlayerManager::Instance().Shutdown();
    ViGEmManager::Instance().Shutdown();
    ConfigManager::Instance().Save();

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
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}
