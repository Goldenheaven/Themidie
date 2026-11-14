#pragma once

#include "pluginsdk/bridgemain.h"
#include "pluginsdk/_plugins.h"

#ifdef THEMIDIE_X32DBG
#pragma comment(lib, "pluginsdk/x32dbg.lib")
#else
#pragma comment(lib, "pluginsdk/x64dbg.lib")
#endif

#define PLUG_EXPORT extern "C" __declspec(dllexport)

extern int pluginHandle, h_menu;
extern HWND__ *hwnd_dlg;
extern HINSTANCE__ *instance;

enum ThemidieMenus : int {
    MENU_START,
    MENU_ABOUT
};