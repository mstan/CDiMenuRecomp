/* SPDX-License-Identifier: MIT */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <wchar.h>

static wchar_t root[32768];
static wchar_t rom[32768];
static wchar_t runtime[32768];
static wchar_t command_line[32768];

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous, PWSTR args, int show) {
    (void)instance;
    (void)previous;
    (void)args;
    (void)show;
    const DWORD count = GetModuleFileNameW(NULL, root, ARRAYSIZE(root));
    if (!count || count == ARRAYSIZE(root)) {
        return 1;
    }
    wchar_t *separator = wcsrchr(root, L'\\');
    if (!separator) {
        return 1;
    }
    *separator = L'\0';

    OPENFILENAMEW picker = {0};
    picker.lStructSize = sizeof(picker);
    picker.lpstrTitle = L"Select the original CD-i player ROM";
    picker.lpstrFilter = L"CD-i player ROM (*.rom;*.bin)\0*.rom;*.bin\0";
    picker.lpstrFile = rom;
    picker.nMaxFile = ARRAYSIZE(rom);
    picker.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetOpenFileNameW(&picker)) {
        return CommDlgExtendedError() ? 1 : 0;
    }
    const int runtime_length = swprintf(runtime, ARRAYSIZE(runtime), L"%ls\\runtime\\CdiMenuRuntime.exe", root);
    if (runtime_length < 0 || runtime_length >= ARRAYSIZE(runtime)) {
        return 1;
    }
    const int command_length = swprintf(command_line, ARRAYSIZE(command_line), L"\"%ls\" \"%ls\"", runtime, rom);
    if (command_length < 0 || command_length >= ARRAYSIZE(command_line)) {
        return 1;
    }
    STARTUPINFOW startup = {0};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process = {0};
    if (!CreateProcessW(runtime, command_line, NULL, NULL, FALSE, CREATE_NO_WINDOW,
                        NULL, root, &startup, &process)) {
        MessageBoxW(NULL, L"Could not open the CD-i runtime. Extract the complete ZIP.",
                    L"CD-i Menu", MB_ICONERROR);
        return 1;
    }
    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD exit_code = 1;
    GetExitCodeProcess(process.hProcess, &exit_code);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    if (exit_code) {
        MessageBoxW(NULL, L"The CD-i runtime rejected the ROM or failed to start. Use the ROM identified in build.json.",
                    L"CD-i Menu", MB_ICONERROR);
    }
    return (int)exit_code;
}
