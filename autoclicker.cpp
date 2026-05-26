// Simin AutoClicker v3 - Goated Edition
#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:WinMainCRTStartup")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "msimg32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <thread>
#include <atomic>
#include <cmath>

// ── IDs ──────────────────────────────────────────────────────────
#define ID_BTN_START      101
#define ID_BTN_STOP       102
#define ID_EDIT_INTERVAL  103
#define ID_SPIN_INTERVAL  104
#define ID_COMBO_BUTTON   105
#define ID_COMBO_TYPE     106
#define ID_COMBO_HOTKEY   107
#define ID_STATIC_STATUS  108
#define ID_CHECK_REPEAT   109
#define ID_EDIT_REPEAT    110
#define ID_SPIN_REPEAT    111
#define ID_STATIC_COUNT   112
#define ID_COMBO_LOCATION 113
#define ID_EDIT_X         114
#define ID_EDIT_Y         115
#define ID_BTN_PICKPOS    116
#define TIMER_UI          200

// ── Palette ───────────────────────────────────────────────────────
#define C_BG          RGB( 8,   7,  16)
#define C_HEADER_TOP  RGB(28,  18,  60)
#define C_HEADER_BOT  RGB( 8,   7,  16)
#define C_CARD        RGB(18,  16,  34)
#define C_CARD2       RGB(22,  20,  42)
#define C_BORD        RGB(60,  45, 110)
#define C_BORD_LIT    RGB(100, 75, 180)
#define C_VIO         RGB(139,  92, 246)
#define C_VIO_DK      RGB( 80,  50, 160)
#define C_VIO_LT      RGB(180, 150, 255)
#define C_FUCH        RGB(217,  70, 239)
#define C_GREEN       RGB( 52, 211, 153)
#define C_GREEN_DK    RGB( 20, 120,  80)
#define C_RED         RGB(240,  90,  90)
#define C_RED_DK      RGB(150,  40,  40)
#define C_TEXT        RGB(240, 238, 255)
#define C_SUB         RGB(160, 148, 210)
#define C_DIM         RGB(100,  88, 150)
#define C_INPUT       RGB(24,  22,  46)

// ── Globals ───────────────────────────────────────────────────────
HWND g_hWnd = NULL, hBtnStart = NULL, hBtnStop = NULL;
HWND hEditInterval = NULL, hSpinInterval = NULL;
HWND hComboButton = NULL, hComboType = NULL, hComboHotkey = NULL;
HWND hStaticStatus = NULL, hCheckRepeat = NULL;
HWND hEditRepeat = NULL, hSpinRepeat = NULL, hStaticCount = NULL;
HWND hComboLocation = NULL, hEditX = NULL, hEditY = NULL, hBtnPickPos = NULL;
HWND hStaticCPS = NULL;

std::atomic<bool>      g_running(false);
std::atomic<bool>      g_stopReq(false);
std::atomic<long long> g_clicks(0);
std::thread            g_thread;
bool  g_picking = false;
HHOOK g_mouseHook = NULL;

// Animation
int   g_phase = 0;   // 0-127, drives pulse glow
DWORD g_cpsTickLast = 0;
long long g_cpsClicksLast = 0;
double g_cps = 0.0;

// GDI objects
HBRUSH hBrBg = NULL, hBrCard = NULL, hBrInput = NULL, hBrVio = NULL, hBrGrn = NULL, hBrRed = NULL;
HFONT hFSm = NULL, hFLab = NULL, hFCtrl = NULL, hFTitle = NULL, hFHero = NULL,
hFSub = NULL, hFStat = NULL, hFCount = NULL, hFBtn = NULL, hFCPS = NULL;

// ── Color math ───────────────────────────────────────────────────
static COLORREF Lerp(COLORREF a, COLORREF b, float t) {
    return RGB(
        (int)(GetRValue(a) + (GetRValue(b) - GetRValue(a)) * t),
        (int)(GetGValue(a) + (GetGValue(b) - GetGValue(a)) * t),
        (int)(GetBValue(a) + (GetBValue(b) - GetBValue(a)) * t));
}
static COLORREF Scale(COLORREF c, float s) {
    return RGB((int)(GetRValue(c) * s), (int)(GetGValue(c) * s), (int)(GetBValue(c) * s));
}

