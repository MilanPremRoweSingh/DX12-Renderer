#include "Shell.h"

#include "Engine.h"
#include <winuser.h>
#include <string>

const wchar_t* lpszWindowClassName = L"Window Class Name";
const wchar_t* lpszWindowName = L"Window Class Name";

HWND hWnd = NULL;
bool trapCursor;

POINTS prevMousePos;

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
                globals.fD3DDebug = true;
            }

            if (wcscmp(plpArgs[i], L"-gpuvalidation") == 0)
            {
                globals.fD3DDebug = true;
                globals.fGPUValidation = true;
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
    switch (uMsg)
    {
        case WM_MOUSEMOVE:
        {
            if (trapCursor)
            {
                POINT mousePos;
                GetCursorPos(&mousePos);

                RECT wRect;
                GetWindowRect(hWnd, &wRect);
                LONG width = wRect.right - wRect.left;
                LONG height = wRect.bottom - wRect.top;
                POINT origin = { (wRect.left + width / 2), wRect.top + height / 2};
                if (mousePos.x != origin.x || mousePos.y != origin.y)
                { 
                    SetCursorPos(origin.x, origin.y);
                }

                Vector2 input = {
                    (mousePos.x - origin.x) / float(width) ,
                    (mousePos.y - origin.y) / float(height) };

                globals.totalMouseDelta += input;

                goto _default;
            }
        } break;

        case WM_KEYDOWN:
        {
            if (((1 << 30) & lParam) == 0)
            {
                globals.cameraMoveDirection.z += (wParam == 'W' || wParam == 'w');
                globals.cameraMoveDirection.z -= (wParam == 'S' || wParam == 's');
                globals.cameraMoveDirection.x += (wParam == 'D' || wParam == 'd');
                globals.cameraMoveDirection.x -= (wParam == 'A' || wParam == 'a');
                globals.cameraMoveDirection.x = Utils::Pin(globals.cameraMoveDirection.x, -1.0f, 1.0f);
                globals.cameraMoveDirection.z = Utils::Pin(globals.cameraMoveDirection.z, -1.0f, 1.0f);
            }
        }

        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        {
            trapCursor = true;
            ShowCursor(false);
        } break;

        case WM_KEYUP:
        {
            if (wParam == VK_ESCAPE)
            {
                trapCursor = false;
                ShowCursor(true);
            } 
            else
            {
                globals.cameraMoveDirection.z -= (wParam == 'W' || wParam == 'w');
                globals.cameraMoveDirection.z += (wParam == 'S' || wParam == 's');
                globals.cameraMoveDirection.x -= (wParam == 'D' || wParam == 'd');
                globals.cameraMoveDirection.x += (wParam == 'A' || wParam == 'a');
                globals.cameraMoveDirection.x = Utils::Pin(globals.cameraMoveDirection.x, -1.0f, 1.0f);
                globals.cameraMoveDirection.z = Utils::Pin(globals.cameraMoveDirection.z, -1.0f, 1.0f);
            }
        }

        default:
            goto _default;
    }

    return result;
_default:
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
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
        trapCursor = true;
        ShowCursor(false);
    }
    DeleteObject(wc.hbrBackground);
}

void ShellInitialise(
    HINSTANCE hInstance)
{
    sParseCmdLine();
    sCreateWindow(hInstance);
}

void ShellDispose(
    HINSTANCE hInstance)
{
    UnregisterClass(lpszWindowClassName, hInstance);
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

float GetWindowAspectRatio()
{
    return float(GetWindowWidth()) / float(GetWindowHeight());
}

HWND GetNativeViewHandle()
{
    return hWnd;
}
