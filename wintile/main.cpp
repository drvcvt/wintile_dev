#include <windows.h>
#include <map>
#include <vector>
#include <set>
#include <dwmapi.h>  // Add DWM API support

// Add includes for unordered_map and hooks
#include <unordered_map>
#include <unordered_set>
#include <windowsx.h> // For GET_X_LPARAM etc if needed

// Hotkey IDs
#define HOTKEY_ID_H 1
#define HOTKEY_ID_J 2
#define HOTKEY_ID_K 3
#define HOTKEY_ID_L 4

#define HOTKEY_ID_SHIFT_H 5
#define HOTKEY_ID_SHIFT_J 6
#define HOTKEY_ID_SHIFT_K 7
#define HOTKEY_ID_SHIFT_L 8

// Border and padding configuration
#define BORDER_WIDTH 2
#define PADDING 6
#define FOCUSED_BORDER_COLOR RGB(100, 149, 237)  // Blue-gray for focused window
#define TRANSPARENT_COLOR RGB(255, 0, 255)  // Magenta for color-key transparency
#define BORDER_UPDATE_TIMER 1
#define BORDER_UPDATE_INTERVAL 20  // Update every 50ms for good balance of performance and responsiveness

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

// Change to unordered_map
std::unordered_map<HWND, WindowState> windowStates;

// Maximized state for full-screen toggle
std::unordered_map<HWND, RECT> originalPositions;

// Border windows management
std::unordered_map<HWND, HWND> windowBorders;  // Maps application window to its border window

// Focus and visibility tracking
HWND currentFocusedWindow = NULL;

// Add global hook variables
static HWINEVENTHOOK hEventHook = NULL;

// Add cache for monitors
std::unordered_map<HMONITOR, MONITORINFO> monitorCache;

// Forward declarations
void SnapWindow(HWND hwnd, WindowState newState, HMONITOR monitor);
void CreateOrUpdateBorder(HWND appWindow);
void RemoveBorder(HWND appWindow);
LRESULT CALLBACK BorderWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool IsWindowFullscreen(HWND hwnd);
void UpdateBorderVisibility(HWND appWindow);
bool ShouldWindowHaveBorder(HWND hwnd);
void UpdateFocusedWindow();
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam);
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);

// Add hook procedure
void CALLBACK WinEventProc(HWINEVENTHOOK hook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) {
    if (hwnd && ShouldWindowHaveBorder(hwnd)) {
        switch (event) {
            case EVENT_SYSTEM_MOVESIZEEND:
            case EVENT_SYSTEM_MINIMIZEEND:
            case EVENT_OBJECT_SHOW:
                if (hwnd == currentFocusedWindow) {
                    CreateOrUpdateBorder(hwnd);
                }
                break;
            case EVENT_SYSTEM_FOREGROUND:
                UpdateFocusedWindow();
                break;
            case EVENT_OBJECT_DESTROY:
            case EVENT_OBJECT_HIDE:
                RemoveBorder(hwnd);
                break;
        }
    }
}

// Modify GetActualWindowRect to use DWM first and cache if possible
bool GetActualWindowRect(HWND hwnd, RECT* rect) {
    HRESULT hr = DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, rect, sizeof(RECT));
    if (SUCCEEDED(hr)) {
        return true;
    }
    return GetWindowRect(hwnd, rect) != 0;
}

// Check if window is in fullscreen mode
bool IsWindowFullscreen(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd)) return false;
    
    // Get window rect
    RECT windowRect;
    if (!GetWindowRect(hwnd, &windowRect)) return false;
    
    // Get monitor info for the window
    HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(mi) };
    if (!GetMonitorInfo(monitor, &mi)) return false;
    
    // Check if window covers the entire monitor (including taskbar area)
    return (windowRect.left <= mi.rcMonitor.left &&
            windowRect.top <= mi.rcMonitor.top &&
            windowRect.right >= mi.rcMonitor.right &&
            windowRect.bottom >= mi.rcMonitor.bottom);
}

