
#include <windows.h>
#include <iostream>
#include "string.h"
#include "winbase.h"
#include "Shell.h"
#include "Engine.h"

int WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd
)
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetBreakAlloc(9554);
    _CrtSetBreakAlloc(9553);
    _CrtSetBreakAlloc(9552);

    ShellInitialise(hInstance);

    EngineInitialise();

    while (WindowExists())
    {
        ProcessWindowMessages();
        EngineIdle();
    }
    
    EngineDispose();

    ShellDispose(hInstance);

    return 0;
}
