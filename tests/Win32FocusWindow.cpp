#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstdlib>

namespace {

constexpr wchar_t WindowClassName[] = L"NotepadPlusPlusQt.FocusTest";

LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam,
                            LPARAM lParam)
{
    if (message == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

bool forceForegroundWindow(HWND window)
{
    if (!window)
        return false;

    const DWORD currentThread = GetCurrentThreadId();
    const DWORD targetThread = GetWindowThreadProcessId(window, nullptr);
    const HWND foregroundWindow = GetForegroundWindow();
    const DWORD foregroundThread = foregroundWindow
        ? GetWindowThreadProcessId(foregroundWindow, nullptr) : 0;
    const bool targetAttached = targetThread != 0
        && targetThread != currentThread
        && AttachThreadInput(currentThread, targetThread, TRUE);
    const bool foregroundAttached = foregroundThread != 0
        && foregroundThread != currentThread
        && foregroundThread != targetThread
        && AttachThreadInput(currentThread, foregroundThread, TRUE);

    ShowWindow(window, SW_SHOW);
    BringWindowToTop(window);
    SetForegroundWindow(window);
    SetActiveWindow(window);
    SetFocus(window);
    const bool activated = GetForegroundWindow() == window;

    if (foregroundAttached)
        AttachThreadInput(currentThread, foregroundThread, FALSE);
    if (targetAttached)
        AttachThreadInput(currentThread, targetThread, FALSE);
    return activated;
}

} // namespace

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, char* commandLine, int)
{
    WNDCLASSW windowClass{};
    windowClass.hInstance = instance;
    windowClass.lpfnWndProc = windowProc;
    windowClass.lpszClassName = WindowClassName;
    if (!RegisterClassW(&windowClass)
        && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return 1;
    }

    HWND window = CreateWindowExW(
        0, WindowClassName, L"Notepad++ Qt focus test",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 320, 180,
        nullptr, nullptr, instance, nullptr);
    if (!window)
        return 2;

    HWND parentWindow = reinterpret_cast<HWND>(
        static_cast<uintptr_t>(std::strtoull(commandLine, nullptr, 10)));
    if (!forceForegroundWindow(parentWindow))
        return 3;
    Sleep(250);
    if (!forceForegroundWindow(window))
        return 4;
    const DWORD endTime = GetTickCount() + 750;
    MSG message{};
    while (GetTickCount() < endTime) {
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        Sleep(10);
    }
    DestroyWindow(window);
    return 0;
}
