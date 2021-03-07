
#include <windows.h>
#include <iostream>
#include "string.h"

#include "Shell.h"
#include "Engine.h"

int WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nShowCmd
)
{
    ShellInitialise(hInstance);

    EngineInitialise();

    while (WindowExists())
    {
        ProcessWindowMessages();
    }
    
    EngineDispose();

    ShellDispose();

    return 0;
}
