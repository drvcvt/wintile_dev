#include <windows.h>
#include <map>
#include <vector>

// Hotkey IDs
#define HOTKEY_ID_H 1
#define HOTKEY_ID_J 2
#define HOTKEY_ID_K 3
#define HOTKEY_ID_L 4

enum class SnapDirection {
    Left,
    Right,
    Up,
    Down
};

enum class WindowState {
    Unknown,
    LeftHalf,
    RightHalf,
    TopHalf,
    BottomHalf,
    TopLeftQuarter,
    TopRightQuarter,
    BottomLeftQuarter,
    BottomRightQuarter,
    Maximized
};

std::map<HWND, WindowState> windowStates;

// Maximized state for full-screen toggle
std::map<HWND, RECT> originalPositions;

void SnapWindow(HWND hwnd, WindowState newState, HMONITOR monitor);

void MaximizeWindow(HWND hwnd) {
    if (originalPositions.find(hwnd) == originalPositions.end()) {
        RECT rc;
        GetWindowRect(hwnd, &rc);
        originalPositions[hwnd] = rc;

        HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = { sizeof(mi) };
        GetMonitorInfo(monitor, &mi);
        SetWindowPos(hwnd, NULL, mi.rcWork.left, mi.rcWork.top, mi.rcWork.right - mi.rcWork.left, mi.rcWork.bottom - mi.rcWork.top, SWP_NOZORDER | SWP_NOACTIVATE);
        windowStates[hwnd] = WindowState::Maximized;
    } else {
        RECT rc = originalPositions[hwnd];
        SetWindowPos(hwnd, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER | SWP_NOACTIVATE);
        originalPositions.erase(hwnd);
        windowStates.erase(hwnd);
    }
}

void MoveWindowToMonitor(HWND hwnd, HMONITOR monitor) {
    if (!hwnd || !monitor) return;

    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfo(monitor, &mi);

    WindowState currentState = windowStates.count(hwnd) ? windowStates[hwnd] : WindowState::Unknown;
    if (currentState != WindowState::Unknown) {
        SnapWindow(hwnd, currentState, monitor);
    } else {
        RECT rc;
        GetWindowRect(hwnd, &rc);
        long width = rc.right - rc.left;
        long height = rc.bottom - rc.top;
        long newX = mi.rcWork.left + (mi.rcWork.right - mi.rcWork.left - width) / 2;
        long newY = mi.rcWork.top + (mi.rcWork.bottom - mi.rcWork.top - height) / 2;
        SetWindowPos(hwnd, NULL, newX, newY, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
    }

    // Move cursor to the center of the window
    RECT rc;
    GetWindowRect(hwnd, &rc);
    SetCursorPos(rc.left + (rc.right - rc.left) / 2, rc.top + (rc.bottom - rc.top) / 2);
}


void SnapWindow(HWND hwnd, WindowState newState, HMONITOR monitor) {
    if (hwnd == NULL) return;

    if (monitor == NULL) {
        monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    }
    MONITORINFO monitorInfo = { sizeof(monitorInfo) };
    if (!GetMonitorInfo(monitor, &monitorInfo)) return;

    long w = monitorInfo.rcWork.right - monitorInfo.rcWork.left;
    long h = monitorInfo.rcWork.bottom - monitorInfo.rcWork.top;
    long x = monitorInfo.rcWork.left;
    long y = monitorInfo.rcWork.top;

    long newX, newY, newWidth, newHeight;

    switch (newState) {
        case WindowState::LeftHalf:
            newX = x; newY = y; newWidth = w / 2; newHeight = h;
            break;
        case WindowState::RightHalf:
            newX = x + w / 2; newY = y; newWidth = w / 2; newHeight = h;
            break;
        case WindowState::TopHalf:
            newX = x; newY = y; newWidth = w; newHeight = h / 2;
            break;
        case WindowState::BottomHalf:
            newX = x; newY = y + h / 2; newWidth = w; newHeight = h / 2;
            break;
        case WindowState::TopLeftQuarter:
            newX = x; newY = y; newWidth = w / 2; newHeight = h / 2;
            break;
        case WindowState::TopRightQuarter:
            newX = x + w / 2; newY = y; newWidth = w / 2; newHeight = h / 2;
            break;
        case WindowState::BottomLeftQuarter:
            newX = x; newY = y + h / 2; newWidth = w / 2; newHeight = h / 2;
            break;
        case WindowState::BottomRightQuarter:
            newX = x + w / 2; newY = y + h / 2; newWidth = w / 2; newHeight = h / 2;
            break;
        default:
            return; 
    }

    SetWindowPos(hwnd, NULL, newX, newY, newWidth, newHeight, SWP_NOZORDER | SWP_NOACTIVATE);
    windowStates[hwnd] = newState;
    originalPositions.erase(hwnd); // Remove maximized state history

    // Move cursor to the center of the window
    RECT rc;
    GetWindowRect(hwnd, &rc);
    SetCursorPos(rc.left + (rc.right - rc.left) / 2, rc.top + (rc.bottom - rc.top) / 2);
}

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    auto* monitors = reinterpret_cast<std::vector<MONITORINFO>*>(dwData);
    MONITORINFO mi = { sizeof(mi) };
    if (GetMonitorInfo(hMonitor, &mi)) {
        monitors->push_back(mi);
    }
    return TRUE;
}

