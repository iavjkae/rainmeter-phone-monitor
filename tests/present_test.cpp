// SPDX-License-Identifier: MIT
#include <d3d11.h>
#include <windows.h>
int WINAPI wWinMain(HINSTANCE h, HINSTANCE, LPWSTR, int) {
    SetProcessDPIAware();
    WNDCLASSW wc{};
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = h;
    wc.lpszClassName = L"PhoneFPSTest";
    RegisterClassW(&wc);
    HWND win = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, wc.lpszClassName,
                               L"FPS capture verification", WS_POPUP, 3700, 2000, 64, 64, nullptr,
                               nullptr, h, nullptr);
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferDesc.Width = 64;
    sd.BufferDesc.Height = 64;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.SampleDesc.Count = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 2;
    sd.OutputWindow = win;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    ID3D11Device *d = nullptr;
    ID3D11DeviceContext *ctx = nullptr;
    IDXGISwapChain *swap = nullptr;
    HRESULT hr =
        D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
                                      D3D11_SDK_VERSION, &sd, &swap, &d, nullptr, &ctx);
    if (FAILED(hr))
        return 2;
    ID3D11Texture2D *tex = nullptr;
    ID3D11RenderTargetView *view = nullptr;
    swap->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&tex);
    d->CreateRenderTargetView(tex, nullptr, &view);
    tex->Release();
    ShowWindow(win, SW_SHOWNOACTIVATE);
    auto start = GetTickCount64();
    while (GetTickCount64() - start < 14000) {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        float color[] = {.12f, .28f, .45f, 1.f};
        ctx->ClearRenderTargetView(view, color);
        swap->Present(0, 0);
        Sleep(16);
    }
    view->Release();
    swap->Release();
    ctx->Release();
    d->Release();
    DestroyWindow(win);
    return 0;
}
