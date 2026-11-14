#define WIN32_MEAN_AND_LEAN
#include <Windows.h>
#include <shellapi.h>
#include <string>
#include "themidie.hpp"

static INT_PTR CALLBACK arguments_dialog_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    std::string* arguments = (std::string*)GetWindowLongPtrA(window, DWLP_USER);

    if (message == WM_INITDIALOG)
    {
        SetWindowLongPtrA(window, DWLP_USER, lparam);
        SetFocus(GetDlgItem(window, IDC_ARGUMENTS));
        return 0;
    }

    if (message == WM_COMMAND && LOWORD(wparam) == IDOK)
    {
        char buffer[32768] = "";
        GetDlgItemTextA(window, IDC_ARGUMENTS, buffer, sizeof(buffer));
        *arguments = buffer;
        EndDialog(window, IDOK);
        return 1;
    }

    if (message == WM_COMMAND && LOWORD(wparam) == IDCANCEL)
    {
        EndDialog(window, IDCANCEL);
        return 1;
    }

    return 0;
}

static bool prompt_for_arguments(std::string& arguments)
{
    bool accepted = DialogBoxParamA(instance, MAKEINTRESOURCEA(IDD_ARGUMENTS), hwnd_dlg,
        arguments_dialog_proc, (LPARAM)&arguments) == IDOK;
    if (accepted && arguments.find_first_not_of(" \t\r\n") == std::string::npos)
        arguments.clear();
    return accepted;
}

static std::string command_line_arguments()
{
    int count = 0;
    LPWSTR* values = CommandLineToArgvW(GetCommandLineW(), &count);
    if (!values || count < 2)
    {
        if (values)
            LocalFree(values);
        return "";
    }

    std::string arguments;
    for (int index = 1; index < count; ++index)
    {
        int size = WideCharToMultiByte(CP_ACP, 0, values[index], -1, 0, 0, 0, 0);
        std::string value(size > 0 ? size - 1 : 0, '\0');
        if (size > 1)
            WideCharToMultiByte(CP_ACP, 0, values[index], size - 1, &value[0], size - 1, 0, 0);

        if (!arguments.empty())
            arguments += " ";
        if (value.find_first_of(" \t\"") != std::string::npos)
            arguments += "\"" + value + "\"";
        else
            arguments += value;
    }

    LocalFree(values);
    return arguments;
}

static std::string quote_command_line_argument(const std::string& value)
{
    return "\"" + value + "\"";
}


std::string 
convert_path(std::string& path)
{
    unsigned long long position{};

    while ((position = path.find("\\")) != std::string::npos) {
        path.erase(position, std::string("\\").size());
        path.insert(position, "/");
    }

    return path;
}

std::string 
remove_exe_name_from_path(std::string& path)
{
    unsigned long long pos = std::string::npos;
    std::string exe_name = "x64dbg.exe";

    while ((pos = path.find(exe_name)) != std::string::npos)
    {
        path.erase(pos, exe_name.length());
    }

    return path;
}

void 
start(std::string exe_path)
{
    _STARTUPINFOA si; memset(&si, 0, sizeof(si)); si.cb = sizeof(si);
    _PROCESS_INFORMATION pi; memset(&pi, 0, sizeof(pi));

    int next = 0;

    HINSTANCE__ *kernel_module = 0;
    void *load_library_function = 0, *themidie_dll_address = 0;
    void *remote_thread = 0;

    std::string arguments = command_line_arguments();
    if (arguments.find_first_not_of(" \t\r\n") == std::string::npos)
        arguments.clear();
    if (arguments.empty() && !prompt_for_arguments(arguments))
        return;

    std::string command_line = quote_command_line_argument(exe_path);
    if (!arguments.empty())
        command_line += " " + arguments;

    next = CreateProcessA(exe_path.c_str(), &command_line[0], 0, 0, 0, 4, 0, 0, &si, &pi);
    if (!next) { error("Could not create target process, please retry with admin rights."); }

    else
    {
        kernel_module = GetModuleHandleA("kernel32.dll");
        if (!kernel_module) { error("Could not find kernel32.dll module into target process. Is it loaded?"); }

        else
        {
            load_library_function = GetProcAddress(kernel_module, "LoadLibraryA");
            if (!load_library_function) { error("Could not find LoadLibraryA function into target process."); }

            else
            {
                char buffer[260]; GetModuleFileNameA(0, buffer, 260);
                std::string themidie_dll_path = convert_path(remove_exe_name_from_path(std::string(buffer)) + "plugins\\Themidie.dll");

                themidie_dll_address = VirtualAllocEx(pi.hProcess, 0, strlen(themidie_dll_path.c_str()), 8192 | 4096, 4);
                if (!themidie_dll_address) { error("Could not allocate memory into target process."); }

                else
                {
                    next = WriteProcessMemory(pi.hProcess, themidie_dll_address, themidie_dll_path.c_str(), strlen(themidie_dll_path.c_str()), 0);
                    if (!next) { error("Could not write Themidie into target process."); }

                    else
                    {
                        remote_thread = CreateRemoteThread(pi.hProcess, 0, 0, (PTHREAD_START_ROUTINE)load_library_function, themidie_dll_address, 0, 0);
                        if (!remote_thread) { error("Could not load Themidie into target process."); }

                        else
                        {
                            next = ShowWindow(GetForegroundWindow(), 6);
                            if (!next) { error("Could not minimize x64dbg window. Is the ShowWindow function hooked by another plugin?"); }

                            else
                            {
                                CloseHandle(pi.hProcess);
                                ResumeThread(pi.hThread);
                            }
                        }
                    }
                }
            }
        }
    }
}

void 
open_and_start_exe() 
{
    tagOFNA ofn;
    char file_name[260] = "";
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(tagOFNA);
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = "Executable Files (*.exe*)\0*.exe*\0";
    ofn.lpstrFile = file_name;
    ofn.nMaxFile = 260;
    ofn.Flags = 524288 | 4096 | 4;
    ofn.lpstrDefExt = "";
    GetOpenFileNameA(&ofn);
    std::string file_name_string = file_name;
    start(convert_path(file_name_string));
}


PLUG_EXPORT void 
CBMENUENTRY(CBTYPE cb_type, PLUG_CB_MENUENTRY* info)
{
    switch (info->hEntry)
    {

        case MENU_START:
            open_and_start_exe();
            break;

        case MENU_ABOUT:
            tagMSGBOXPARAMSA mpar;
            char hdr[64];
            char about[128];
            wsprintfA(hdr, "Themidie");
            wsprintfA(about, "Author: VenTaz\nDiscord: VenTaz#8766\nhttps://hactify.ru");
            memset(&mpar, 0, sizeof(mpar));
            mpar.cbSize = sizeof(mpar);
            mpar.hInstance = instance;
            mpar.hwndOwner = hwnd_dlg;
            mpar.dwStyle = 0L | 128L;
            mpar.dwLanguageId = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
            mpar.lpszIcon = MAKEINTRESOURCEA(IDI_ICON1);
            mpar.lpszText = about;
            mpar.lpszCaption = hdr;
            MessageBoxIndirectA(&mpar);
            break;

        default:
            break;

    }
}


void 
plugin_setup(PLUG_SETUPSTRUCT *setup_struct)
{
    h_menu = setup_struct->hMenu;

    _plugin_menuaddentry(h_menu, MENU_START, "&Start");
    _plugin_menuaddseparator(h_menu);
	_plugin_menuaddentry(h_menu, MENU_ABOUT, "&About");
}