HMONITOR FindNextMonitor(HMONITOR current, SnapDirection direction) {
    std::vector<MONITORINFO> monitors;
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, reinterpret_cast<LPARAM>(&monitors));
    if (monitors.size() <= 1) return current;

    MONITORINFO currentMi = { sizeof(currentMi) };
    GetMonitorInfo(current, &currentMi);

    HMONITOR bestMatch = NULL;
    long bestDist = LONG_MAX;

    for (const auto& mi : monitors) {
        HMONITOR hOther = MonitorFromRect(&mi.rcWork, MONITOR_DEFAULTTONEAREST);
        if (hOther == current) continue;

        long dist = 0;
        bool isCandidate = false;

        switch (direction) {
            case SnapDirection::Left:
                if (mi.rcWork.right <= currentMi.rcWork.left) {
                    dist = currentMi.rcWork.left - mi.rcWork.right;
                    isCandidate = true;
                }
                break;
            case SnapDirection::Right:
                if (mi.rcWork.left >= currentMi.rcWork.right) {
                    dist = mi.rcWork.left - currentMi.rcWork.right;
                    isCandidate = true;
                }
                break;
            case SnapDirection::Up:
                 if (mi.rcWork.bottom <= currentMi.rcWork.top) {
                    dist = currentMi.rcWork.top - mi.rcWork.bottom;
                    isCandidate = true;
                }
                break;
            case SnapDirection::Down:
                if (mi.rcWork.top >= currentMi.rcWork.bottom) {
                    dist = mi.rcWork.top - currentMi.rcWork.bottom;
                    isCandidate = true;
                }
                break;
        }

        if (isCandidate && dist < bestDist) {
            bestDist = dist;
            bestMatch = hOther;
        }
    }
    return (bestMatch != NULL) ? bestMatch : current;
}


void HandleMonitorSwitch(SnapDirection direction) {
    POINT p;
    if (!GetCursorPos(&p)) {
        return;
    }
    HWND hwnd = WindowFromPoint(p);
    if (!hwnd) {
        return;
    }
    hwnd = GetAncestor(hwnd, GA_ROOT);
    if (!hwnd) {
        return;
    }

    wchar_t class_name[256];
    GetClassNameW(hwnd, class_name, sizeof(class_name) / sizeof(wchar_t));
    if (lstrcmpW(class_name, L"Progman") == 0 || lstrcmpW(class_name, L"WorkerW") == 0 || lstrcmpW(class_name, L"Shell_TrayWnd") == 0) {
        return;
    }

    HMONITOR currentMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    HMONITOR nextMonitor = FindNextMonitor(currentMonitor, direction);

    if (nextMonitor && nextMonitor != currentMonitor) {
        MoveWindowToMonitor(hwnd, nextMonitor);
    }
}

