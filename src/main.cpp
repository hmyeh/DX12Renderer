#include <Windows.h>
#include <shellapi.h> // For CommandLineToArgvW

#include "application.h"
#include "utility.h"

// Use WARP adapter
bool g_UseWarp = false;
//
uint32_t g_ClientWidth = 1280;
uint32_t g_ClientHeight = 720;


void ParseCommandLineArguments()
{
    int argc;
    wchar_t** argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);

    for (size_t i = 0; i < argc; ++i)
    {
        if (::wcscmp(argv[i], L"-w") == 0 || ::wcscmp(argv[i], L"--width") == 0)
        {
            g_ClientWidth = ::wcstol(argv[++i], nullptr, 10);
        }
        if (::wcscmp(argv[i], L"-h") == 0 || ::wcscmp(argv[i], L"--height") == 0)
        {
            g_ClientHeight = ::wcstol(argv[++i], nullptr, 10);
        }
        if (::wcscmp(argv[i], L"-warp") == 0 || ::wcscmp(argv[i], L"--warp") == 0)
        {
            g_UseWarp = true;
        }
    }

    // Free memory allocated by CommandLineToArgvW
    ::LocalFree(argv);
}


int CALLBACK wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR lpCmdLine, _In_ int nCmdShow)
{
    ParseCommandLineArguments();
    
    // Initialize required for DirectXTex library https://github.com/microsoft/DirectXTex/wiki/DirectXTex
    ThrowIfFailed(CoInitializeEx(nullptr, COINIT_MULTITHREADED));

    Application app(hInstance, L"DX12 Renderer", g_ClientWidth, g_ClientHeight);
    app.Show();

    return 0;
}