// Check if window should have a border
bool ShouldWindowHaveBorder(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd) || !IsWindowVisible(hwnd)) {
        return false;
    }
    
    // Don't show borders for fullscreen windows
    if (IsWindowFullscreen(hwnd)) {
        return false;
    }
    
    // Don't show borders for minimized windows
    if (IsIconic(hwnd)) {
        return false;
    }
    
    // Don't show borders for desktop, taskbar, etc.
    wchar_t className[256];
    if (GetClassNameW(hwnd, className, sizeof(className) / sizeof(wchar_t))) {
        if (lstrcmpW(className, L"Progman") == 0 || 
            lstrcmpW(className, L"WorkerW") == 0 || 
            lstrcmpW(className, L"Shell_TrayWnd") == 0 ||
            lstrcmpW(className, L"DV2ControlHost") == 0 ||
            lstrcmpW(className, L"MsgrIMEWindowClass") == 0 ||
            lstrcmpW(className, L"SysShadow") == 0 ||
            lstrcmpW(className, L"SnapAssistFlyout") == 0 ||
            lstrcmpW(className, L"SearchUI") == 0 ||
            lstrcmpW(className, L"Shell_Flyout") == 0) {
            return false;
        }
    }
    
    // Don't show borders for very small windows (likely system windows)
    RECT rect;
    if (GetWindowRect(hwnd, &rect)) {
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;
        if (width < 100 || height < 50) {
            return false;
        }
    }
    
    // Check window style - only show for normal application windows
    LONG style = GetWindowLong(hwnd, GWL_STYLE);
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    
    // Must have a caption or be a popup with visible border
    if (!(style & WS_CAPTION) && !(style & WS_POPUP)) {
        return false;
    }
    
    // Skip tool windows and other special windows
    if (exStyle & WS_EX_TOOLWINDOW) {
        return false;
    }
    
    return true;
}

// Update border visibility based on window state
void UpdateBorderVisibility(HWND appWindow) {
    if (appWindow == currentFocusedWindow && ShouldWindowHaveBorder(appWindow)) {
        CreateOrUpdateBorder(appWindow);
    } else {
        RemoveBorder(appWindow);
    }
}

// Update focused window
void UpdateFocusedWindow() {
    HWND newFocusedWindow = GetForegroundWindow();
    if (newFocusedWindow != currentFocusedWindow) {
        HWND oldFocusedWindow = currentFocusedWindow;
        currentFocusedWindow = newFocusedWindow;

        // Remove border from the old focused window
        if (oldFocusedWindow) {
            RemoveBorder(oldFocusedWindow);
        }
        // Create or update border for the new focused window
        if (currentFocusedWindow && ShouldWindowHaveBorder(currentFocusedWindow)) {
            CreateOrUpdateBorder(currentFocusedWindow);
        }
    }
}

// Enumerate windows callback
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    // This function is no longer strictly necessary with the new border logic,
    // but can be kept for future debugging or functionality.
    // For now, it does nothing.
    return TRUE;
}

// Border window class registration
bool RegisterBorderWindowClass(HINSTANCE hInstance) {
    const WCHAR BORDER_CLASS_NAME[] = L"WinTilerBorderClass";
    
    WNDCLASSW wc = { };
    wc.lpfnWndProc = BorderWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = BORDER_CLASS_NAME;
    wc.hbrBackground = NULL;  // No automatic background drawing
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.style = CS_HREDRAW | CS_VREDRAW;  // Redraw on resize
    
    return RegisterClassW(&wc) != 0;
}

