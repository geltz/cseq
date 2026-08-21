#pragma once
#include "cseq_globals.h"
#include "cseq_dsp.h"
#include "cseq_ui.h"
#include "cseq_actions.h"
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

/* ============================================================
 * KEYBINDS POPUP DIALOG
 * ============================================================ */
static HWND g_keybindsHwnd = NULL;

typedef struct {
    const char *key;
    const char *desc;
} KeybindRow;

static const KeybindRow kKeybindList[] = {
    { "Space / Enter",          "Play / Pause toggle" },
    { "Home",                   "Jump playhead to start" },
    { "End",                    "Stop & jump to start" },
    { "+  /  -",                "BPM adjust (+/- 2 BPM)" },
    { "<  /  >",                "Change Bar Count (+/- 1 Bar)" },
    { "[  /  ]",                "Adjust Swing (+/- 5%)" },
    { "S",                      "Split clip(s) at playhead" },
    { "Q",                      "Toggle 1/16 Beat Snap" },
    { "L",                      "Toggle Lo-Fi 12-bit FX" },
    { "Delete",                 "Delete selected clip(s)" },
    { "Insert",                 "Import audio sample at cursor" },
    { "Page Up / Down",         "Zoom Timeline in / out" },
    { "Ctrl + T / Shift + T",   "Add / Remove Track" },
    { "Ctrl + C / V / X",       "Copy / Paste / Cut selected clips" },
    { "Ctrl + Z / Y",           "Undo / Redo action" },
    { "Ctrl + A / D",           "Select All / Deselect All clips" },
    { "Ctrl + S / O",           "Save / Load project (.csq)" },
    { "E",                      "Export timeline to 32-bit WAV" },
    { "Alt + Drag Clip",        "Slip edit audio content" },
    { "Shift + Drag / Wheel",   "Adjust Clip Volume" },
    { "Right-Click Clip/Track", "Context Menu (EQ, Rate, Fades)" },
    { "Middle-Click Drag",      "Pan timeline viewport" }
};
#define KEYBIND_COUNT ((int)(sizeof(kKeybindList) / sizeof(kKeybindList[0])))

static LRESULT CALLBACK KeybindsWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;
        if (w <= 0 || h <= 0) { EndPaint(hwnd, &ps); return 0; }

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBmp = CreateCompatibleBitmap(hdc, w, h);
        HGDIOBJ oldBmp = SelectObject(memDC, memBmp);

        HBRUSH bg = CreateSolidBrush(RGB(17, 20, 26));
        FillRect(memDC, &rc, bg);
        DeleteObject(bg);

        SetBkMode(memDC, TRANSPARENT);

        // Centered Two-Column Keybind List starting cleanly at top
        int startY = 14;
        int rowH = 18;
        int colKeyRight = w / 2 - 12;
        int colDescLeft = w / 2 + 12;

        for (int i = 0; i < KEYBIND_COUNT; ++i) {
            int y = startY + i * rowH;

            // Key shortcut
            SetTextColor(memDC, RGB(110, 210, 240));
            RECT keyRc = { 16, y, colKeyRight, y + rowH };
            DrawTextA(memDC, kKeybindList[i].key, -1, &keyRc, DT_RIGHT | DT_SINGLELINE | DT_NOPREFIX);

            // Description
            SetTextColor(memDC, RGB(170, 180, 195));
            RECT descRc = { colDescLeft, y, w - 16, y + rowH };
            DrawTextA(memDC, kKeybindList[i].desc, -1, &descRc, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);
        }

        // Close Hint at Bottom (Cleanly spaced, no overlap)
        SetTextColor(memDC, RGB(85, 96, 115));
        RECT hintRc = {0, h - 22, w, h - 4};
        DrawTextA(memDC, "Press [ESC] or [ENTER] to close", -1, &hintRc, DT_CENTER | DT_SINGLELINE | DT_NOPREFIX);

        BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, oldBmp);
        DeleteObject(memBmp);
        DeleteDC(memDC);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE || wParam == VK_RETURN) {
            ShowWindow(hwnd, SW_HIDE);
        }
        return 0;

    case WM_CLOSE:
        ShowWindow(hwnd, SW_HIDE);
        return 0;

    case WM_DESTROY:
        g_keybindsHwnd = NULL;
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static inline void open_keybinds_dialog(HWND parentHwnd) {
    if (!g_keybindsHwnd) {
        static bool s_kbRegistered = false;
        if (!s_kbRegistered) {
            WNDCLASSA wc = {0};
            wc.lpfnWndProc = KeybindsWndProc;
            wc.hInstance = GetModuleHandle(NULL);
            wc.lpszClassName = "CSeqKeybindsClass";
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            RegisterClassA(&wc);
            s_kbRegistered = true;
        }

        int rw = 490, rh = 480;
        int scrW = GetSystemMetrics(SM_CXSCREEN);
        int scrH = GetSystemMetrics(SM_CYSCREEN);
        int rx = (scrW - rw) / 2;
        int ry = (scrH - rh) / 2;

        g_keybindsHwnd = CreateWindowExA(
            WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
            "CSeqKeybindsClass",
            "Keybinds & Shortcuts",
            WS_POPUPWINDOW | WS_CAPTION | WS_VISIBLE,
            rx, ry, rw, rh,
            parentHwnd, NULL, GetModuleHandle(NULL), NULL
        );
    }

    ShowWindow(g_keybindsHwnd, SW_SHOW);
    SetForegroundWindow(g_keybindsHwnd);
    InvalidateRect(g_keybindsHwnd, NULL, FALSE);
}

/* ============================================================
 * CUSTOM PLAYBACK RATE DIALOG
 * ============================================================ */
typedef struct {
    HWND hwnd;
    int clipIdx;
    int trackIdx;
    bool isDragging;
} RateWindowContext;

static RateWindowContext g_RateWin = {0};

static inline bool rate_win_track_has_selection(int trackIdx) {
    seq_lock();
    for (int i = 0; i < g_Seq.clipCount; ++i) {
        if (g_Seq.clips[i].track == trackIdx && g_Seq.clips[i].isSelected) {
            seq_unlock();
            return true;
        }
    }
    seq_unlock();
    return false;
}

static inline float get_custom_rate_value(void) {
    seq_lock();
    if (g_RateWin.clipIdx >= 0 && g_RateWin.clipIdx < g_Seq.clipCount) {
        float r = g_Seq.clips[g_RateWin.clipIdx].playbackRate;
        seq_unlock();
        return r;
    }
    if (g_RateWin.trackIdx >= 0 && g_RateWin.trackIdx < g_Seq.trackCount) {
        bool hasSelectionOnTrack = false;
        for (int i = 0; i < g_Seq.clipCount; ++i) {
            if (g_Seq.clips[i].track == g_RateWin.trackIdx && g_Seq.clips[i].isSelected) {
                hasSelectionOnTrack = true;
                break;
            }
        }
        for (int i = 0; i < g_Seq.clipCount; ++i) {
            if (g_Seq.clips[i].track != g_RateWin.trackIdx) continue;
            if (hasSelectionOnTrack && !g_Seq.clips[i].isSelected) continue;
            float r = g_Seq.clips[i].playbackRate;
            seq_unlock();
            return r;
        }
    }
    seq_unlock();
    return 1.0f;
}

