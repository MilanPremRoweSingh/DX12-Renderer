#include "Shell.h"

#include <winuser.h>

const wchar_t* lpszWindowClassName = L"Window Class Name";
const wchar_t* lpszWindowName = L"Window Class Name";

HWND hWnd = NULL;

static void sParseCmdLine()
{
    int nNumArgs;
    LPWSTR* plpArgs = CommandLineToArgvW(GetCommandLineW(), &nNumArgs);

    if (nNumArgs > 1)
    {
        for (int32 i = 1; i < nNumArgs; i++)
        {
            if (wcscmp(plpArgs[i], L"-d3ddebug") == 0)
            {
                tGlobals.fD3DDebug = true;
            }
        }
    }
}

static DWORD sGetWindowStyle()
{
    return WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
}

static LRESULT CALLBACK sWindowProc(
    _In_ HWND   hwnd,
    _In_ UINT   uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
)
{
    LRESULT result = 0;
    switch (wParam)
    {
        default:
        {
            result = DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
    }

    return result;
}

static void sCreateWindow(
    HINSTANCE hInstance)
{
    WNDCLASS wc;

    wc = {};
    wc.lpfnWndProc = sWindowProc; 
    wc.hInstance = hInstance;   
    wc.lpszClassName = lpszWindowClassName;
    wc.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));

    RegisterClass(&wc);

    hWnd = CreateWindowEx(
        0,
        lpszWindowClassName,
        lpszWindowName,
        sGetWindowStyle(),
        CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL,
        NULL,
        hInstance,
        NULL);

    if (hWnd)
    {
        ShowWindow(hWnd, SW_SHOW);
    }
}

void ShellInitialise(
    HINSTANCE hInstance)
{
    sParseCmdLine();
    sCreateWindow(hInstance);
}

void ProcessWindowMessages()
{
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

bool WindowExists()
{
    return IsWindow(hWnd);
}

int32 GetWindowWidth()
{
    return WINDOW_WIDTH;
}

int32 GetWindowHeight()
{
    return WINDOW_HEIGHT;
}