void HandleSnapRequest(SnapDirection direction) {
    POINT p;
    if (!GetCursorPos(&p)) {
        return;
    }
    HWND hwnd = WindowFromPoint(p);
    if (!hwnd) {
        return;
    }

    // Get the top-level parent window
    hwnd = GetAncestor(hwnd, GA_ROOT);
    if (!hwnd) {
        return;
    }

    // Don't tile desktop or taskbar
    wchar_t class_name[256];
    GetClassNameW(hwnd, class_name, sizeof(class_name) / sizeof(wchar_t));
    if (lstrcmpW(class_name, L"Progman") == 0 || lstrcmpW(class_name, L"WorkerW") == 0 || lstrcmpW(class_name, L"Shell_TrayWnd") == 0) {
        return;
    }
    
    WindowState currentState = windowStates.count(hwnd) ? windowStates[hwnd] : WindowState::Unknown;
    
    HMONITOR currentMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    bool isAtLeftEdge = currentState == WindowState::LeftHalf || currentState == WindowState::TopLeftQuarter || currentState == WindowState::BottomLeftQuarter;
    bool isAtRightEdge = currentState == WindowState::RightHalf || currentState == WindowState::TopRightQuarter || currentState == WindowState::BottomRightQuarter;
    bool isAtTopEdge = currentState == WindowState::TopHalf || currentState == WindowState::TopLeftQuarter || currentState == WindowState::TopRightQuarter;
    bool isAtBottomEdge = currentState == WindowState::BottomHalf || currentState == WindowState::BottomLeftQuarter || currentState == WindowState::BottomRightQuarter;
    bool isAtTopLeftQuarter = currentState == WindowState::TopLeftQuarter;
    bool isAtTopRightQuarter = currentState == WindowState::TopRightQuarter;
    bool isAtBottomLeftQuarter = currentState == WindowState::BottomLeftQuarter;
    bool isAtBottomRightQuarter = currentState == WindowState::BottomRightQuarter;

    if ((direction == SnapDirection::Left && isAtLeftEdge) ||
        (direction == SnapDirection::Right && isAtRightEdge) ||
        (direction == SnapDirection::Up && isAtTopEdge) ||
        (direction == SnapDirection::Down && isAtBottomEdge)) {
        
        HMONITOR nextMonitor = FindNextMonitor(currentMonitor, direction);
        if (nextMonitor != currentMonitor) {
            WindowState targetState = currentState;
            // When moving to a new monitor, snap to the opposite side
            if (direction == SnapDirection::Left) targetState = WindowState::RightHalf;
            if (direction == SnapDirection::Right) targetState = WindowState::LeftHalf;
            if (direction == SnapDirection::Up) targetState = WindowState::BottomHalf;
            if (direction == SnapDirection::Down) targetState = WindowState::TopHalf;
            
            // Adjust for corners
            if (isAtTopLeftQuarter && direction == SnapDirection::Left) targetState = WindowState::TopRightQuarter;
            if (isAtTopLeftQuarter && direction == SnapDirection::Up) targetState = WindowState::BottomLeftQuarter;
            if (isAtTopRightQuarter && direction == SnapDirection::Right) targetState = WindowState::TopLeftQuarter;
            if (isAtTopRightQuarter && direction == SnapDirection::Up) targetState = WindowState::BottomRightQuarter;
            if (isAtBottomLeftQuarter && direction == SnapDirection::Left) targetState = WindowState::BottomRightQuarter;
            if (isAtBottomLeftQuarter && direction == SnapDirection::Down) targetState = WindowState::TopLeftQuarter;
            if (isAtBottomRightQuarter && direction == SnapDirection::Right) targetState = WindowState::BottomLeftQuarter;
            if (isAtBottomRightQuarter && direction == SnapDirection::Down) targetState = WindowState::TopRightQuarter;

            SnapWindow(hwnd, targetState, nextMonitor);
            return;
        }
    }
    
    WindowState newState = WindowState::Unknown;

    switch (direction) {
        case SnapDirection::Left:
            if (currentState == WindowState::RightHalf || currentState == WindowState::TopRightQuarter || currentState == WindowState::BottomRightQuarter)
                newState = WindowState::LeftHalf;
            else if (currentState == WindowState::TopHalf)
                newState = WindowState::TopLeftQuarter;
            else if (currentState == WindowState::BottomHalf)
                newState = WindowState::BottomLeftQuarter;
            else if (currentState == WindowState::LeftHalf)
                MaximizeWindow(hwnd);
            else
                newState = WindowState::LeftHalf;
            break;
        case SnapDirection::Right:
            if (currentState == WindowState::LeftHalf || currentState == WindowState::TopLeftQuarter || currentState == WindowState::BottomLeftQuarter)
                newState = WindowState::RightHalf;
            else if (currentState == WindowState::TopHalf)
                newState = WindowState::TopRightQuarter;
            else if (currentState == WindowState::BottomHalf)
                newState = WindowState::BottomRightQuarter;
            else if (currentState == WindowState::RightHalf)
                MaximizeWindow(hwnd);
            else
                newState = WindowState::RightHalf;
            break;
        case SnapDirection::Up:
            if (currentState == WindowState::BottomHalf || currentState == WindowState::BottomLeftQuarter || currentState == WindowState::BottomRightQuarter)
                 newState = WindowState::TopHalf;
            else if (currentState == WindowState::LeftHalf)
                newState = WindowState::TopLeftQuarter;
            else if (currentState == WindowState::RightHalf)
                newState = WindowState::TopRightQuarter;
            else if (currentState == WindowState::TopHalf)
                MaximizeWindow(hwnd);
            else
                newState = WindowState::TopHalf;
            break;
        case SnapDirection::Down:
            if (currentState == WindowState::TopHalf || currentState == WindowState::TopLeftQuarter || currentState == WindowState::TopRightQuarter)
                newState = WindowState::BottomHalf;
            else if (currentState == WindowState::LeftHalf)
                newState = WindowState::BottomLeftQuarter;
            else if (currentState == WindowState::RightHalf)
                newState = WindowState::BottomRightQuarter;
            else if (currentState == WindowState::BottomHalf)
                MaximizeWindow(hwnd);
            else
                newState = WindowState::BottomHalf;
            break;
    }

    if (newState != WindowState::Unknown) {
        SnapWindow(hwnd, newState, NULL);
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_HOTKEY:
            switch (wParam) {
                case HOTKEY_ID_H: HandleSnapRequest(SnapDirection::Left); break;
                case HOTKEY_ID_L: HandleSnapRequest(SnapDirection::Right); break;
                case HOTKEY_ID_K: HandleSnapRequest(SnapDirection::Up); break;
                case HOTKEY_ID_J: HandleSnapRequest(SnapDirection::Down); break;
            }
            break;
        case WM_DESTROY:
            UnregisterHotKey(hwnd, HOTKEY_ID_H);
            UnregisterHotKey(hwnd, HOTKEY_ID_J);
            UnregisterHotKey(hwnd, HOTKEY_ID_K);
            UnregisterHotKey(hwnd, HOTKEY_ID_L);
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const WCHAR CLASS_NAME[] = L"WinVimTilerWindowClass";

    WNDCLASSW wc = { };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    if (!RegisterClassW(&wc)) {
        MessageBoxW(NULL, L"Window Registration Failed!", L"Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    HWND hwnd = CreateWindowExW(
        0, CLASS_NAME, L"WinVimTiler", 0, 0, 0, 0, 0,
        HWND_MESSAGE, NULL, hInstance, NULL
    );

    if (hwnd == NULL) {
        MessageBoxW(NULL, L"Window Creation Failed!", L"Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    if (!RegisterHotKey(hwnd, HOTKEY_ID_H, MOD_ALT | MOD_CONTROL, 'H')) {
        MessageBoxW(NULL, L"Failed to register hotkey H!", L"Error", MB_ICONEXCLAMATION | MB_OK);
    }
    if (!RegisterHotKey(hwnd, HOTKEY_ID_J, MOD_ALT | MOD_CONTROL, 'J')) {
        MessageBoxW(NULL, L"Failed to register hotkey J!", L"Error", MB_ICONEXCLAMATION | MB_OK);
    }
    if (!RegisterHotKey(hwnd, HOTKEY_ID_K, MOD_ALT | MOD_CONTROL, 'K')) {
        MessageBoxW(NULL, L"Failed to register hotkey K!", L"Error", MB_ICONEXCLAMATION | MB_OK);
    }
    if (!RegisterHotKey(hwnd, HOTKEY_ID_L, MOD_ALT | MOD_CONTROL, 'L')) {
        MessageBoxW(NULL, L"Failed to register hotkey L!", L"Error", MB_ICONEXCLAMATION | MB_OK);
    }
    
    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
} 