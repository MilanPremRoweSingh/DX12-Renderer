
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
    ShellInitialise(hInstance);

    EngineInitialise();

    while (WindowExists())
    {
        ProcessWindowMessages();
        EngineIdle();
    }
    
    EngineDispose();

    ShellDispose();

    return 0;
}