// ── GDI helpers ──────────────────────────────────────────────────
static void FillRR(HDC dc, RECT r, int rad, COLORREF c) {
    HBRUSH br = CreateSolidBrush(c);
    HRGN rg = CreateRoundRectRgn(r.left, r.top, r.right, r.bottom, rad, rad);
    FillRgn(dc, rg, br); DeleteObject(rg); DeleteObject(br);
}
static void StrokeRR(HDC dc, RECT r, int rad, COLORREF c, int w = 1) {
    HPEN pen = CreatePen(PS_SOLID, w, c);
    HBRUSH nb = (HBRUSH)GetStockObject(NULL_BRUSH);
    HPEN op = (HPEN)SelectObject(dc, pen);
    HBRUSH ob = (HBRUSH)SelectObject(dc, nb);
    RoundRect(dc, r.left, r.top, r.right, r.bottom, rad, rad);
    SelectObject(dc, op); SelectObject(dc, ob); DeleteObject(pen);
}
// Vertical gradient fill
static void GradV(HDC dc, RECT r, COLORREF top, COLORREF bot) {
    int h = r.bottom - r.top;
    for (int i = 0;i < h;i++) {
        float t = (float)i / h;
        COLORREF c = Lerp(top, bot, t);
        HPEN pen = CreatePen(PS_SOLID, 1, c);
        HPEN op = (HPEN)SelectObject(dc, pen);
        MoveToEx(dc, r.left, r.top + i, NULL); LineTo(dc, r.right, r.top + i);
        SelectObject(dc, op); DeleteObject(pen);
    }
}
// Horizontal gradient fill
static void GradH(HDC dc, RECT r, COLORREF left, COLORREF right) {
    int w = r.right - r.left;
    for (int i = 0;i < w;i++) {
        float t = (float)i / w;
        COLORREF c = Lerp(left, right, t);
        HPEN pen = CreatePen(PS_SOLID, 1, c);
        HPEN op = (HPEN)SelectObject(dc, pen);
        MoveToEx(dc, r.left + i, r.top, NULL); LineTo(dc, r.left + i, r.bottom);
        SelectObject(dc, op); DeleteObject(pen);
    }
}
// Glow layers around a rounded rect
static void Glow(HDC dc, RECT r, int rad, COLORREF c, int layers) {
    for (int i = layers;i >= 1;i--) {
        float s = (float)(layers - i + 1) / (layers + 2);
        COLORREF gc = Scale(c, s * 0.6f);
        RECT gr = { r.left - i,r.top - i,r.right + i,r.bottom + i };
        StrokeRR(dc, gr, rad + i, gc, 1);
    }
}
// Shadow (dark rect offset)
static void Shadow(HDC dc, RECT r, int rad) {
    RECT sr = { r.left + 3,r.top + 4,r.right + 3,r.bottom + 4 };
    FillRR(dc, sr, rad, RGB(0, 0, 0));
}
// Dot (filled circle)
static void Dot(HDC dc, int cx, int cy, int radius, COLORREF c) {
    HBRUSH br = CreateSolidBrush(c);
    HRGN rg = CreateEllipticRgn(cx - radius, cy - radius, cx + radius, cy + radius);
    FillRgn(dc, rg, br); DeleteObject(rg); DeleteObject(br);
}

static int CtrlY(HWND h) { RECT r; GetWindowRect(h, &r); POINT p = { r.left,r.top }; ScreenToClient(g_hWnd, &p); return p.y; }
static int CtrlX(HWND h) { RECT r; GetWindowRect(h, &r); POINT p = { r.left,r.top }; ScreenToClient(g_hWnd, &p); return p.x; }

// ── Font callback ─────────────────────────────────────────────────
static BOOL CALLBACK SetFontProc(HWND h, LPARAM f) {
    SendMessageW(h, WM_SETFONT, (WPARAM)f, TRUE); return TRUE;
}

// ── Click logic ───────────────────────────────────────────────────
static void DoClick(int btn, int type) {
    DWORD dn, up;
    switch (btn) {
    case 1: dn = MOUSEEVENTF_RIGHTDOWN;  up = MOUSEEVENTF_RIGHTUP;  break;
    case 2: dn = MOUSEEVENTF_MIDDLEDOWN; up = MOUSEEVENTF_MIDDLEUP; break;
    default:dn = MOUSEEVENTF_LEFTDOWN;   up = MOUSEEVENTF_LEFTUP;   break;
    }
    int n = (type == 1) ? 2 : 1;
    for (int i = 0;i < n;i++) {
        mouse_event(dn, 0, 0, 0, 0); Sleep(18);
        mouse_event(up, 0, 0, 0, 0);
        if (i == 0 && n > 1) Sleep(30);
    }
}
static void ClickThreadFunc(int ms, int btn, int type, bool fix, int fx, int fy, bool lim, int maxN) {
    long long c = 0;
    while (!g_stopReq.load()) {
        if (fix) SetCursorPos(fx, fy);
        DoClick(btn, type);
        g_clicks.store(++c);
        if (lim && c >= maxN) break;
        for (int e = 0;e < ms && !g_stopReq.load();e += 10) Sleep(10);
    }
    g_running.store(false);
    PostMessageW(g_hWnd, WM_USER + 1, 0, 0);
}

