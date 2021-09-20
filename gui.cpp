#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <d3d11.h>
#include <tchar.h>
#include <cassert>
#include <stdexcept>
#include <system_error>
#include <vector>
#include "gui.h"

// Data
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr; 
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Forward declarations of helper functions
HWND CreateHWND(std::size_t win_height , std::size_t win_width) {
    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    auto wc = WNDCLASSEX{ 
        sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, 
        GetModuleHandle(NULL), NULL, NULL, NULL, NULL, 
        _T("Fluid Demo"), NULL 
    };

    ::RegisterClassEx(&wc);
    auto hwnd = ::CreateWindow(wc.lpszClassName, _T("Demo"), WS_OVERLAPPEDWINDOW^WS_THICKFRAME, 100, 100, win_width, win_height, NULL, NULL, wc.hInstance, NULL);
    assert(hwnd);
    
    auto rec1 = RECT{};
    GetWindowRect(hwnd,&rec1);  
    auto rec2 = RECT{};
    GetClientRect(hwnd,&rec2);  
    int newWidth = win_width +(rec1.right-rec1.left)-(rec2.right-rec2.left) + 16;
    int newHeight= win_height +(rec1.bottom-rec1.top)-(rec2.bottom-rec2.top) + 16;
    MoveWindow(hwnd,rec1.left , rec1.top,newWidth,newHeight,false);

    return hwnd;
}

void CreateRenderTarget(){
    ID3D11Texture2D* pBackBuffer{};
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

bool CreateDeviceD3D(HWND hWnd){
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
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
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupRenderTarget(){
    if (g_mainRenderTargetView) {
        g_mainRenderTargetView->Release(); 
        g_mainRenderTargetView = nullptr; 
    }
}
void CleanupDeviceD3D(){
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }

}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam){
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg){
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED){
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

struct D3DResDeleter{
    void operator()(ID3D11Texture2D * p) const noexcept {
        if(p) p->Release();
    }
};

struct GUI::Impl{
    // windows info
    std::size_t height;
    std::size_t width;
    HWND hwnd{};

    ID3D11ShaderResourceView * texture_view{};
    std::unique_ptr<ID3D11Texture2D , D3DResDeleter>  texture{};
};

static auto make_sys_error() -> std::system_error{
    return std::error_code(::GetLastError() , std::system_category());
}

auto GetTexture2D(std::uint32_t height,  std::uint32_t width) {
    auto image_data = std::vector<RGBA>(height * width , RGBA{0,0,0,255});

    // Create texture
    D3D11_TEXTURE2D_DESC desc{
        .Width = width , 
        .Height = height ,
        .MipLevels  =1,
        .ArraySize = 1 ,
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM ,
        .SampleDesc = DXGI_SAMPLE_DESC{.Count = 1 , .Quality = 0},
        .Usage = D3D11_USAGE_DYNAMIC ,//D3D11_USAGE_DEFAULT ,
        .BindFlags = D3D11_BIND_SHADER_RESOURCE ,
        .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE
    };

    ID3D11Texture2D *pTexture {};
    D3D11_SUBRESOURCE_DATA subResource{
        .pSysMem = image_data.data(),
        .SysMemPitch = desc.Width * 4, 
        .SysMemSlicePitch = 0 ,
    };

    g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);

    ID3D11ShaderResourceView * pTextID {};

    // Create texture view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
        .Texture2D = D3D11_TEX2D_SRV{.MostDetailedMip = 0 , .MipLevels = desc.MipLevels }
    };
    g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, &pTextID);

    return std::pair{ pTextID , pTexture };
}

GUI::GUI(std::size_t height , std::size_t width) 
: m_impl(std::make_unique<Impl>(height, width)){

    if(auto hwnd = CreateHWND(height , width) ; hwnd ) {
        m_impl->hwnd = hwnd ;
    }else{
        throw make_sys_error();
    } 
        
    if(!CreateDeviceD3D(m_impl->hwnd)) {
        throw make_sys_error();
    }
        
    ::ShowWindow(m_impl->hwnd  , SW_SHOWDEFAULT);
    ::UpdateWindow(m_impl->hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    // auto & io = ImGui::GetIO();
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard ;
    // ImGui::StyleColorsLight();
    ImGui::StyleColorsClassic();
    ImGui_ImplWin32_Init(m_impl->hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice , g_pd3dDeviceContext);

    auto [texid , texture] = GetTexture2D(height , width);
    m_impl->texture_view = texid ;
    m_impl->texture.reset(texture);
}

GUI::~GUI(){
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(m_impl->hwnd);
}

void GUI::SetWindowsTitle(std::string str){
    if(m_impl->hwnd) 
        ::SetWindowText(m_impl->hwnd , str.data());
}

// return true as quit
bool GUI::ProcessMessage(){
    auto msg = MSG{};
    while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)){
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
        if (msg.message == WM_QUIT) return true;
    }
    return false;
}

void GUI::RenderBegin(){
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    auto viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    
    auto flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse ;

    ImGui::Begin("fullscreen" , nullptr , flags | ImGuiWindowFlags_NoTitleBar);
    ImGui::Image(m_impl->texture_view , ImVec2(m_impl->height ,m_impl->width));
    ImGui::End();
}

void GUI::RenderEnd(){
    ImGui::Render();
}

void GUI::Update(){
    static const float clear_color[4] = {0.45f, 0.55f, 0.60f, 1.00f};
    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView , nullptr );
    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    g_pSwapChain->Present(1, 0);
}

void GUI::UpdateFrameBuffer(std::span<const RGBA> src){
    assert(src.data());
    auto map_res = D3D11_MAPPED_SUBRESOURCE{};
    auto hr = g_pd3dDeviceContext->Map(m_impl->texture.get() , 0 , D3D11_MAP_WRITE_DISCARD , 0 , &map_res);
    assert(hr == S_OK);
    auto data = reinterpret_cast<std::byte*>(map_res.pData) ;
    auto src_ptr = src.data();
    for(int i = 0 ; i < m_impl->height ; ++i) {
        memcpy(data , src_ptr , sizeof(RGBA) * m_impl->width);
        data += map_res.RowPitch;
        src_ptr += m_impl->width ;
    }
    g_pd3dDeviceContext->Unmap(m_impl->texture.get() ,0);
}