// Border window procedure
LRESULT CALLBACK BorderWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            RECT rect;
            GetClientRect(hwnd, &rect);
            
            // First, fill entire background with transparent color
            HBRUSH transparentBrush = CreateSolidBrush(TRANSPARENT_COLOR);
            FillRect(hdc, &rect, transparentBrush);
            DeleteObject(transparentBrush);
            
            // Border is always for the focused window
            COLORREF borderColor = FOCUSED_BORDER_COLOR;
            
            // Create brush for border color
            HBRUSH borderBrush = CreateSolidBrush(borderColor);
            
            // Draw border (2px thick)
            RECT topBorder = {0, 0, rect.right, BORDER_WIDTH};
            FillRect(hdc, &topBorder, borderBrush);
            
            RECT bottomBorder = {0, rect.bottom - BORDER_WIDTH, rect.right, rect.bottom};
            FillRect(hdc, &bottomBorder, borderBrush);
            
            RECT leftBorder = {0, 0, BORDER_WIDTH, rect.bottom};
            FillRect(hdc, &leftBorder, borderBrush);
            
            RECT rightBorder = {rect.right - BORDER_WIDTH, 0, rect.right, rect.bottom};
            FillRect(hdc, &rightBorder, borderBrush);
            
            DeleteObject(borderBrush);
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_NCHITTEST:
            return HTTRANSPARENT;
        case WM_ERASEBKGND:
            return 1;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

void CreateOrUpdateBorder(HWND appWindow) {
    if (!appWindow || appWindow != currentFocusedWindow) return;
    
    RECT appRect;
    if (!GetActualWindowRect(appWindow, &appRect)) return;
    
    // Calculate border window position (extending around the app window)
    RECT borderRect;
    borderRect.left = appRect.left - BORDER_WIDTH;
    borderRect.top = appRect.top - BORDER_WIDTH;
    borderRect.right = appRect.right + BORDER_WIDTH;
    borderRect.bottom = appRect.bottom + BORDER_WIDTH;
    
    HWND borderWindow = NULL;
    if (windowBorders.count(appWindow)) {
        // Update existing border window - position it right behind the app window
        borderWindow = windowBorders[appWindow];
        SetWindowPos(borderWindow, appWindow, 
                    borderRect.left, borderRect.top,
                    borderRect.right - borderRect.left,
                    borderRect.bottom - borderRect.top,
                    SWP_NOACTIVATE | SWP_SHOWWINDOW);
        
        // Trigger repaint to update border color
        InvalidateRect(borderWindow, NULL, TRUE);
    } else {
        // Create new border window without TOPMOST flag
        borderWindow = CreateWindowExW(
            WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
            L"WinTilerBorderClass",
            L"",
            WS_POPUP,
            borderRect.left, borderRect.top,
            borderRect.right - borderRect.left,
            borderRect.bottom - borderRect.top,
            NULL, NULL, GetModuleHandle(NULL), NULL
        );
        
        if (borderWindow) {
            // Use pure color-key transparency to ensure no darkening occurs
            SetLayeredWindowAttributes(borderWindow, TRANSPARENT_COLOR, 255, LWA_COLORKEY);
            
            // Position border window right behind the app window in Z-order
            SetWindowPos(borderWindow, appWindow, 0, 0, 0, 0, 
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            
            windowBorders[appWindow] = borderWindow;
            ShowWindow(borderWindow, SW_SHOWNOACTIVATE);
        }
    }
}

void RemoveBorder(HWND appWindow) {
    if (windowBorders.count(appWindow)) {
        HWND borderWindow = windowBorders[appWindow];
        DestroyWindow(borderWindow);
        windowBorders.erase(appWindow);
    }
}

// Modify UpdateAllBorders to be called less frequently or on demand
void UpdateAllBorders() {
    UpdateFocusedWindow();
}

// Add function to refresh monitor cache
void RefreshMonitorCache() {
    monitorCache.clear();
    std::vector<MONITORINFO> monitors;
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, reinterpret_cast<LPARAM>(&monitors));
    for (const auto& mi : monitors) {
        HMONITOR hmon = MonitorFromRect(&mi.rcMonitor, MONITOR_DEFAULTTONEAREST);
        monitorCache[hmon] = mi;
    }
}