static LRESULT CALLBACK MouseProc(int code, WPARAM wp, LPARAM lp) {
    if (code >= 0 && wp == WM_LBUTTONDOWN && g_picking) {
        MSLLHOOKSTRUCT* ms = (MSLLHOOKSTRUCT*)lp;
        wchar_t tmp[16];
        wsprintfW(tmp, L"%d", ms->pt.x); SetWindowTextW(hEditX, tmp);
        wsprintfW(tmp, L"%d", ms->pt.y); SetWindowTextW(hEditY, tmp);
        g_picking = false;
        SetWindowTextW(hBtnPickPos, L"Pick Pos");
        UnhookWindowsHookEx(g_mouseHook); g_mouseHook = NULL;
        ShowWindow(g_hWnd, SW_RESTORE); SetForegroundWindow(g_hWnd);
        return 1;
    }
    return CallNextHookEx(g_mouseHook, code, wp, lp);
}

static int GetInt(HWND h, int def = 100) {
    wchar_t b[32]; GetWindowTextW(h, b, 32);
    try { return std::stoi(b); }
    catch (...) { return def; }
}
static int HotkeyVK() {
    int s = (int)SendMessageW(hComboHotkey, CB_GETCURSEL, 0, 0);
    int v[] = { VK_F6,VK_F7,VK_F8,VK_F9 };
    return v[s < 4 ? s : 0];
}
static void UpdateBtns() {
    bool r = g_running.load();
    EnableWindow(hBtnStart, r ? FALSE : TRUE);
    EnableWindow(hBtnStop, r ? TRUE : FALSE);
    InvalidateRect(hBtnStart, NULL, TRUE);
    InvalidateRect(hBtnStop, NULL, TRUE);
}
static void StopClicking() {
    g_stopReq.store(true);
    if (g_thread.joinable()) g_thread.join();
    g_running.store(false);
    g_cps = 0.0;
    SetWindowTextW(hStaticStatus, L"IDLE");
    SetWindowTextW(hStaticCPS, L"0.0 clicks / sec");
    UpdateBtns();
    InvalidateRect(g_hWnd, NULL, FALSE);
}
static void StartClicking() {
    if (g_running.load()) return;
    int ms = GetInt(hEditInterval, 100); if (ms < 10)ms = 10;
    int btn = (int)SendMessageW(hComboButton, CB_GETCURSEL, 0, 0);
    int tp = (int)SendMessageW(hComboType, CB_GETCURSEL, 0, 0);
    int loc = (int)SendMessageW(hComboLocation, CB_GETCURSEL, 0, 0);
    bool fix = (loc == 1);
    int fx = GetInt(hEditX, 0), fy = GetInt(hEditY, 0);
    bool lim = (SendMessageW(hCheckRepeat, BM_GETCHECK, 0, 0) == BST_CHECKED);
    int maxN = GetInt(hEditRepeat, 100);
    g_clicks.store(0); g_cps = 0.0;
    g_cpsTickLast = GetTickCount(); g_cpsClicksLast = 0;
    g_running.store(true); g_stopReq.store(false);
    if (g_thread.joinable()) g_thread.join();
    g_thread = std::thread(ClickThreadFunc, ms, btn, tp, fix, fx, fy, lim, maxN);
    SetWindowTextW(hStaticStatus, L"RUNNING");
    UpdateBtns();
    InvalidateRect(g_hWnd, NULL, FALSE);
}