static inline void set_custom_rate_value(float rate) {
    if (rate < 0.01f) rate = 0.01f;
    if (rate > 2.00f) rate = 2.00f;

    seq_lock();
    if (g_RateWin.clipIdx >= 0 && g_RateWin.clipIdx < g_Seq.clipCount) {
        if (g_Seq.clips[g_RateWin.clipIdx].isSelected) {
            for (int i = 0; i < g_Seq.clipCount; ++i) {
                if (g_Seq.clips[i].isSelected) {
                    g_Seq.clips[i].playbackRate = rate;
                }
            }
        } else {
            g_Seq.clips[g_RateWin.clipIdx].playbackRate = rate;
        }
    } else if (g_RateWin.trackIdx >= 0 && g_RateWin.trackIdx < g_Seq.trackCount) {
        bool hasSelectionOnTrack = false;
        for (int i = 0; i < g_Seq.clipCount; ++i) {
            if (g_Seq.clips[i].track == g_RateWin.trackIdx && g_Seq.clips[i].isSelected) {
                hasSelectionOnTrack = true;
                break;
            }
        }
        for (int i = 0; i < g_Seq.clipCount; ++i) {
            if (g_Seq.clips[i].track != g_RateWin.trackIdx) continue;
            if (hasSelectionOnTrack && !g_Seq.clips[i].isSelected) continue;
            g_Seq.clips[i].playbackRate = rate;
        }
    }
    seq_unlock();
}