void MaximizeWindow(HWND hwnd) {
    if (originalPositions.find(hwnd) == originalPositions.end()) {
        RECT rc;
        GetWindowRect(hwnd, &rc);
        originalPositions[hwnd] = rc;

        SnapWindow(hwnd, WindowState::Maximized, NULL);
    } else {
        RECT rc = originalPositions[hwnd];
        SetWindowPos(hwnd, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER | SWP_NOACTIVATE);
        originalPositions.erase(hwnd);
        windowStates.erase(hwnd);
        
        // Update border visibility
        UpdateBorderVisibility(hwnd);
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
        
        // Update border visibility
        UpdateBorderVisibility(hwnd);
    }

    // Move cursor to the center of the window
    RECT rc;
    GetWindowRect(hwnd, &rc);
    SetCursorPos(rc.left + (rc.right - rc.left) / 2, rc.top + (rc.bottom - rc.top) / 2);
}


void SnapWindow(HWND hwnd, WindowState newState, HMONITOR monitor) {
    if (hwnd == NULL) return;

    // Before snapping, remove the old border to prevent artifacts
    RemoveBorder(hwnd);

    if (monitor == NULL) {
        monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    }
    
    auto it = monitorCache.find(monitor);
    MONITORINFO monitorInfo = (it != monitorCache.end()) ? it->second : MONITORINFO{ sizeof(MONITORINFO) };
    if (it == monitorCache.end() && !GetMonitorInfo(monitor, &monitorInfo)) return;
    monitorCache[monitor] = monitorInfo; // Cache it

    long w = monitorInfo.rcWork.right - monitorInfo.rcWork.left;
    long h = monitorInfo.rcWork.bottom - monitorInfo.rcWork.top;
    long x = monitorInfo.rcWork.left;
    long y = monitorInfo.rcWork.top;

    long newX, newY, newWidth, newHeight;

    switch (newState) {
        case WindowState::LeftHalf:
            newX = x + PADDING;
            newY = y + PADDING;
            newWidth = (w - 3 * PADDING) / 2;
            newHeight = h - 2 * PADDING;
            break;
        case WindowState::RightHalf:
            newX = x + w / 2 + PADDING / 2;
            newY = y + PADDING;
            newWidth = (w - 3 * PADDING) / 2;
            newHeight = h - 2 * PADDING;
            break;
        case WindowState::TopHalf:
            newX = x + PADDING;
            newY = y + PADDING;
            newWidth = w - 2 * PADDING;
            newHeight = (h - 3 * PADDING) / 2;
            break;
        case WindowState::BottomHalf:
            newX = x + PADDING;
            newY = y + h / 2 + PADDING / 2;
            newWidth = w - 2 * PADDING;
            newHeight = (h - 3 * PADDING) / 2;
            break;
        case WindowState::TopLeftQuarter:
            newX = x + PADDING;
            newY = y + PADDING;
            newWidth = (w - 3 * PADDING) / 2;
            newHeight = (h - 3 * PADDING) / 2;
            break;
        case WindowState::TopRightQuarter:
            newX = x + w / 2 + PADDING / 2;
            newY = y + PADDING;
            newWidth = (w - 3 * PADDING) / 2;
            newHeight = (h - 3 * PADDING) / 2;
            break;
        case WindowState::BottomLeftQuarter:
            newX = x + PADDING;
            newY = y + h / 2 + PADDING / 2;
            newWidth = (w - 3 * PADDING) / 2;
            newHeight = (h - 3 * PADDING) / 2;
            break;
        case WindowState::BottomRightQuarter:
            newX = x + w / 2 + PADDING / 2;
            newY = y + h / 2 + PADDING / 2;
            newWidth = (w - 3 * PADDING) / 2;
            newHeight = (h - 3 * PADDING) / 2;
            break;
        case WindowState::Maximized:
            newX = x + PADDING;
            newY = y + PADDING;
            newWidth = w - 2 * PADDING;
            newHeight = h - 2 * PADDING;
            break;
        default:
            return;
    }

    SetWindowPos(hwnd, NULL, newX, newY, newWidth, newHeight, SWP_NOZORDER | SWP_NOACTIVATE);
    windowStates[hwnd] = newState;
    originalPositions.erase(hwnd); // Remove maximized state history

    // After snapping, create the new border at the correct position
    CreateOrUpdateBorder(hwnd);

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
                    dist = mi.rcWork.top - currentMi.rcWork.bottom; // <-- THIS IS THE CORRECTED LINE
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
                case HOTKEY_ID_SHIFT_H: HandleMonitorSwitch(SnapDirection::Left); break;
                case HOTKEY_ID_SHIFT_L: HandleMonitorSwitch(SnapDirection::Right); break;
                case HOTKEY_ID_SHIFT_K: HandleMonitorSwitch(SnapDirection::Up); break;
                case HOTKEY_ID_SHIFT_J: HandleMonitorSwitch(SnapDirection::Down); break;
            }
            break;
        case WM_DESTROY:
            // Cleanup all border windows
            for (auto& pair : windowBorders) {
                DestroyWindow(pair.second);
            }
            windowBorders.clear();

            UnregisterHotKey(hwnd, HOTKEY_ID_H);
            UnregisterHotKey(hwnd, HOTKEY_ID_J);
            UnregisterHotKey(hwnd, HOTKEY_ID_K);
            UnregisterHotKey(hwnd, HOTKEY_ID_L);
            UnregisterHotKey(hwnd, HOTKEY_ID_SHIFT_H);
            UnregisterHotKey(hwnd, HOTKEY_ID_SHIFT_J);
            UnregisterHotKey(hwnd, HOTKEY_ID_SHIFT_K);
            UnregisterHotKey(hwnd, HOTKEY_ID_SHIFT_L);

            PostQuitMessage(0);
            break;
        case WM_DISPLAYCHANGE:
            RefreshMonitorCache();
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

    // Register border window class
    if (!RegisterBorderWindowClass(hInstance)) {
        MessageBoxW(NULL, L"Border Window Class Registration Failed!", L"Error", MB_ICONEXCLAMATION | MB_OK);
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
    
    if (!RegisterHotKey(hwnd, HOTKEY_ID_SHIFT_H, MOD_ALT | MOD_CONTROL | MOD_SHIFT, 'H')) {
        MessageBoxW(NULL, L"Failed to register hotkey Shift+H!", L"Error", MB_ICONEXCLAMATION | MB_OK);
    }
    if (!RegisterHotKey(hwnd, HOTKEY_ID_SHIFT_J, MOD_ALT | MOD_CONTROL | MOD_SHIFT, 'J')) {
        MessageBoxW(NULL, L"Failed to register hotkey Shift+J!", L"Error", MB_ICONEXCLAMATION | MB_OK);
    }
    if (!RegisterHotKey(hwnd, HOTKEY_ID_SHIFT_K, MOD_ALT | MOD_CONTROL | MOD_SHIFT, 'K')) {
        MessageBoxW(NULL, L"Failed to register hotkey Shift+K!", L"Error", MB_ICONEXCLAMATION | MB_OK);
    }
    if (!RegisterHotKey(hwnd, HOTKEY_ID_SHIFT_L, MOD_ALT | MOD_CONTROL | MOD_SHIFT, 'L')) {
        MessageBoxW(NULL, L"Failed to register hotkey Shift+L!", L"Error", MB_ICONEXCLAMATION | MB_OK);
    }
    
    // Setup event hook instead of timer
    hEventHook = SetWinEventHook(
        EVENT_MIN, EVENT_MAX,
        NULL,
        WinEventProc,
        0, 0,
        WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS
    );

    // Refresh monitor cache
    RefreshMonitorCache();

    // Initialize borders
    UpdateAllBorders();
    
    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
} 