// ── Owner-draw button ────────────────────────────────────────────
static void DrawBtn(DRAWITEMSTRUCT* di, const wchar_t* label,
    COLORREF cTop, COLORREF cBot, COLORREF cGlow) {
    HDC dc = di->hDC;
    RECT rc = di->rcItem;
    bool dis = (di->itemState & ODS_DISABLED) != 0;
    bool sel = (di->itemState & ODS_SELECTED) != 0;

    if (dis) {
        FillRR(dc, rc, 10, RGB(20, 18, 36));
        StrokeRR(dc, rc, 10, RGB(50, 40, 80), 1);
        SetBkMode(dc, TRANSPARENT);
        SelectObject(dc, hFBtn);
        SetTextColor(dc, RGB(60, 50, 90));
        DrawTextW(dc, label, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        return;
    }

    // Shadow
    RECT sr = { rc.left + 2,rc.top + 3,rc.right + 2,rc.bottom + 3 };
    FillRR(dc, sr, 10, RGB(0, 0, 0));

    // Gradient fill
    RECT gr = rc;
    if (sel) { gr.top += 2; gr.left += 1; gr.right -= 1; }
    // Clip to rounded region first
    HRGN clip = CreateRoundRectRgn(gr.left, gr.top, gr.right, gr.bottom, 10, 10);
    SelectClipRgn(dc, clip);
    COLORREF t = sel ? Scale(cTop, 0.75f) : cTop;
    COLORREF b = sel ? Scale(cBot, 0.75f) : cBot;
    GradV(dc, gr, t, b);
    SelectClipRgn(dc, NULL);
    DeleteObject(clip);

    // Border glow
    if (!sel) Glow(dc, rc, 10, cGlow, 3);
    StrokeRR(dc, rc, 10, Scale(cGlow, 0.8f), 1);

    // Shine strip at top
    RECT shine = { rc.left + 8,rc.top + 3,rc.right - 8,rc.top + 14 };
    HBRUSH shBr = CreateSolidBrush(RGB(255, 255, 255));
    HRGN shRg = CreateRoundRectRgn(shine.left, shine.top, shine.right, shine.bottom, 6, 6);
    // Manual 15% opacity shine: just a lighter rect
    COLORREF shC = Lerp(cTop, RGB(255, 255, 255), 0.25f);
    FillRR(dc, shine, 6, shC);
    DeleteObject(shRg); DeleteObject(shBr);

    SetBkMode(dc, TRANSPARENT);
    SelectObject(dc, hFBtn);
    SetTextColor(dc, C_TEXT);
    RECT tr = gr;
    DrawTextW(dc, label, -1, &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// ── WndProc ───────────────────────────────────────────────────────
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
    static bool hotWas = false;

    switch (msg) {

    case WM_CREATE: {
        hBrBg = CreateSolidBrush(C_BG);
        hBrCard = CreateSolidBrush(C_CARD);
        hBrInput = CreateSolidBrush(C_INPUT);
        hBrVio = CreateSolidBrush(C_VIO);
        hBrGrn = CreateSolidBrush(C_GREEN);
        hBrRed = CreateSolidBrush(C_RED);

        auto MF = [](int h, int w, COLORREF col = 0, const wchar_t* face = L"Segoe UI")->HFONT {
            return CreateFontW(h, 0, 0, 0, w, 0, 0, 0, DEFAULT_CHARSET, 0, 0,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, face);
            };
        hFSm = MF(11, FW_NORMAL);
        hFLab = MF(11, FW_BOLD);
        hFCtrl = MF(14, FW_NORMAL);
        hFTitle = MF(24, FW_BOLD);
        hFHero = MF(30, FW_BOLD);
        hFSub = MF(12, FW_NORMAL);
        hFStat = MF(18, FW_BOLD);
        hFCount = MF(13, FW_NORMAL, 0, L"Consolas");
        hFCPS = MF(20, FW_BOLD, 0, L"Consolas");
        hFBtn = MF(15, FW_BOLD);

        int LM = 14, CW = 322, EH = 26, Y = 200, G = 8;

        // ── Card 1: Interval ─────────────────────────────────────
        hEditInterval = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"100",
            WS_CHILD | WS_VISIBLE | ES_NUMBER | ES_RIGHT,
            LM + 10, Y, CW - 32, EH, hWnd, (HMENU)ID_EDIT_INTERVAL, NULL, NULL);
        hSpinInterval = CreateWindowExW(0, UPDOWN_CLASSW, NULL,
            WS_CHILD | WS_VISIBLE | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_SETBUDDYINT,
            0, 0, 0, 0, hWnd, (HMENU)ID_SPIN_INTERVAL, NULL, NULL);
        SendMessageW(hSpinInterval, UDM_SETBUDDY, (WPARAM)hEditInterval, 0);
        SendMessageW(hSpinInterval, UDM_SETRANGE32, 10, 99999);
        SendMessageW(hSpinInterval, UDM_SETPOS32, 0, 100);
        Y += EH + G + 34;

        // ── Card 2: Mouse settings ────────────────────────────────
        hComboButton = CreateWindowExW(0, L"COMBOBOX", NULL,
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
            LM + 10, Y, 150, 130, hWnd, (HMENU)ID_COMBO_BUTTON, NULL, NULL);
        SendMessageW(hComboButton, CB_ADDSTRING, 0, (LPARAM)L"Left Button");
        SendMessageW(hComboButton, CB_ADDSTRING, 0, (LPARAM)L"Right Button");
        SendMessageW(hComboButton, CB_ADDSTRING, 0, (LPARAM)L"Middle Button");
        SendMessageW(hComboButton, CB_SETCURSEL, 0, 0);
        hComboType = CreateWindowExW(0, L"COMBOBOX", NULL,
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
            LM + 170, Y, 152, 130, hWnd, (HMENU)ID_COMBO_TYPE, NULL, NULL);
        SendMessageW(hComboType, CB_ADDSTRING, 0, (LPARAM)L"Single Click");
        SendMessageW(hComboType, CB_ADDSTRING, 0, (LPARAM)L"Double Click");
        SendMessageW(hComboType, CB_SETCURSEL, 0, 0);
        Y += EH + G + 34;

        // ── Card 3: Location ─────────────────────────────────────
        hComboLocation = CreateWindowExW(0, L"COMBOBOX", NULL,
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
            LM + 10, Y, CW - 20, 130, hWnd, (HMENU)ID_COMBO_LOCATION, NULL, NULL);
        SendMessageW(hComboLocation, CB_ADDSTRING, 0, (LPARAM)L"Follow cursor position");
        SendMessageW(hComboLocation, CB_ADDSTRING, 0, (LPARAM)L"Fixed screen position");
        SendMessageW(hComboLocation, CB_SETCURSEL, 0, 0);
        Y += EH + G;
        hEditX = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"0",
            WS_CHILD | WS_VISIBLE | ES_NUMBER, LM + 10, Y, 80, EH, hWnd, (HMENU)ID_EDIT_X, NULL, NULL);
        hEditY = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"0",
            WS_CHILD | WS_VISIBLE | ES_NUMBER, LM + 100, Y, 80, EH, hWnd, (HMENU)ID_EDIT_Y, NULL, NULL);
        hBtnPickPos = CreateWindowExW(0, L"BUTTON", L"Pick Pos",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            LM + 190, Y, 142, EH, hWnd, (HMENU)ID_BTN_PICKPOS, NULL, NULL);
        Y += EH + G + 34;

        // ── Card 4: Limit ────────────────────────────────────────
        hCheckRepeat = CreateWindowExW(0, L"BUTTON", L"Enable click limit",
            WS_CHILD | WS_VISIBLE | BS_CHECKBOX | BS_AUTOCHECKBOX,
            LM + 10, Y, 180, 24, hWnd, (HMENU)ID_CHECK_REPEAT, NULL, NULL);
        hEditRepeat = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"100",
            WS_CHILD | WS_VISIBLE | ES_NUMBER, LM + 200, Y, 82, EH, hWnd, (HMENU)ID_EDIT_REPEAT, NULL, NULL);
        hSpinRepeat = CreateWindowExW(0, UPDOWN_CLASSW, NULL,
            WS_CHILD | WS_VISIBLE | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_SETBUDDYINT,
            0, 0, 0, 0, hWnd, (HMENU)ID_SPIN_REPEAT, NULL, NULL);
        SendMessageW(hSpinRepeat, UDM_SETBUDDY, (WPARAM)hEditRepeat, 0);
        SendMessageW(hSpinRepeat, UDM_SETRANGE32, 1, 9999999);
        SendMessageW(hSpinRepeat, UDM_SETPOS32, 0, 100);
        Y += EH + G + 34;

        // ── Card 5: Hotkey ───────────────────────────────────────
        hComboHotkey = CreateWindowExW(0, L"COMBOBOX", NULL,
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
            LM + 10, Y, 110, 130, hWnd, (HMENU)ID_COMBO_HOTKEY, NULL, NULL);
        SendMessageW(hComboHotkey, CB_ADDSTRING, 0, (LPARAM)L"F6");
        SendMessageW(hComboHotkey, CB_ADDSTRING, 0, (LPARAM)L"F7");
        SendMessageW(hComboHotkey, CB_ADDSTRING, 0, (LPARAM)L"F8");
        SendMessageW(hComboHotkey, CB_ADDSTRING, 0, (LPARAM)L"F9");
        SendMessageW(hComboHotkey, CB_SETCURSEL, 0, 0);
        Y += EH + G + 34;

        // ── Status / CPS statics ─────────────────────────────────
        hStaticStatus = CreateWindowExW(0, L"STATIC", L"IDLE",
            WS_CHILD | WS_VISIBLE | SS_CENTER, LM, Y, CW, 28, hWnd, (HMENU)ID_STATIC_STATUS, NULL, NULL);
        Y += 32;
        hStaticCPS = CreateWindowExW(0, L"STATIC", L"0.0 clicks / sec",
            WS_CHILD | WS_VISIBLE | SS_CENTER, LM, Y, CW, 24, hWnd, (HMENU)ID_STATIC_COUNT + 1, NULL, NULL);
        Y += 26;
        hStaticCount = CreateWindowExW(0, L"STATIC", L"Total: 0",
            WS_CHILD | WS_VISIBLE | SS_CENTER, LM, Y, CW, 20, hWnd, (HMENU)ID_STATIC_COUNT, NULL, NULL);
        Y += 30;

        // ── Dual buttons ────────────────────────────────────────
        int BW = (CW - 8) / 2;
        hBtnStart = CreateWindowExW(0, L"BUTTON", L"START",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            LM, Y, BW, 46, hWnd, (HMENU)ID_BTN_START, NULL, NULL);
        hBtnStop = CreateWindowExW(0, L"BUTTON", L"STOP",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            LM + BW + 8, Y, BW, 46, hWnd, (HMENU)ID_BTN_STOP, NULL, NULL);
        EnableWindow(hBtnStop, FALSE);

        EnumChildWindows(hWnd, SetFontProc, (LPARAM)hFCtrl);
        SendMessageW(hStaticStatus, WM_SETFONT, (WPARAM)hFStat, TRUE);
        SendMessageW(hStaticCPS, WM_SETFONT, (WPARAM)hFCPS, TRUE);
        SendMessageW(hStaticCount, WM_SETFONT, (WPARAM)hFCount, TRUE);
        SendMessageW(hBtnStart, WM_SETFONT, (WPARAM)hFBtn, TRUE);
        SendMessageW(hBtnStop, WM_SETFONT, (WPARAM)hFBtn, TRUE);
        SendMessageW(hCheckRepeat, WM_SETFONT, (WPARAM)hFCtrl, TRUE);

        SetTimer(hWnd, TIMER_UI, 40, NULL); // ~25fps
        return 0;
    }

    case WM_TIMER: {
        if (wp == TIMER_UI) {
            // Hotkey poll
            bool dn = (GetAsyncKeyState(HotkeyVK()) & 0x8000) != 0;
            if (dn && !hotWas) { if (g_running.load())StopClicking(); else StartClicking(); }
            hotWas = dn;

            // Animation phase
            g_phase = (g_phase + 2) & 127;

            // CPS update every 600ms
            if (g_running.load()) {
                DWORD now = GetTickCount();
                DWORD dt = now - g_cpsTickLast;
                if (dt >= 600) {
                    long long cur = g_clicks.load();
                    long long delta = cur - g_cpsClicksLast;
                    g_cps = (double)delta / ((double)dt / 1000.0);
                    g_cpsClicksLast = cur;
                    g_cpsTickLast = now;
                    wchar_t b[64];
                    // Format CPS
                    int ci = (int)g_cps;
                    int cf = (int)((g_cps - (double)ci) * 10.0);
                    wsprintfW(b, L"%d.%d clicks / sec", ci, cf);
                    SetWindowTextW(hStaticCPS, b);
                    wsprintfW(b, L"Total: %lld", cur);
                    SetWindowTextW(hStaticCount, b);
                }
            }

            // Single invalidate - double buffer prevents flicker
            InvalidateRect(hWnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_USER + 1: {
        SetWindowTextW(hStaticStatus, L"IDLE");
        SetWindowTextW(hStaticCPS, L"0.0 clicks / sec");
        wchar_t b[64]; wsprintfW(b, L"Total: %lld", g_clicks.load());
        SetWindowTextW(hStaticCount, b);
        if (g_thread.joinable()) g_thread.join();
        g_cps = 0.0;
        UpdateBtns();
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    }

    case WM_COMMAND: {
        int id = LOWORD(wp);
        if (id == ID_BTN_START) StartClicking();
        if (id == ID_BTN_STOP)  StopClicking();
        if (id == ID_BTN_PICKPOS && !g_picking) {
            g_picking = true;
            SetWindowTextW(hBtnPickPos, L"Click anywhere...");
            ShowWindow(g_hWnd, SW_MINIMIZE);
            g_mouseHook = SetWindowsHookExW(WH_MOUSE_LL, MouseProc, NULL, 0);
        }
        return 0;
    }

    case WM_DRAWITEM: {
        DRAWITEMSTRUCT* di = (DRAWITEMSTRUCT*)lp;
        if (di->CtlID == ID_BTN_START)
            DrawBtn(di, L"START", RGB(110, 70, 220), RGB(70, 35, 150), C_VIO);
        if (di->CtlID == ID_BTN_STOP)
            DrawBtn(di, L"STOP", RGB(200, 60, 60), RGB(130, 30, 30), C_RED);
        return TRUE;
    }

    case WM_CTLCOLORSTATIC: {
        HDC dc = (HDC)wp; HWND hc = (HWND)lp;
        SetBkMode(dc, TRANSPARENT);
        if (hc == hStaticStatus) {
            SetTextColor(dc, g_running.load() ? C_GREEN : C_VIO_LT);
            SelectObject(dc, hFStat);
        }
        else if (hc == hStaticCPS) {
            SetTextColor(dc, g_running.load() ? C_GREEN : C_DIM);
            SelectObject(dc, hFCPS);
        }
        else if (hc == hStaticCount) {
            SetTextColor(dc, C_DIM);
            SelectObject(dc, hFCount);
        }
        else {
            SetTextColor(dc, C_TEXT);
        }
        return (LRESULT)hBrBg;
    }
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX: {
        HDC dc = (HDC)wp;
        SetTextColor(dc, C_TEXT); SetBkColor(dc, C_INPUT);
        return (LRESULT)hBrInput;
    }
    case WM_CTLCOLORBTN: {
        SetBkMode((HDC)wp, TRANSPARENT);
        SetTextColor((HDC)wp, C_TEXT);
        return (LRESULT)hBrBg;
    }

    case WM_ERASEBKGND:
        return 1; // suppressed - double buffer handles bg

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC realDC = BeginPaint(hWnd, &ps);
        RECT rc; GetClientRect(hWnd, &rc);
        int W = rc.right, LM = 14, RX = W - LM;

        // ── Double buffer setup ───────────────────────────────────
        HDC     dc = CreateCompatibleDC(realDC);
        HBITMAP bmp = CreateCompatibleBitmap(realDC, W, rc.bottom);
        HBITMAP old = (HBITMAP)SelectObject(dc, bmp);
        SetBkMode(dc, TRANSPARENT);

        // ── Full background ───────────────────────────────────────
        FillRect(dc, &rc, hBrBg);

        // ── Header gradient ───────────────────────────────────────
        RECT hdrR = { 0,0,W,80 };
        GradV(dc, hdrR, C_HEADER_TOP, C_BG);

        // Header side glow (left violet streak)
        RECT vstr = { 0,0,4,80 };
        GradV(dc, vstr, C_VIO, Scale(C_VIO, 0.2f));

        // Title
        SelectObject(dc, hFTitle);
        SetTextColor(dc, C_TEXT);
        RECT tr = { LM + 10,10,W - 10,46 };
        DrawTextW(dc, L"Simin AutoClicker", -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        // Subtitle
        SelectObject(dc, hFSub);
        SetTextColor(dc, C_VIO_LT);
        RECT sub = { LM + 10,46,W - 10,66 };
        DrawTextW(dc, L"v3.0  |  Goated Edition", -1, &sub, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        // Accent underline with gradient
        RECT aline = { 0,76,W,80 };
        GradH(dc, aline, C_VIO, C_FUCH);

        // ── Hero status panel (pulsing) ────────────────────────────
        bool running = g_running.load();
        // Pulse: triangle wave 0->1->0 over 128 phases
        float pulse = (g_phase < 64) ? (float)g_phase / 64.0f : (float)(128 - g_phase) / 64.0f;
        COLORREF heroGlow = running
            ? Lerp(C_GREEN_DK, C_GREEN, pulse)
            : Lerp(Scale(C_VIO, 0.3f), Scale(C_VIO, 0.6f), pulse);

        RECT heroCard = { LM,88,RX,188 };
        Shadow(dc, heroCard, 12);
        FillRR(dc, heroCard, 12, C_CARD2);
        Glow(dc, heroCard, 12, heroGlow, 4);
        StrokeRR(dc, heroCard, 12, heroGlow, 1);

        // Left accent bar on hero card
        RECT hbar = { LM,88,LM + 5,188 };
        COLORREF barC = running ? C_GREEN : C_VIO;
        GradV(dc, hbar, barC, Scale(barC, 0.3f));

        // Pulsing dot
        int dotX = LM + 28, dotY = 138;
        float dotScale = 0.7f + pulse * 0.3f;
        int dotR = (int)(7 * dotScale);
        // Outer glow
        Dot(dc, dotX, dotY, dotR + 4, Scale(barC, 0.2f));
        Dot(dc, dotX, dotY, dotR + 2, Scale(barC, 0.4f));
        Dot(dc, dotX, dotY, dotR, barC);

        // Hotkey badge top-right of hero
        int hkSel = (int)SendMessageW(hComboHotkey, CB_GETCURSEL, 0, 0);
        const wchar_t* hkN[] = { L"F6",L"F7",L"F8",L"F9" };
        wchar_t hkTxt[24];
        wsprintfW(hkTxt, L"Hotkey: %s", hkN[hkSel < 4 ? hkSel : 0]);
        RECT hkR = { RX - 110,92,RX - 8,108 };
        FillRR(dc, hkR, 6, RGB(30, 25, 55));
        StrokeRR(dc, hkR, 6, C_BORD, 1);
        SelectObject(dc, hFSm);
        SetTextColor(dc, C_VIO_LT);
        DrawTextW(dc, hkTxt, -1, &hkR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // ── Settings cards (macro-expanded, no lambdas for MSVC) ─────
#define DRAW_CARD(topCtrl,botCtrl,padT,padB,lbl,accent) { \
            int ty=CtrlY(topCtrl), by=CtrlY(botCtrl); \
            RECT cr={LM,ty-(padT),RX,by+26+(padB)}; \
            Shadow(dc,cr,9); FillRR(dc,cr,9,C_CARD); StrokeRR(dc,cr,9,C_BORD,1); \
            RECT strip={LM,ty-(padT),LM+4,by+26+(padB)}; \
            GradV(dc,strip,(accent),Scale((accent),0.3f)); \
            SelectObject(dc,hFLab); SetTextColor(dc,Scale((accent),0.9f)); \
            RECT lr2={LM+12,ty-(padT)+4,RX-8,ty-(padT)+18}; \
            DrawTextW(dc,(lbl),-1,&lr2,DT_LEFT|DT_VCENTER|DT_SINGLELINE); }

        DRAW_CARD(hEditInterval, hEditInterval, 20, 8, L"CLICK INTERVAL (ms)", C_VIO);
        DRAW_CARD(hComboButton, hComboType, 20, 8, L"MOUSE SETTINGS", C_FUCH);
        DRAW_CARD(hComboLocation, hEditX, 20, 8, L"CLICK LOCATION", C_VIO);
        DRAW_CARD(hCheckRepeat, hCheckRepeat, 20, 8, L"CLICK LIMIT", C_FUCH);
        DRAW_CARD(hComboHotkey, hComboHotkey, 20, 8, L"TOGGLE HOTKEY", C_VIO);
#undef DRAW_CARD

        // Sub-labels inside cards
#define SUB_LAB(ctrl,ox,oy,t) { \
            int sx=CtrlX(ctrl)+(ox), sy2=CtrlY(ctrl)+(oy); \
            SelectObject(dc,hFSm); SetTextColor(dc,C_DIM); \
            RECT slr={sx,sy2,sx+160,sy2+14}; \
            DrawTextW(dc,(t),-1,&slr,DT_LEFT|DT_VCENTER|DT_SINGLELINE); }

        SUB_LAB(hComboButton, 0, -16, L"BUTTON");
        SUB_LAB(hComboType, 0, -16, L"TYPE");
        SUB_LAB(hComboLocation, 0, -16, L"MODE");
        SUB_LAB(hEditX, 0, -16, L"X");
        SUB_LAB(hEditY, 0, -16, L"Y");
        SUB_LAB(hBtnPickPos, 0, -16, L"CAPTURE");
        SUB_LAB(hEditRepeat, 0, -16, L"COUNT");
        SUB_LAB(hComboHotkey, 0, -16, L"KEY");
#undef SUB_LAB

        // ── Status/CPS card ───────────────────────────────────────
        int sY = CtrlY(hStaticStatus);
        RECT sCard = { LM,sY - 12,RX,sY + 92 };
        Shadow(dc, sCard, 10);
        FillRR(dc, sCard, 10, C_CARD2);
        // Animated border
        COLORREF sBord = running ? Lerp(C_GREEN_DK, C_GREEN, pulse) : Lerp(C_BORD, C_BORD_LIT, pulse * 0.5f);
        Glow(dc, sCard, 10, sBord, 3);
        StrokeRR(dc, sCard, 10, sBord, 1);
        // Left strip
        RECT sStrip = { LM,sY - 12,LM + 4,sY + 92 };
        COLORREF sC = running ? C_GREEN : C_VIO;
        GradV(dc, sStrip, sC, Scale(sC, 0.2f));

        // ── Blit offscreen buffer to screen (no flicker) ────────
        BitBlt(realDC, 0, 0, W, rc.bottom, dc, 0, 0, SRCCOPY);
        SelectObject(dc, old);
        DeleteObject(bmp);
        DeleteDC(dc);
        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        StopClicking();
        if (g_mouseHook) { UnhookWindowsHookEx(g_mouseHook);g_mouseHook = NULL; }
        KillTimer(hWnd, TIMER_UI);
        DeleteObject(hBrBg);  DeleteObject(hBrCard); DeleteObject(hBrInput);
        DeleteObject(hBrVio); DeleteObject(hBrGrn);  DeleteObject(hBrRed);
        DeleteObject(hFSm);   DeleteObject(hFLab);   DeleteObject(hFCtrl);
        DeleteObject(hFTitle);DeleteObject(hFHero);  DeleteObject(hFSub);
        DeleteObject(hFStat); DeleteObject(hFCount); DeleteObject(hFBtn); DeleteObject(hFCPS);
        PostQuitMessage(0);
        return 0;

    default: return DefWindowProcW(hWnd, msg, wp, lp);
    }
}

// ── Entry Point ───────────────────────────────────────────────────
int WINAPI WinMain(
    _In_     HINSTANCE hi,
    _In_opt_ HINSTANCE hpi,
    _In_     LPSTR     lp,
    _In_     int       nc)
{
    (void)hpi;(void)lp;(void)nc;
    INITCOMMONCONTROLSEX icc = { sizeof(icc),ICC_WIN95_CLASSES | ICC_UPDOWN_CLASS };
    InitCommonControlsEx(&icc);
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hi;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"SiminAC3";
    wc.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    RegisterClassExW(&wc);
    g_hWnd = CreateWindowExW(WS_EX_APPWINDOW, L"SiminAC3", L"Simin AutoClicker",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 350, 700, NULL, NULL, hi, NULL);
    ShowWindow(g_hWnd, SW_SHOW);
    UpdateWindow(g_hWnd);
    MSG m;
    while (GetMessageW(&m, NULL, 0, 0)) { TranslateMessage(&m);DispatchMessageW(&m); }
    return (int)m.wParam;
}