static LRESULT CALLBACK RateWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_ERASEBKGND:
        return 1;

    case WM_LBUTTONDOWN: {
        int mx = GET_X_LPARAM(lParam);
        RECT rc;
        GetClientRect(hwnd, &rc);
        int trackLeft = 24, trackRight = rc.right - 24;
        int trackW = trackRight - trackLeft;

        if (mx >= trackLeft && mx <= trackRight && trackW > 0) {
            float norm = (float)(mx - trackLeft) / (float)trackW;
            if (norm < 0.0f) norm = 0.0f;
            if (norm > 1.0f) norm = 1.0f;

            float rate = 0.01f + norm * (2.00f - 0.01f);
            set_custom_rate_value(rate);
            g_RateWin.isDragging = true;
            SetCapture(hwnd);
            InvalidateRect(hwnd, NULL, FALSE);
            if (g_hWnd) InvalidateRect(g_hWnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_MOUSEMOVE: {
        if (g_RateWin.isDragging) {
            int mx = GET_X_LPARAM(lParam);
            RECT rc;
            GetClientRect(hwnd, &rc);
            int trackLeft = 24, trackRight = rc.right - 24;
            int trackW = trackRight - trackLeft;

            if (trackW > 0) {
                float norm = (float)(mx - trackLeft) / (float)trackW;
                if (norm < 0.0f) norm = 0.0f;
                if (norm > 1.0f) norm = 1.0f;

                float rate = 0.01f + norm * (2.00f - 0.01f);
                set_custom_rate_value(rate);
                InvalidateRect(hwnd, NULL, FALSE);
                if (g_hWnd) InvalidateRect(g_hWnd, NULL, FALSE);
            }
        }
        return 0;
    }

    case WM_LBUTTONUP: {
        if (g_RateWin.isDragging) {
            g_RateWin.isDragging = false;
            ReleaseCapture();
            InvalidateRect(hwnd, NULL, FALSE);
            if (g_hWnd) InvalidateRect(g_hWnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_MOUSEWHEEL: {
        short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
        float currentRate = get_custom_rate_value();
        float delta = (zDelta > 0) ? 0.05f : -0.05f;
        float newRate = currentRate + delta;
        if (newRate < 0.01f) newRate = 0.01f;
        if (newRate > 2.00f) newRate = 2.00f;
        set_custom_rate_value(newRate);
        InvalidateRect(hwnd, NULL, FALSE);
        if (g_hWnd) InvalidateRect(g_hWnd, NULL, FALSE);
        return 0;
    }

    case WM_RBUTTONDOWN: {
        set_custom_rate_value(1.0f);
        InvalidateRect(hwnd, NULL, FALSE);
        if (g_hWnd) InvalidateRect(g_hWnd, NULL, FALSE);
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;
        if (w <= 0 || h <= 0) { EndPaint(hwnd, &ps); return 0; }

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBmp = CreateCompatibleBitmap(hdc, w, h);
        HGDIOBJ oldBmp = SelectObject(memDC, memBmp);
        HGDIOBJ origPen = GetCurrentObject(memDC, OBJ_PEN);
        HGDIOBJ origBrush = GetCurrentObject(memDC, OBJ_BRUSH);

        HBRUSH bg = CreateSolidBrush(RGB(17, 20, 26));
        FillRect(memDC, &rc, bg);
        DeleteObject(bg);

        int trackLeft = 24, trackRight = w - 24;
        int trackY = 56;
        int trackW = trackRight - trackLeft;

        HPEN railPen = CreatePen(PS_SOLID, 4, RGB(28, 33, 42));
        SelectObject(memDC, railPen);
        MoveToEx(memDC, trackLeft, trackY, NULL);
        LineTo(memDC, trackRight, trackY);
        SelectObject(memDC, origPen);
        DeleteObject(railPen);

        int centerNormX = trackLeft + (int)(((1.0f - 0.01f) / (2.00f - 0.01f)) * (float)trackW);
        HPEN notchPen = CreatePen(PS_SOLID, 2, RGB(60, 72, 90));
        SelectObject(memDC, notchPen);
        MoveToEx(memDC, centerNormX, trackY - 6, NULL);
        LineTo(memDC, centerNormX, trackY + 7);
        SelectObject(memDC, origPen);
        DeleteObject(notchPen);

        float currentRate = get_custom_rate_value();
        float norm = (currentRate - 0.01f) / (2.00f - 0.01f);
        if (norm < 0.0f) norm = 0.0f;
        if (norm > 1.0f) norm = 1.0f;
        int thumbX = trackLeft + (int)(norm * (float)trackW);

        HPEN fillPen = CreatePen(PS_SOLID, 4, RGB(80, 210, 240));
        SelectObject(memDC, fillPen);
        MoveToEx(memDC, trackLeft, trackY, NULL);
        LineTo(memDC, thumbX, trackY);
        SelectObject(memDC, origPen);
        DeleteObject(fillPen);

        HBRUSH thumbBrush = CreateSolidBrush(RGB(80, 240, 180));
        HPEN thumbBorder = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
        SelectObject(memDC, thumbBrush);
        SelectObject(memDC, thumbBorder);
        Ellipse(memDC, thumbX - 7, trackY - 7, thumbX + 8, trackY + 8);
        SelectObject(memDC, origPen);
        SelectObject(memDC, origBrush);
        DeleteObject(thumbBorder);
        DeleteObject(thumbBrush);

        SetBkMode(memDC, TRANSPARENT);
        SetTextColor(memDC, RGB(215, 225, 240));
        char valBuf[32];
        snprintf(valBuf, sizeof(valBuf), "%.2fx", currentRate);
        RECT valRc = {0, 12, w, 34};
        DrawTextA(memDC, valBuf, -1, &valRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SetTextColor(memDC, RGB(80, 95, 115));
        TextOutA(memDC, trackLeft, trackY + 12, "0.01x", 5);
        TextOutA(memDC, centerNormX - 14, trackY + 12, "1.00x", 5);
        TextOutA(memDC, trackRight - 28, trackY + 12, "2.00x", 5);

        BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, origPen);
        SelectObject(memDC, origBrush);
        SelectObject(memDC, oldBmp);
        DeleteObject(memBmp);
        DeleteDC(memDC);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_CLOSE:
        ShowWindow(hwnd, SW_HIDE);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static inline void open_custom_rate_dialog(HWND parentHwnd, int clipIdx, int trackIdx) {
    if (!g_RateWin.hwnd) {
        static bool s_rateRegistered = false;
        if (!s_rateRegistered) {
            WNDCLASSA wc = {0};
            wc.lpfnWndProc = RateWndProc;
            wc.hInstance = GetModuleHandle(NULL);
            wc.lpszClassName = "CSeqRateWindowClass";
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            RegisterClassA(&wc);
            s_rateRegistered = true;
        }

        int rw = 320, rh = 130;
        int scrW = GetSystemMetrics(SM_CXSCREEN);
        int scrH = GetSystemMetrics(SM_CYSCREEN);
        int rx = (scrW - rw) / 2;
        int ry = (scrH - rh) / 2;

        g_RateWin.hwnd = CreateWindowExA(
            WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
            "CSeqRateWindowClass",
            "Custom Playback Rate",
            WS_POPUPWINDOW | WS_CAPTION | WS_VISIBLE,
            rx, ry, rw, rh,
            parentHwnd, NULL, GetModuleHandle(NULL), NULL
        );
    }

    g_RateWin.clipIdx = clipIdx;
    g_RateWin.trackIdx = trackIdx;

    char titleBuf[64];
    if (clipIdx >= 0) {
        snprintf(titleBuf, sizeof(titleBuf), "Clip Playback Rate");
    } else {
        snprintf(titleBuf, sizeof(titleBuf), "Track %d - Playback Rate", trackIdx + 1);
    }
    SetWindowTextA(g_RateWin.hwnd, titleBuf);

    ShowWindow(g_RateWin.hwnd, SW_SHOW);
    SetForegroundWindow(g_RateWin.hwnd);
    InvalidateRect(g_RateWin.hwnd, NULL, FALSE);
}

/* ============================================================
 * FULL-RANGE PARAMETRIC TRACK EQ
 * ============================================================ */
static int  g_eqTrack = 0;
static HWND g_eqHwnd  = NULL;

static inline float get_eq_band_freq_hz(int trackIdx, int band) {
    if (trackIdx < 0 || trackIdx >= MAX_TRACKS || band < 0 || band >= 3) return 1000.0f;
    float param = g_Seq.trackEqFreq[trackIdx][band];
    if (param < 0.0f) param = 0.0f;
    if (param > 1.0f) param = 1.0f;
    return 20.0f * powf(1000.0f, param);
}

static inline float get_eq_band_q_factor(int trackIdx, int band) {
    if (trackIdx < 0 || trackIdx >= MAX_TRACKS || band < 0 || band >= 3) return 0.7f;
    float q = 0.35f + g_Seq.trackEqQ[trackIdx][band] * 4.65f;
    if (q < 0.20f) q = 0.20f;
    if (q > 8.00f) q = 8.00f;
    return q;
}

static inline void update_track_eq_params(int trackIdx) {
    if (trackIdx < 0 || trackIdx >= g_Seq.trackCount || trackIdx >= MAX_TRACKS) return;

    seq_lock();
    smooth_eq3_set_params(&g_Seq.trackEQ[trackIdx],
                          g_Seq.trackEqHigh[trackIdx],
                          g_Seq.trackEqMid[trackIdx],
                          g_Seq.trackEqLow[trackIdx]);

    float gains[3] = {
        g_Seq.trackEqLow[trackIdx],
        g_Seq.trackEqMid[trackIdx],
        g_Seq.trackEqHigh[trackIdx]
    };

    for (int b = 0; b < 3; ++b) {
        float f = get_eq_band_freq_hz(trackIdx, b);
        float q = get_eq_band_q_factor(trackIdx, b);
        float gainDb = (gains[b] - 0.5f) * 24.0f;
        peak_biquad_set(&g_Seq.trackPeak[trackIdx][b], f, q, gainDb, (float)SAMPLE_RATE);
    }

    g_Seq.trackEqActive[trackIdx] = true;
    seq_unlock();
}

static LRESULT CALLBACK EqWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static int dragBand = -1;

    switch (msg) {
    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;
        if (w <= 0 || h <= 0) { EndPaint(hwnd, &ps); return 0; }

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBmp = CreateCompatibleBitmap(hdc, w, h);
        HGDIOBJ oldBmp = SelectObject(memDC, memBmp);
        HGDIOBJ origPen = GetCurrentObject(memDC, OBJ_PEN);
        HGDIOBJ origBrush = GetCurrentObject(memDC, OBJ_BRUSH);

        HBRUSH bg = CreateSolidBrush(RGB(17, 20, 26));
        FillRect(memDC, &rc, bg);
        DeleteObject(bg);

        int graphL = 52, graphR = w - 16, graphT = 16, graphB = h - 46;
        int graphW = graphR - graphL, graphH = graphB - graphT;
        int graphMidY = graphT + graphH / 2;
        float pxPerDb = (float)(graphH / 2 - 6) / 12.0f;

        int yPlus12  = graphMidY - (int)(12.0f * pxPerDb);
        int yMinus12 = graphMidY + (int)(12.0f * pxPerDb);

        HPEN gridPen = CreatePen(PS_SOLID, 1, RGB(27, 32, 42));
        SelectObject(memDC, gridPen);
        MoveToEx(memDC, graphL, yPlus12, NULL);  LineTo(memDC, graphR, yPlus12);
        MoveToEx(memDC, graphL, graphMidY, NULL); LineTo(memDC, graphR, graphMidY);
        MoveToEx(memDC, graphL, yMinus12, NULL); LineTo(memDC, graphR, yMinus12);
        SelectObject(memDC, origPen);
        DeleteObject(gridPen);

        SetBkMode(memDC, TRANSPARENT);
        SetTextColor(memDC, RGB(85, 96, 114));
        TextOutA(memDC, 10, yPlus12 - 7, "+12 dB", 6);
        TextOutA(memDC, 18, graphMidY - 7, "0", 1);
        TextOutA(memDC, 10, yMinus12 - 7, "-12 dB", 6);

        float gains[3] = {
            g_Seq.trackEqLow[g_eqTrack],
            g_Seq.trackEqMid[g_eqTrack],
            g_Seq.trackEqHigh[g_eqTrack]
        };

        const COLORREF bandColors[3]   = { RGB(255, 160, 60), RGB(90, 245, 180), RGB(135, 195, 255) };
        const COLORREF dimDottedCol[3] = { RGB(140, 85, 40),  RGB(45, 125, 95),   RGB(55, 105, 145) };

        POINT nodePts[3];
        for (int i = 0; i < 3; ++i) {
            float normX = g_Seq.trackEqFreq[g_eqTrack][i];
            if (normX < 0.0f) normX = 0.0f;
            if (normX > 1.0f) normX = 1.0f;

            float gainDb = (gains[i] - 0.5f) * 24.0f;
            nodePts[i].x = graphL + (int)(normX * (float)graphW);
            nodePts[i].y = graphMidY - (int)(gainDb * pxPerDb);

            if (nodePts[i].y < graphT + 2) nodePts[i].y = graphT + 2;
            if (nodePts[i].y > graphB - 2) nodePts[i].y = graphB - 2;
        }

        for (int b = 0; b < 3; ++b) {
            float gainDb = (gains[b] - 0.5f) * 24.0f;
            if (fabsf(gainDb) < 0.2f) continue;

            float bNormX = g_Seq.trackEqFreq[g_eqTrack][b];
            float bGainPx = gainDb * pxPerDb;
            float bQ = get_eq_band_q_factor(g_eqTrack, b);
            float sigma = 0.18f / sqrtf(bQ);

            HPEN dotPen = CreatePen(PS_DOT, 1, dimDottedCol[b]);
            SelectObject(memDC, dotPen);
            bool started = false;

            int startX = max(graphL, (int)(graphL + (bNormX - 3.0f * sigma) * graphW));
            int endX   = min(graphR, (int)(graphL + (bNormX + 3.0f * sigma) * graphW));

            for (int x = startX; x <= endX; x += 2) {
                float normX = (float)(x - graphL) / (float)graphW;
                float dist = (normX - bNormX);
                float val = bGainPx * expf(-(dist * dist) / (2.0f * sigma * sigma));

                if (fabsf(val) < 0.5f) val = 0.0f;

                int cy = graphMidY - (int)val;
                if (cy < graphT) cy = graphT;
                if (cy > graphB) cy = graphB;

                if (!started) { MoveToEx(memDC, x, cy, NULL); started = true; }
                else LineTo(memDC, x, cy);
            }
            SelectObject(memDC, origPen);
            DeleteObject(dotPen);
        }

        POINT fillPts[512];
        int ptCount = 0;
        fillPts[ptCount++] = (POINT){ graphL, graphMidY };

        for (int x = graphL; x <= graphR; x += 2) {
            float normX = (float)(x - graphL) / (float)graphW;
            float totalDeltaY = 0.0f;

            for (int b = 0; b < 3; ++b) {
                float bNormX = g_Seq.trackEqFreq[g_eqTrack][b];
                float bGainPx = (gains[b] - 0.5f) * 24.0f * pxPerDb;
                float bQ = get_eq_band_q_factor(g_eqTrack, b);
                float sigma = 0.18f / sqrtf(bQ);
                float dist = (normX - bNormX);
                totalDeltaY += bGainPx * expf(-(dist * dist) / (2.0f * sigma * sigma));
            }

            int cy = graphMidY - (int)totalDeltaY;
            if (cy < graphT) cy = graphT;
            if (cy > graphB) cy = graphB;

            if (ptCount < 510) fillPts[ptCount++] = (POINT){ x, cy };
        }
        fillPts[ptCount++] = (POINT){ graphR, graphMidY };

        HBRUSH fillBrush = CreateSolidBrush(RGB(18, 38, 48));
        SelectObject(memDC, fillBrush);
        SelectObject(memDC, GetStockObject(NULL_PEN));
        Polygon(memDC, fillPts, ptCount);
        SelectObject(memDC, origBrush);
        SelectObject(memDC, origPen);
        DeleteObject(fillBrush);

        HPEN mainCurvePen = CreatePen(PS_SOLID, 2, RGB(55, 145, 185));
        SelectObject(memDC, mainCurvePen);
        for (int i = 1; i < ptCount - 1; ++i) {
            if (i == 1) MoveToEx(memDC, fillPts[i].x, fillPts[i].y, NULL);
            else LineTo(memDC, fillPts[i].x, fillPts[i].y);
        }
        SelectObject(memDC, origPen);
        DeleteObject(mainCurvePen);

        for (int i = 0; i < 3; ++i) {
            HPEN shadowPen = CreatePen(PS_SOLID, 1, RGB(10, 12, 16));
            SelectObject(memDC, shadowPen);
            SelectObject(memDC, GetStockObject(NULL_BRUSH));
            Ellipse(memDC, nodePts[i].x - 8, nodePts[i].y - 8, nodePts[i].x + 9, nodePts[i].y + 9);
            SelectObject(memDC, origPen);
            SelectObject(memDC, origBrush);
            DeleteObject(shadowPen);

            HBRUSH nb = CreateSolidBrush(bandColors[i]);
            HPEN np = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
            SelectObject(memDC, nb);
            SelectObject(memDC, np);
            Ellipse(memDC, nodePts[i].x - 6, nodePts[i].y - 6, nodePts[i].x + 7, nodePts[i].y + 7);
            SelectObject(memDC, origPen);
            SelectObject(memDC, origBrush);
            DeleteObject(np);
            DeleteObject(nb);
        }

        const char* bandNames[3] = { "LOW", "MID", "HIGH" };
        int colCenters[3] = { (int)(w * 0.22f), (int)(w * 0.50f), (int)(w * 0.78f) };

        for (int i = 0; i < 3; ++i) {
            float gainDb = (gains[i] - 0.5f) * 24.0f;
            char mainBuf[32], subBuf[32];
            snprintf(mainBuf, sizeof(mainBuf), "%s: %+.1fdB", bandNames[i], gainDb);

            float curF = get_eq_band_freq_hz(g_eqTrack, i);
            float curQ = get_eq_band_q_factor(g_eqTrack, i);
            if (curF >= 1000.0f) snprintf(subBuf, sizeof(subBuf), "Q: %.2f (%.1fkHz)", curQ, curF / 1000.0f);
            else snprintf(subBuf, sizeof(subBuf), "Q: %.2f (%.0fHz)", curQ, curF);

            SetTextColor(memDC, bandColors[i]);
            RECT mainRc = { colCenters[i] - 55, h - 38, colCenters[i] + 55, h - 22 };
            DrawTextA(memDC, mainBuf, -1, &mainRc, DT_CENTER | DT_SINGLELINE);

            SetTextColor(memDC, RGB(80, 95, 115));
            RECT subRc = { colCenters[i] - 55, h - 20, colCenters[i] + 55, h - 6 };
            DrawTextA(memDC, subBuf, -1, &subRc, DT_CENTER | DT_SINGLELINE);
        }

        BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, origBrush);
        SelectObject(memDC, origPen);
        SelectObject(memDC, oldBmp);
        DeleteObject(memBmp);
        DeleteDC(memDC);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        int mx = GET_X_LPARAM(lParam);
        int my = GET_Y_LPARAM(lParam);
        RECT rc;
        GetClientRect(hwnd, &rc);
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;

        int graphL = 52, graphR = w - 16, graphT = 16, graphB = h - 46;
        int graphW = graphR - graphL, graphH = graphB - graphT;
        int graphMidY = graphT + graphH / 2;
        float pxPerDb = (float)(graphH / 2 - 6) / 12.0f;

        float gains[3] = {
            g_Seq.trackEqLow[g_eqTrack],
            g_Seq.trackEqMid[g_eqTrack],
            g_Seq.trackEqHigh[g_eqTrack]
        };

        int bestBand = -1;
        float bestDistSq = 1e9f;

        for (int i = 0; i < 3; ++i) {
            float normX = g_Seq.trackEqFreq[g_eqTrack][i];
            int nx = graphL + (int)(normX * (float)graphW);
            int ny = graphMidY - (int)((gains[i] - 0.5f) * 24.0f * pxPerDb);

            float dx = (float)(mx - nx);
            float dy = (float)(my - ny);
            float dSq = dx * dx + dy * dy;

            if (dSq < bestDistSq) {
                bestDistSq = dSq;
                bestBand = i;
            }
        }

        if (bestBand >= 0 && (bestDistSq <= 600.0f || my < graphB)) {
            dragBand = bestBand;
        } else {
            if (mx < w * 0.35f) dragBand = 0;
            else if (mx < w * 0.65f) dragBand = 1;
            else dragBand = 2;
        }

        float* pGains[3] = { &g_Seq.trackEqLow[g_eqTrack], &g_Seq.trackEqMid[g_eqTrack], &g_Seq.trackEqHigh[g_eqTrack] };

        float newGain = 0.5f + (float)(graphMidY - my) / (24.0f * pxPerDb);
        if (newGain < 0.0f) newGain = 0.0f;
        if (newGain > 1.0f) newGain = 1.0f;
        *pGains[dragBand] = newGain;

        float newNormX = (float)(mx - graphL) / (float)graphW;
        if (newNormX < 0.0f) newNormX = 0.0f;
        if (newNormX > 1.0f) newNormX = 1.0f;
        g_Seq.trackEqFreq[g_eqTrack][dragBand] = newNormX;

        update_track_eq_params(g_eqTrack);
        SetCapture(hwnd);
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_MOUSEMOVE: {
        if (dragBand >= 0 && dragBand <= 2) {
            int mx = GET_X_LPARAM(lParam);
            int my = GET_Y_LPARAM(lParam);

            RECT rc;
            GetClientRect(hwnd, &rc);
            int w = rc.right - rc.left;
            int h = rc.bottom - rc.top;

            int graphL = 52, graphR = w - 16, graphT = 16, graphB = h - 46;
            int graphW = graphR - graphL, graphH = graphB - graphT;
            int graphMidY = graphT + graphH / 2;
            float pxPerDb = (float)(graphH / 2 - 6) / 12.0f;

            float* pGains[3] = {
                &g_Seq.trackEqLow[g_eqTrack],
                &g_Seq.trackEqMid[g_eqTrack],
                &g_Seq.trackEqHigh[g_eqTrack]
            };

            float newGain = 0.5f + (float)(graphMidY - my) / (24.0f * pxPerDb);
            if (newGain < 0.0f) newGain = 0.0f;
            if (newGain > 1.0f) newGain = 1.0f;
            *pGains[dragBand] = newGain;

            float newNormX = (float)(mx - graphL) / (float)graphW;
            if (newNormX < 0.0f) newNormX = 0.0f;
            if (newNormX > 1.0f) newNormX = 1.0f;
            g_Seq.trackEqFreq[g_eqTrack][dragBand] = newNormX;

            update_track_eq_params(g_eqTrack);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_MOUSEWHEEL: {
        short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
        int mx = GET_X_LPARAM(lParam);
        POINT pt = {mx, 0};
        ScreenToClient(hwnd, &pt);

        RECT rc;
        GetClientRect(hwnd, &rc);
        int w = rc.right - rc.left;

        int targetBand = 0;
        if (pt.x < w * 0.35f) targetBand = 0;
        else if (pt.x < w * 0.65f) targetBand = 1;
        else targetBand = 2;

        g_Seq.trackEqQ[g_eqTrack][targetBand] += (zDelta > 0) ? 0.05f : -0.05f;
        if (g_Seq.trackEqQ[g_eqTrack][targetBand] < 0.0f) g_Seq.trackEqQ[g_eqTrack][targetBand] = 0.0f;
        if (g_Seq.trackEqQ[g_eqTrack][targetBand] > 1.0f) g_Seq.trackEqQ[g_eqTrack][targetBand] = 1.0f;

        update_track_eq_params(g_eqTrack);
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_RBUTTONDOWN: {
        int mx = GET_X_LPARAM(lParam);
        RECT rc;
        GetClientRect(hwnd, &rc);
        int w = rc.right - rc.left;

        int targetBand = 0;
        if (mx < w * 0.35f) targetBand = 0;
        else if (mx < w * 0.65f) targetBand = 1;
        else targetBand = 2;

        float* pGains[3] = { &g_Seq.trackEqLow[g_eqTrack], &g_Seq.trackEqMid[g_eqTrack], &g_Seq.trackEqHigh[g_eqTrack] };
        *pGains[targetBand] = 0.5f;

        const float defFreqs[3] = { 0.25f, 0.50f, 0.85f };
        g_Seq.trackEqFreq[g_eqTrack][targetBand] = defFreqs[targetBand];
        g_Seq.trackEqQ[g_eqTrack][targetBand] = 0.50f;

        update_track_eq_params(g_eqTrack);
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_LBUTTONUP:
        if (dragBand >= 0) {
            dragBand = -1;
            ReleaseCapture();
        }
        return 0;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE || wParam == VK_RETURN) {
            ShowWindow(hwnd, SW_HIDE);
        }
        return 0;

    case WM_DESTROY:
        g_eqHwnd = NULL;
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static inline void open_track_eq_dialog(HWND parentHwnd, int trackIdx) {
    if (trackIdx < 0 || trackIdx >= g_Seq.trackCount || trackIdx >= MAX_TRACKS) return;
    g_eqTrack = trackIdx;

    if (!g_eqHwnd) {
        static bool s_eqRegistered = false;
        if (!s_eqRegistered) {
            WNDCLASSA wc = {0};
            wc.lpfnWndProc   = EqWndProc;
            wc.hInstance     = GetModuleHandle(NULL);
            wc.lpszClassName = "CSeqEqClass";
            wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
            RegisterClassA(&wc);
            s_eqRegistered = true;
        }

        int rw = 380, rh = 210;
        int scrW = GetSystemMetrics(SM_CXSCREEN);
        int scrH = GetSystemMetrics(SM_CYSCREEN);
        int rx = (scrW - rw) / 2;
        int ry = (scrH - rh) / 2;

        g_eqHwnd = CreateWindowExA(
            WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
            "CSeqEqClass",
            "Track EQ",
            WS_POPUPWINDOW | WS_CAPTION | WS_VISIBLE,
            rx, ry, rw, rh,
            parentHwnd, NULL, GetModuleHandle(NULL), NULL
        );
    }

    char titleBuf[64];
    snprintf(titleBuf, sizeof(titleBuf), "Track %d - Parametric EQ", trackIdx + 1);
    SetWindowTextA(g_eqHwnd, titleBuf);

    ShowWindow(g_eqHwnd, SW_SHOW);
    SetForegroundWindow(g_eqHwnd);
    InvalidateRect(g_eqHwnd, NULL, FALSE);
}

/* ============================================================
 * CONTEXT MENUS
 * ============================================================ */
static inline void show_clip_context_menu(HWND hwnd, int clipIdx, int screenX, int screenY) {
    if (clipIdx < 0 || clipIdx >= g_Seq.clipCount) return;
    HMENU hMenu = CreatePopupMenu();
    Clip *c = &g_Seq.clips[clipIdx];

    HMENU hRateMenu = CreatePopupMenu();
    AppendMenuA(hRateMenu, MF_STRING, ID_RATE_CUSTOM, "Custom...");
    AppendMenuA(hRateMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuA(hRateMenu, MF_STRING, ID_RATE_050,
        fabsf(c->playbackRate - 0.5000f) < 0.01f ? "[x] 0.50x (-1 Octave)" : "[ ] 0.50x (-1 Octave)");
    AppendMenuA(hRateMenu, MF_STRING, ID_RATE_075,
        fabsf(c->playbackRate - 0.7492f) < 0.01f ? "[x] 0.75x (-5 Semitones)" : "[ ] 0.75x (-5 Semitones)");
    AppendMenuA(hRateMenu, MF_STRING, ID_RATE_100,
        fabsf(c->playbackRate - 1.0000f) < 0.01f ? "[x] 1.00x (C)" : "[ ] 1.00x (C)");
    AppendMenuA(hRateMenu, MF_STRING, ID_RATE_125,
        fabsf(c->playbackRate - 1.3348f) < 0.01f ? "[x] 1.33x (+5 Semitones)" : "[ ] 1.33x (+5 Semitones)");
    AppendMenuA(hRateMenu, MF_STRING, ID_RATE_150,
        fabsf(c->playbackRate - 1.4983f) < 0.01f ? "[x] 1.50x (+7 Semitones)" : "[ ] 1.50x (+7 Semitones)");
    AppendMenuA(hRateMenu, MF_STRING, ID_RATE_200,
        fabsf(c->playbackRate - 2.0000f) < 0.01f ? "[x] 2.00x (+1 Octave)" : "[ ] 2.00x (+1 Octave)");
    AppendMenuA(hMenu, MF_POPUP, (UINT_PTR)hRateMenu, "Playback Rate");

    HMENU hFadeMenu = CreatePopupMenu();
    HMENU hFadeInMenu = CreatePopupMenu();
    AppendMenuA(hFadeInMenu, MF_STRING, ID_CLIP_FADE_IN_000,
                c->fadeInBeats <= 0.01f ? "[x] None" : "[ ] None");
    AppendMenuA(hFadeInMenu, MF_STRING, ID_CLIP_FADE_IN_025,
                fabsf(c->fadeInBeats - 0.125f) < 0.01f ? "[x] 1/8 Beat" : "[ ] 1/8 Beat");
    AppendMenuA(hFadeInMenu, MF_STRING, ID_CLIP_FADE_IN_050,
                fabsf(c->fadeInBeats - 0.25f) < 0.01f ? "[x] 1/4 Beat" : "[ ] 1/4 Beat");
    AppendMenuA(hFadeInMenu, MF_STRING, ID_CLIP_FADE_IN_100,
                fabsf(c->fadeInBeats - 0.5f) < 0.01f ? "[x] 1/2 Beat" : "[ ] 1/2 Beat");
    AppendMenuA(hFadeInMenu, MF_STRING, ID_CLIP_FADE_IN_150,
                fabsf(c->fadeInBeats - 1.0f) < 0.01f ? "[x] 1 Beat" : "[ ] 1 Beat");
    AppendMenuA(hFadeInMenu, MF_STRING, ID_CLIP_FADE_IN_200,
                fabsf(c->fadeInBeats - 2.0f) < 0.01f ? "[x] 2 Beats" : "[ ] 2 Beats");
    AppendMenuA(hFadeInMenu, MF_STRING, ID_CLIP_FADE_IN_300,
                fabsf(c->fadeInBeats - 4.0f) < 0.01f ? "[x] 4 Beats" : "[ ] 4 Beats");
    AppendMenuA(hFadeMenu, MF_POPUP, (UINT_PTR)hFadeInMenu, "Fade In");

    HMENU hFadeOutMenu = CreatePopupMenu();
    AppendMenuA(hFadeOutMenu, MF_STRING, ID_CLIP_FADE_OUT_000,
                c->fadeOutBeats <= 0.01f ? "[x] None" : "[ ] None");
    AppendMenuA(hFadeOutMenu, MF_STRING, ID_CLIP_FADE_OUT_025,
                fabsf(c->fadeOutBeats - 0.125f) < 0.01f ? "[x] 1/8 Beat" : "[ ] 1/8 Beat");
    AppendMenuA(hFadeOutMenu, MF_STRING, ID_CLIP_FADE_OUT_050,
                fabsf(c->fadeOutBeats - 0.25f) < 0.01f ? "[x] 1/4 Beat" : "[ ] 1/4 Beat");
    AppendMenuA(hFadeOutMenu, MF_STRING, ID_CLIP_FADE_OUT_100,
                fabsf(c->fadeOutBeats - 0.5f) < 0.01f ? "[x] 1/2 Beat" : "[ ] 1/2 Beat");
    AppendMenuA(hFadeOutMenu, MF_STRING, ID_CLIP_FADE_OUT_150,
                fabsf(c->fadeOutBeats - 1.0f) < 0.01f ? "[x] 1 Beat" : "[ ] 1 Beat");
    AppendMenuA(hFadeOutMenu, MF_STRING, ID_CLIP_FADE_OUT_200,
                fabsf(c->fadeOutBeats - 2.0f) < 0.01f ? "[x] 2 Beats" : "[ ] 2 Beats");
    AppendMenuA(hFadeOutMenu, MF_STRING, ID_CLIP_FADE_OUT_300,
                fabsf(c->fadeOutBeats - 4.0f) < 0.01f ? "[x] 4 Beats" : "[ ] 4 Beats");
    AppendMenuA(hFadeMenu, MF_POPUP, (UINT_PTR)hFadeOutMenu, "Fade Out");

    AppendMenuA(hFadeMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuA(hFadeMenu, MF_STRING, ID_CLIP_FADE_CLEAR, "Reset Fades");
    AppendMenuA(hMenu, MF_POPUP, (UINT_PTR)hFadeMenu, "Fade Envelope");

    AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuA(hMenu, MF_STRING, ID_VOL_RESET, "Reset Volume (100%)");
    AppendMenuA(hMenu, MF_STRING, ID_CLIP_DELETE, "Delete Selected (Del)");

    MENUINFO miNoCheck = {sizeof(MENUINFO)};
    miNoCheck.fMask = MIM_STYLE;
    miNoCheck.dwStyle = MNS_NOCHECK;
    SetMenuInfo(hMenu, &miNoCheck);
    SetMenuInfo(hRateMenu, &miNoCheck);
    SetMenuInfo(hFadeMenu, &miNoCheck);
    SetMenuInfo(hFadeInMenu, &miNoCheck);
    SetMenuInfo(hFadeOutMenu, &miNoCheck);

    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON, screenX, screenY, 0, hwnd, NULL);
    DestroyMenu(hMenu);

    if (cmd == ID_RATE_CUSTOM) {
        open_custom_rate_dialog(hwnd, clipIdx, -1);
    } else if (cmd == ID_RATE_050) {
        seq_lock(); for (int i = 0; i < g_Seq.clipCount; ++i) if (g_Seq.clips[i].isSelected) g_Seq.clips[i].playbackRate = 0.5000f; seq_unlock();
    } else if (cmd == ID_RATE_075) {
        seq_lock(); for (int i = 0; i < g_Seq.clipCount; ++i) if (g_Seq.clips[i].isSelected) g_Seq.clips[i].playbackRate = 0.7492f; seq_unlock();
    } else if (cmd == ID_RATE_100) {
        seq_lock(); for (int i = 0; i < g_Seq.clipCount; ++i) if (g_Seq.clips[i].isSelected) g_Seq.clips[i].playbackRate = 1.0000f; seq_unlock();
    } else if (cmd == ID_RATE_125) {
        seq_lock(); for (int i = 0; i < g_Seq.clipCount; ++i) if (g_Seq.clips[i].isSelected) g_Seq.clips[i].playbackRate = 1.3348f; seq_unlock();
    } else if (cmd == ID_RATE_150) {
        seq_lock(); for (int i = 0; i < g_Seq.clipCount; ++i) if (g_Seq.clips[i].isSelected) g_Seq.clips[i].playbackRate = 1.4983f; seq_unlock();
    } else if (cmd == ID_RATE_200) {
        seq_lock(); for (int i = 0; i < g_Seq.clipCount; ++i) if (g_Seq.clips[i].isSelected) g_Seq.clips[i].playbackRate = 2.0000f; seq_unlock();
    } else if (cmd >= ID_CLIP_FADE_IN_000 && cmd <= ID_CLIP_FADE_IN_300) {
        float inBeats[] = {0.0f, 0.125f, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f};
        int idx = cmd - ID_CLIP_FADE_IN_000;
        if (idx >= 0 && idx < 7) { seq_lock(); c->fadeInBeats = inBeats[idx]; seq_unlock(); }
    } else if (cmd >= ID_CLIP_FADE_OUT_000 && cmd <= ID_CLIP_FADE_OUT_300) {
        float outBeats[] = {0.0f, 0.125f, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f};
        int idx = cmd - ID_CLIP_FADE_OUT_000;
        if (idx >= 0 && idx < 7) { seq_lock(); c->fadeOutBeats = outBeats[idx]; seq_unlock(); }
    } else if (cmd == ID_CLIP_FADE_CLEAR) {
        seq_lock(); c->fadeInBeats = 0.0f; c->fadeOutBeats = 0.0f; seq_unlock();
    } else if (cmd == ID_VOL_RESET) {
        seq_lock(); c->volume = 1.0f; seq_unlock();
        g_Seq.volumePopupClip = clipIdx;
        g_Seq.volumePopupExpiry = GetTickCount() + 1200;
    } else if (cmd == ID_CLIP_DELETE) {
        delete_selected_clips();
    }
    InvalidateRect(hwnd, NULL, FALSE);
}

static inline void show_track_context_menu(HWND hwnd, int trackIdx, int screenX, int screenY) {
    if (trackIdx < 0 || trackIdx >= g_Seq.trackCount) return;
    HMENU hMenu = CreatePopupMenu();
    bool isMuted = g_Seq.trackMuted[trackIdx];

    AppendMenuA(hMenu, MF_STRING, ID_TRACK_EQ, "Parametric EQ...");
    AppendMenuA(hMenu, MF_STRING, ID_TRACK_MUTE, isMuted ? "[x] Mute Track" : "[ ] Mute Track");
    AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);

    HMENU hRateMenu = CreatePopupMenu();
    AppendMenuA(hRateMenu, MF_STRING, ID_TRACK_RATE_CUSTOM, "Custom...");
    AppendMenuA(hRateMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuA(hRateMenu, MF_STRING, ID_TRACK_RATE_050, "0.50x (-1 Octave)");
    AppendMenuA(hRateMenu, MF_STRING, ID_TRACK_RATE_075, "0.75x (-5 Semitones)");
    AppendMenuA(hRateMenu, MF_STRING, ID_TRACK_RATE_100, "1.00x (C)");
    AppendMenuA(hRateMenu, MF_STRING, ID_TRACK_RATE_125, "1.33x (+5 Semitones)");
    AppendMenuA(hRateMenu, MF_STRING, ID_TRACK_RATE_150, "1.50x (+7 Semitones)");
    AppendMenuA(hRateMenu, MF_STRING, ID_TRACK_RATE_200, "2.00x (+1 Octave)");
    AppendMenuA(hMenu, MF_POPUP, (UINT_PTR)hRateMenu, "Playback Rate");

    HMENU hBatchFadeMenu = CreatePopupMenu();

    HMENU hBatchFadeInMenu = CreatePopupMenu();
    AppendMenuA(hBatchFadeInMenu, MF_STRING, ID_TRACK_FADE_IN_000, "None");
    AppendMenuA(hBatchFadeInMenu, MF_STRING, ID_TRACK_FADE_IN_025, "1/8 Beat");
    AppendMenuA(hBatchFadeInMenu, MF_STRING, ID_TRACK_FADE_IN_050, "1/4 Beat");
    AppendMenuA(hBatchFadeInMenu, MF_STRING, ID_TRACK_FADE_IN_100, "1/2 Beat");
    AppendMenuA(hBatchFadeInMenu, MF_STRING, ID_TRACK_FADE_IN_150, "1 Beat");
    AppendMenuA(hBatchFadeInMenu, MF_STRING, ID_TRACK_FADE_IN_200, "2 Beats");
    AppendMenuA(hBatchFadeInMenu, MF_STRING, ID_TRACK_FADE_IN_400, "4 Beats");
    AppendMenuA(hBatchFadeMenu, MF_POPUP, (UINT_PTR)hBatchFadeInMenu, "Fade In");

    HMENU hBatchFadeOutMenu = CreatePopupMenu();
    AppendMenuA(hBatchFadeOutMenu, MF_STRING, ID_TRACK_FADE_OUT_000, "None");
    AppendMenuA(hBatchFadeOutMenu, MF_STRING, ID_TRACK_FADE_OUT_025, "1/8 Beat");
    AppendMenuA(hBatchFadeOutMenu, MF_STRING, ID_TRACK_FADE_OUT_050, "1/4 Beat");
    AppendMenuA(hBatchFadeOutMenu, MF_STRING, ID_TRACK_FADE_OUT_100, "1/2 Beat");
    AppendMenuA(hBatchFadeOutMenu, MF_STRING, ID_TRACK_FADE_OUT_150, "1 Beat");
    AppendMenuA(hBatchFadeOutMenu, MF_STRING, ID_TRACK_FADE_OUT_200, "2 Beats");
    AppendMenuA(hBatchFadeOutMenu, MF_STRING, ID_TRACK_FADE_OUT_400, "4 Beats");
    AppendMenuA(hBatchFadeMenu, MF_POPUP, (UINT_PTR)hBatchFadeOutMenu, "Fade Out");

    AppendMenuA(hBatchFadeMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuA(hBatchFadeMenu, MF_STRING, ID_TRACK_FADE_CLEAR, "Reset Fades");
    AppendMenuA(hMenu, MF_POPUP, (UINT_PTR)hBatchFadeMenu, "Fade Envelope");

    AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuA(hMenu, MF_STRING, ID_TRACK_CLEAR, "Clear Clips");

    MENUINFO miNoCheck = {sizeof(MENUINFO)};
    miNoCheck.fMask = MIM_STYLE;
    miNoCheck.dwStyle = MNS_NOCHECK;
    SetMenuInfo(hMenu, &miNoCheck);
    SetMenuInfo(hRateMenu, &miNoCheck);
    SetMenuInfo(hBatchFadeMenu, &miNoCheck);
    SetMenuInfo(hBatchFadeInMenu, &miNoCheck);
    SetMenuInfo(hBatchFadeOutMenu, &miNoCheck);

    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON, screenX, screenY, 0, hwnd, NULL);
    DestroyMenu(hMenu);

    float targetRate = 0.0f;
    if (cmd == ID_TRACK_RATE_CUSTOM) {
        open_custom_rate_dialog(hwnd, -1, trackIdx);
    } else if (cmd == ID_TRACK_RATE_050) targetRate = 0.5000f;
    else if (cmd == ID_TRACK_RATE_075) targetRate = 0.7492f;
    else if (cmd == ID_TRACK_RATE_100) targetRate = 1.0000f;
    else if (cmd == ID_TRACK_RATE_125) targetRate = 1.3348f;
    else if (cmd == ID_TRACK_RATE_150) targetRate = 1.4983f;
    else if (cmd == ID_TRACK_RATE_200) targetRate = 2.0000f;

    if (targetRate > 0.0f) {
        seq_lock();
        for (int i = 0; i < g_Seq.clipCount; ++i) {
            if (g_Seq.clips[i].track == trackIdx) g_Seq.clips[i].playbackRate = targetRate;
        }
        seq_unlock();
    }

    if (cmd == ID_TRACK_EQ) {
        open_track_eq_dialog(hwnd, trackIdx);
    } else if (cmd == ID_TRACK_MUTE) {
        seq_lock();
        g_Seq.trackMuted[trackIdx] = !g_Seq.trackMuted[trackIdx];
        seq_unlock();
    } else if (cmd >= ID_TRACK_FADE_IN_000 && cmd <= ID_TRACK_FADE_IN_400) {
        float inBeats[] = {0.0f, 0.125f, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f};
        int idx = cmd - ID_TRACK_FADE_IN_000;
        if (idx >= 0 && idx < 7) {
            seq_lock();
            for (int i = 0; i < g_Seq.clipCount; ++i) if (g_Seq.clips[i].track == trackIdx) g_Seq.clips[i].fadeInBeats = inBeats[idx];
            seq_unlock();
        }
    } else if (cmd >= ID_TRACK_FADE_OUT_000 && cmd <= ID_TRACK_FADE_OUT_400) {
        float outBeats[] = {0.0f, 0.125f, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f};
        int idx = cmd - ID_TRACK_FADE_OUT_000;
        if (idx >= 0 && idx < 7) {
            seq_lock();
            for (int i = 0; i < g_Seq.clipCount; ++i) if (g_Seq.clips[i].track == trackIdx) g_Seq.clips[i].fadeOutBeats = outBeats[idx];
            seq_unlock();
        }
    } else if (cmd == ID_TRACK_FADE_CLEAR) {
        seq_lock();
        for (int i = 0; i < g_Seq.clipCount; ++i) {
            if (g_Seq.clips[i].track == trackIdx) { g_Seq.clips[i].fadeInBeats = 0.0f; g_Seq.clips[i].fadeOutBeats = 0.0f; }
        }
        seq_unlock();
    } else if (cmd == ID_TRACK_CLEAR) {
        seq_lock();
        for (int i = 0; i < g_Seq.clipCount;) {
            if (g_Seq.clips[i].track == trackIdx) {
                for (int j = i; j < g_Seq.clipCount - 1; ++j) g_Seq.clips[j] = g_Seq.clips[j + 1];
                g_Seq.clipCount--;
            } else i++;
        }
        seq_unlock();
    }
    InvalidateRect(hwnd, NULL, FALSE);
}