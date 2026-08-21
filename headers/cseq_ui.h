#pragma once
#include "cseq_globals.h"
#include "cseq_dsp.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#pragma comment(lib, "msimg32.lib")

/* ============================================================
 * FAKE ATTRACTOR VISUALIZER
 * ============================================================ */
static POINT g_fakePts[FAKE_ATTRACTOR_POINTS];

static inline void update_fake_attractor(int cx, int cy, float scale, float t) {
    for (int i = 0; i < FAKE_ATTRACTOR_POINTS; ++i) {
        float p = (float)i / (float)(FAKE_ATTRACTOR_POINTS - 1);
        float a = t * 0.62f + p * 6.283185f * 1.75f;
        float b = t * 0.91f + p * 6.283185f * 2.35f;

        float x = sinf(a) * (1.05f + 0.32f * cosf(b * 1.35f));
        float y = cosf(a * 0.88f) * sinf(b) * 0.92f + 0.22f * sinf(a * 2.05f + b * 0.7f);

        g_fakePts[i].x = cx + (int)(x * scale);
        g_fakePts[i].y = cy + (int)(y * scale * 0.72f);
    }
}

static inline void draw_fake_attractor(HDC hdc, int cx, int cy, float scale) {
    float t = (float)GetTickCount() * 0.00115f;
    update_fake_attractor(cx, cy, scale, t);

    HPEN pen = CreatePen(PS_SOLID, 1, RGB(140, 200, 255));
    HGDIOBJ old = SelectObject(hdc, pen);
    Polyline(hdc, g_fakePts, FAKE_ATTRACTOR_POINTS);
    SelectObject(hdc, old);
    DeleteObject(pen);
}

/* ============================================================
 * COLOR UTILITIES & THEMES
 * ============================================================ */
static inline COLORREF hsl_to_rgb(float h, float s, float l) {
    float c = (1.0f - fabsf(2.0f * l - 1.0f)) * s;
    float x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
    float m = l - c / 2.0f;
    float r = 0.0f, g = 0.0f, b = 0.0f;

    if (h < 60.0f) {
        r = c; g = x; b = 0.0f;
    } else if (h < 120.0f) {
        r = x; g = c; b = 0.0f;
    } else if (h < 180.0f) {
        r = 0.0f; g = c; b = x;
    } else if (h < 240.0f) {
        r = 0.0f; g = x; b = c;
    } else if (h < 300.0f) {
        r = x; g = 0.0f; b = c;
    } else {
        r = c; g = 0.0f; b = x;
    }

    return RGB((BYTE)((r + m) * 255.0f), (BYTE)((g + m) * 255.0f), (BYTE)((b + m) * 255.0f));
}

static inline COLORREF get_volume_gradient_color(float vol) {
    float norm = vol / 1.5f;
    if (norm < 0.0f) norm = 0.0f;
    if (norm > 1.0f) norm = 1.0f;
    float hue = norm * 135.0f;
    return hsl_to_rgb(hue, 0.85f, 0.52f);
}

static inline void init_track_theme(int trackIdx) {
    if (trackIdx < 0 || trackIdx >= MAX_TRACKS) return;
    static float baseSeed = 0.0f;
    if (trackIdx == 0 && baseSeed == 0.0f) {
        baseSeed = (float)(rand() % 360);
    }

    float hue = fmodf(baseSeed + (float)trackIdx * 137.508f, 360.0f);
    g_Seq.trackThemes[trackIdx].waveColor = hsl_to_rgb(hue, 0.45f, 0.60f);
    g_Seq.trackThemes[trackIdx].selectWaveColor = hsl_to_rgb(hue, 0.60f, 0.80f);
    g_Seq.trackThemes[trackIdx].bgColor = hsl_to_rgb(hue, 0.20f, 0.15f);
    g_Seq.trackThemes[trackIdx].selectBgColor = hsl_to_rgb(hue, 0.30f, 0.22f);
    g_Seq.trackThemes[trackIdx].borderColor = hsl_to_rgb(hue, 0.35f, 0.38f);
}

/* ============================================================
 * TOPBAR LAYOUT & SLOTS
 * ============================================================ */
#define TOPBAR_SLOT_COUNT 12
#define TOPBAR_START_X 12
#define TOPBAR_SLOT_GAP 6

static const char *kSlotMaxLabels[TOPBAR_SLOT_COUNT] = {
    "PAUSE",
    "300 BPM",
    "32 BARS",
    "SWING 100%",
    "SNAP 1/16",
    "FROM CURSOR",
    "LO-FI OFF",
    "IMPORT",
    "EXPORT",
    "SAVE",
    "LOAD",
    "KEYBINDS"
};

static inline void get_topbar_slot_bounds(HDC hdc, int slotIdx, int *outX, int *outW) {
    HDC useDC = hdc;
    bool releaseDC = false;
    if (!useDC) {
        useDC = GetDC(NULL);
        releaseDC = true;
    }

    SIZE szBracket;
    GetTextExtentPoint32A(useDC, "[", 1, &szBracket);

    int curX = TOPBAR_START_X;
    for (int i = 0; i < TOPBAR_SLOT_COUNT; ++i) {
        SIZE szText;
        GetTextExtentPoint32A(useDC, kSlotMaxLabels[i], (int)strlen(kSlotMaxLabels[i]), &szText);
        int slotWidth = szText.cx + (szBracket.cx * 2) + 10;

        if (i == slotIdx) {
            if (outX) *outX = curX;
            if (outW) *outW = slotWidth;
            break;
        }
        curX += slotWidth + TOPBAR_SLOT_GAP;
    }

    if (releaseDC) {
        ReleaseDC(NULL, useDC);
    }
}

static inline void draw_fixed_badge(HDC hdc, int slotIdx, int y, const char *text, COLORREF textCol) {
    int x = 0, w = 0;
    get_topbar_slot_bounds(hdc, slotIdx, &x, &w);

    SetBkMode(hdc, TRANSPARENT);
    SetTextCharacterExtra(hdc, 0);

    SIZE szBracket;
    GetTextExtentPoint32A(hdc, "[", 1, &szBracket);

    SetTextColor(hdc, RGB(70, 78, 92));
    TextOutA(hdc, x, y + 1, "[", 1);
    TextOutA(hdc, x + w - szBracket.cx, y + 1, "]", 1);

    SetTextColor(hdc, textCol);
    RECT textRect = {
        x + szBracket.cx,
        y + 1,
        x + w - szBracket.cx,
        y + 21
    };
    DrawTextA(hdc, text, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
}

static inline void draw_alpha_box(HDC hdc, int x, int y, int w, int h, COLORREF fillColor, BYTE alpha, COLORREF borderColor) {
    if (w <= 0 || h <= 0) return;
    HDC memDC = CreateCompatibleDC(hdc);
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void *pBits = NULL;
    HBITMAP hBmp = CreateDIBSection(memDC, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    if (hBmp && pBits) {
        HGDIOBJ oldBmp = SelectObject(memDC, hBmp);
        BYTE r = GetRValue(fillColor);
        BYTE g = GetGValue(fillColor);
        BYTE b = GetBValue(fillColor);
        BYTE pr = (BYTE)((r * alpha) / 255);
        BYTE pg = (BYTE)((g * alpha) / 255);
        BYTE pb = (BYTE)((b * alpha) / 255);
        DWORD pixel = ((DWORD)alpha << 24) | ((DWORD)pr << 16) | ((DWORD)pg << 8) | (DWORD)pb;
        DWORD *pPix = (DWORD *)pBits;
        int count = w * h;
        for (int i = 0; i < count; ++i) pPix[i] = pixel;

        BLENDFUNCTION bf = {AC_SRC_OVER, 0, alpha, AC_SRC_ALPHA};
        AlphaBlend(hdc, x, y, w, h, memDC, 0, 0, w, h, bf);

        SelectObject(memDC, oldBmp);
        DeleteObject(hBmp);
    }
    DeleteDC(memDC);

    HPEN borderPen = CreatePen(PS_DOT, 1, borderColor);
    HGDIOBJ oldPen = SelectObject(hdc, borderPen);
    HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, x, y, x + w, y + h);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(borderPen);
}

/* ============================================================
 * WAVEFORM RENDERING
 * ============================================================ */
static inline void draw_waveform_clip(HDC hdc, const Clip *clip, const RECT *rect, bool isHovered, bool isOverlapped) {
    if (!clip || clip->sampleIndex < 0 || clip->sampleIndex >= g_Seq.sampleCount) return;
    const AudioSample *s = &g_Seq.samples[clip->sampleIndex];
    if (!s->loaded || !s->pFrames || s->frameCount == 0) return;

    int clipWidth = rect->right - rect->left;
    int clipHeight = rect->bottom - rect->top;
    if (clipWidth <= 0 || clipHeight <= 0) return;

    int midY = rect->top + clipHeight / 2;

    int tIdx = clip->track >= 0 && clip->track < MAX_TRACKS ? clip->track : 0;
    TrackTheme *theme = &g_Seq.trackThemes[tIdx];
    bool isMuted = (clip->track >= 0 && clip->track < g_Seq.trackCount) ? g_Seq.trackMuted[clip->track] : false;

    COLORREF fillCol = isMuted ? RGB(28, 30, 36) : (clip->isSelected ? theme->selectBgColor : theme->bgColor);
    COLORREF borderCol = isMuted ? RGB(45, 48, 55) : (clip->isSelected ? RGB(255, 255, 255) : theme->borderColor);
    COLORREF waveCol = isMuted ? RGB(70, 75, 85) : (clip->isSelected ? theme->selectWaveColor : theme->waveColor);

    if (isOverlapped && !isHovered && !clip->isSelected) {
        fillCol = RGB(GetRValue(fillCol) / 2, GetGValue(fillCol) / 2, GetBValue(fillCol) / 2);
        waveCol = RGB(GetRValue(waveCol) * 2 / 3, GetGValue(waveCol) * 2 / 3, GetBValue(waveCol) * 2 / 3);
    } else if (isHovered && !clip->isSelected) {
        borderCol = RGB(120, 155, 185);
    }

    float edgeAlpha = 1.0f;
    if (rect->left < TRACK_HEADER_WIDTH) {
        const int kEdgeFadeWidth = 32;
        int visibleWidth = rect->right - TRACK_HEADER_WIDTH;
        if (visibleWidth < kEdgeFadeWidth && visibleWidth > 0) {
            edgeAlpha = (float)visibleWidth / (float)kEdgeFadeWidth;
            if (edgeAlpha < 0.3f) edgeAlpha = 0.3f;
        }
    }

    if (edgeAlpha < 0.99f) {
        fillCol = RGB((BYTE)(GetRValue(fillCol) * edgeAlpha),
                      (BYTE)(GetGValue(fillCol) * edgeAlpha),
                      (BYTE)(GetBValue(fillCol) * edgeAlpha));
        borderCol = RGB((BYTE)(GetRValue(borderCol) * edgeAlpha),
                        (BYTE)(GetGValue(borderCol) * edgeAlpha),
                        (BYTE)(GetBValue(borderCol) * edgeAlpha));
        waveCol = RGB((BYTE)(GetRValue(waveCol) * edgeAlpha),
                      (BYTE)(GetGValue(waveCol) * edgeAlpha),
                      (BYTE)(GetBValue(waveCol) * edgeAlpha));
    }

    HBRUSH bgBrush = CreateSolidBrush(fillCol);
    HPEN borderPen = CreatePen(PS_SOLID, 1, borderCol);
    HGDIOBJ oldBrush = SelectObject(hdc, bgBrush);
    HGDIOBJ oldPen = SelectObject(hdc, borderPen);

    Rectangle(hdc, rect->left, rect->top, rect->right, rect->bottom);

    SetBkMode(hdc, TRANSPARENT);
    COLORREF textBase = clip->isSelected ? RGB(255, 255, 255) : (isMuted ? RGB(120, 125, 135) : RGB(210, 218, 228));
    SetTextColor(hdc, RGB((BYTE)(GetRValue(textBase) * edgeAlpha),
                          (BYTE)(GetGValue(textBase) * edgeAlpha),
                          (BYTE)(GetBValue(textBase) * edgeAlpha)));

    char titleBuf[128];
    if (fabsf(clip->playbackRate - 1.0f) > 0.01f) {
        snprintf(titleBuf, sizeof(titleBuf), "%s (%.2fx)", s->name, clip->playbackRate);
    } else {
        snprintf(titleBuf, sizeof(titleBuf), "%s", s->name);
    }

    RECT textRect = {
        rect->left + 6,
        rect->top + 3,
        rect->right - 4,
        rect->top + 18
    };
    DrawTextA(hdc, titleBuf, -1, &textRect, DT_SINGLELINE | DT_LEFT | DT_END_ELLIPSIS);

    HPEN wavePen = CreatePen(PS_SOLID, 1, waveCol);
    SelectObject(hdc, wavePen);

    float halfHeight = (float)(clipHeight / 2 - 4);
    if (halfHeight < 2.0f) halfHeight = 2.0f;
    float volScale = (clip->volume > 1.5f) ? 1.5f : clip->volume;
    float fpb = frames_per_beat(g_Seq.bpm);
    float pRate = (clip->playbackRate > 0.01f) ? clip->playbackRate : 1.0f;
    float ppb = get_pixels_per_beat();

    double framesPerPixel = ((double)fpb * (double)pRate) / (double)ppb;
    if (framesPerPixel < 1.0) framesPerPixel = 1.0;

    ma_uint64 framesPerPeak = s->frameCount / PEAK_CACHE_SIZE;
    if (framesPerPeak == 0) framesPerPeak = 1;

    for (int x = 0; x < clipWidth; ++x) {
        int screenX = rect->left + x;
        if (screenX < TRACK_HEADER_WIDTH) continue;

        ma_uint64 fStart = clip->sampleOffsetFrames + (ma_uint64)((double)x * framesPerPixel);
        ma_uint64 fEnd = fStart + (ma_uint64)framesPerPixel;
        if (fStart >= s->frameCount) break;
        if (fEnd > s->frameCount) fEnd = s->frameCount;

        float minV = 0.0f, maxV = 0.0f;

        if (framesPerPixel < 2048 && s->pFrames) {
            ma_uint64 step = (fEnd > fStart + 32) ? ((fEnd - fStart) / 32) : 1;
            for (ma_uint64 f = fStart; f < fEnd; f += step) {
                float mono = (s->pFrames[f * 2 + 0] + s->pFrames[f * 2 + 1]) * 0.5f;
                if (mono < minV) minV = mono;
                if (mono > maxV) maxV = mono;
            }
        } else {
            int pStart = (int)(fStart / framesPerPeak);
            int pEnd = (int)(fEnd / framesPerPeak);
            if (pStart >= PEAK_CACHE_SIZE) pStart = PEAK_CACHE_SIZE - 1;
            if (pEnd >= PEAK_CACHE_SIZE) pEnd = PEAK_CACHE_SIZE - 1;

            if (pStart <= pEnd) {
                minV = s->peaks[pStart].min;
                maxV = s->peaks[pStart].max;
                for (int p = pStart + 1; p <= pEnd; ++p) {
                    if (s->peaks[p].min < minV) minV = s->peaks[p].min;
                    if (s->peaks[p].max > maxV) maxV = s->peaks[p].max;
                }
            }
        }

        float curBeatOffset = (float)x / ppb;
        float fadeMultiplier = 1.0f;
        if (clip->fadeInBeats > 0.001f && curBeatOffset < clip->fadeInBeats) {
            fadeMultiplier = curBeatOffset / clip->fadeInBeats;
        } else if (clip->fadeOutBeats > 0.001f && (clip->lengthBeats - curBeatOffset) < clip->fadeOutBeats) {
            fadeMultiplier = (clip->lengthBeats - curBeatOffset) / clip->fadeOutBeats;
            if (fadeMultiplier < 0.0f) fadeMultiplier = 0.0f;
        }

        minV *= (volScale * fadeMultiplier);
        maxV *= (volScale * fadeMultiplier);
        if (minV < -1.0f) minV = -1.0f;
        if (maxV > 1.0f) maxV = 1.0f;

        int y1 = midY + (int)(minV * halfHeight);
        int y2 = midY + (int)(maxV * halfHeight);
        if (y1 == y2) y2 = y1 + 1;

        MoveToEx(hdc, screenX, y1, NULL);
        LineTo(hdc, screenX, y2);
    }

    SelectObject(hdc, oldPen);
    DeleteObject(wavePen);

    BYTE r = GetRValue(waveCol);
    BYTE g = GetGValue(waveCol);
    BYTE b = GetBValue(waveCol);
    HPEN fadePen = CreatePen(PS_DOT, 1, RGB(r / 2 + 60, g / 2 + 60, b / 2 + 60));
    HGDIOBJ oldFadeP = SelectObject(hdc, fadePen);

    int inW = (int)(clip->fadeInBeats * ppb);
    if (inW > clipWidth) inW = clipWidth;
    int inApexX = rect->left + inW;
    int inApexY = rect->top + 3;

    MoveToEx(hdc, rect->left, rect->bottom - 2, NULL);
    LineTo(hdc, inApexX, inApexY);

    int outW = (int)(clip->fadeOutBeats * ppb);
    if (outW > clipWidth) outW = clipWidth;
    int outApexX = rect->right - outW;
    int outApexY = rect->top + 3;

    MoveToEx(hdc, outApexX, outApexY, NULL);
    LineTo(hdc, rect->right, rect->bottom - 2);

    SelectObject(hdc, oldFadeP);
    DeleteObject(fadePen);

    int clipIndex = (int)(clip - g_Seq.clips);
    bool isBeingDragged = (g_Seq.draggedClipIndex == clipIndex);
    bool isAnyFadeDragging = (g_Seq.isFadeInDragging || g_Seq.isFadeOutDragging);

    bool mouseNearIn = (abs(g_Seq.mouseX - inApexX) <= 16 && g_Seq.mouseY >= rect->top - 2 && g_Seq.mouseY <= rect->top + 28);
    bool mouseNearOut = (abs(g_Seq.mouseX - outApexX) <= 16 && g_Seq.mouseY >= rect->top - 2 && g_Seq.mouseY <= rect->top + 28);

    bool showInHandle = false;
    bool showOutHandle = false;

    if (isAnyFadeDragging) {
        if (isBeingDragged) {
            showInHandle = g_Seq.isFadeInDragging;
            showOutHandle = g_Seq.isFadeOutDragging;
        }
    } else if (isHovered || mouseNearIn || mouseNearOut) {
        showInHandle = mouseNearIn || (isHovered && clip->fadeInBeats > 0.001f);
        showOutHandle = mouseNearOut || (isHovered && clip->fadeOutBeats > 0.001f);
    }

    if (showInHandle) {
        HBRUSH nb = CreateSolidBrush(RGB(255, 255, 255));
        HPEN np = CreatePen(PS_SOLID, 1, RGB(35, 42, 54));
        HGDIOBJ oldB = SelectObject(hdc, nb);
        HGDIOBJ oldP = SelectObject(hdc, np);
        Ellipse(hdc, inApexX - 4, inApexY - 4, inApexX + 5, inApexY + 5);
        SelectObject(hdc, oldP);
        SelectObject(hdc, oldB);
        DeleteObject(np);
        DeleteObject(nb);
    }

    if (showOutHandle) {
        HBRUSH nb = CreateSolidBrush(RGB(255, 255, 255));
        HPEN np = CreatePen(PS_SOLID, 1, RGB(35, 42, 54));
        HGDIOBJ oldB = SelectObject(hdc, nb);
        HGDIOBJ oldP = SelectObject(hdc, np);
        Ellipse(hdc, outApexX - 4, outApexY - 4, outApexX + 5, outApexY + 5);
        SelectObject(hdc, oldP);
        SelectObject(hdc, oldB);
        DeleteObject(np);
        DeleteObject(nb);
    }

    SelectObject(hdc, oldBrush);
    DeleteObject(bgBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(borderPen);
}

/* ============================================================
 * TIMELINE CACHE & STATE DETECTOR
 * ============================================================ */
static HDC g_cacheDC = NULL;
static HBITMAP g_cacheBmp = NULL;
static HBITMAP g_cacheOldBmp = NULL;
static int g_cacheW = 0;
static int g_cacheH = 0;
static bool g_timelineDirty = true;

static inline DWORD hash_dword(DWORD hash, DWORD val) {
    return (hash ^ val) * 16777619u;
}

static inline DWORD hash_float(DWORD hash, float val) {
    DWORD u = 0;
    memcpy(&u, &val, sizeof(float));
    return (hash ^ u) * 16777619u;
}

static inline bool is_timeline_dirty(int w, int h) {
    static DWORD s_lastHash = 0;
    static int s_lastW = 0, s_lastH = 0;

    if (w != s_lastW || h != s_lastH || g_timelineDirty) {
        s_lastW = w;
        s_lastH = h;
        return true;
    }

    DWORD hsh = 2166136261u;

    seq_lock();
    hsh = hash_dword(hsh, (DWORD)g_Seq.scrollX);
    hsh = hash_dword(hsh, (DWORD)g_Seq.scrollY);
    hsh = hash_float(hsh, g_Seq.zoom);
    hsh = hash_dword(hsh, (DWORD)g_Seq.barCount);
    hsh = hash_dword(hsh, (DWORD)g_Seq.trackCount);
    hsh = hash_dword(hsh, (DWORD)g_Seq.hoveredClip);
    hsh = hash_dword(hsh, (DWORD)g_Seq.clipCount);

    for (int t = 0; t < g_Seq.trackCount && t < MAX_TRACKS; ++t) {
        hsh = hash_dword(hsh, g_Seq.trackMuted[t] ? 1 : 0);
        hsh = hash_float(hsh, g_Seq.trackVolume[t]);
    }

    for (int i = 0; i < g_Seq.clipCount && i < MAX_CLIPS; ++i) {
        const Clip *c = &g_Seq.clips[i];
        hsh = hash_dword(hsh, (DWORD)c->sampleIndex);
        hsh = hash_dword(hsh, (DWORD)c->track);
        hsh = hash_float(hsh, c->startBeat);
        hsh = hash_float(hsh, c->lengthBeats);
        hsh = hash_dword(hsh, (DWORD)c->sampleOffsetFrames);
        hsh = hash_float(hsh, c->volume);
        hsh = hash_float(hsh, c->playbackRate);
        hsh = hash_float(hsh, c->fadeInBeats);
        hsh = hash_float(hsh, c->fadeOutBeats);
        hsh = hash_dword(hsh, c->isSelected ? 1 : 0);
    }
    seq_unlock();

    if (hsh != s_lastHash) {
        s_lastHash = hsh;
        return true;
    }
    return false;
}

/* ============================================================
 * UPDATE TIMELINE CACHE
 * ============================================================ */
static inline void update_timeline_cache(HDC hdc, int w, int h) {
    if (!g_cacheDC) {
        g_cacheDC = CreateCompatibleDC(hdc);
    }

    if (g_cacheW != w || g_cacheH != h || !g_cacheBmp) {
        if (g_cacheBmp) {
            SelectObject(g_cacheDC, g_cacheOldBmp);
            DeleteObject(g_cacheBmp);
        }
        g_cacheBmp = CreateCompatibleBitmap(hdc, w, h);
        g_cacheOldBmp = (HBITMAP)SelectObject(g_cacheDC, g_cacheBmp);
        g_cacheW = w;
        g_cacheH = h;
    }

    RECT rc = {0, 0, w, h};
    HBRUSH bgBrush = CreateSolidBrush(RGB(17, 19, 23));
    FillRect(g_cacheDC, &rc, bgBrush);
    DeleteObject(bgBrush);

    int viewportTop = HEADER_HEIGHT;
    int viewportBottom = h - BOTTOM_DOCK_HEIGHT;
    int trackAreaEndY = HEADER_HEIGHT - g_Seq.scrollY + g_Seq.trackCount * TRACK_HEIGHT;
    float ppb = get_pixels_per_beat();

    HRGN timelineClip = CreateRectRgn(TRACK_HEADER_WIDTH, viewportTop, w, viewportBottom);
    SelectClipRgn(g_cacheDC, timelineClip);

    HPEN sixteenthPen = CreatePen(PS_SOLID, 1, RGB(25, 28, 35));
    HPEN beatPen = CreatePen(PS_SOLID, 1, RGB(38, 43, 53));
    HPEN barPen = CreatePen(PS_SOLID, 1, RGB(60, 68, 85));
    HGDIOBJ origPen = SelectObject(g_cacheDC, sixteenthPen);

    int total16thSteps = (int)(total_beats() * 4.0f);
    int gridTop = viewportTop;
    int gridBottom = min(viewportBottom, trackAreaEndY);

    if (gridBottom > gridTop) {
        for (int s = 0; s <= total16thSteps; ++s) {
            float beat = (float)s * 0.25f;
            int gridX = TRACK_HEADER_WIDTH - g_Seq.scrollX + (int)(beat * ppb);
            if (gridX < TRACK_HEADER_WIDTH || gridX > w) continue;

            if (s % 16 == 0) SelectObject(g_cacheDC, barPen);
            else if (s % 4 == 0) SelectObject(g_cacheDC, beatPen);
            else SelectObject(g_cacheDC, sixteenthPen);

            MoveToEx(g_cacheDC, gridX, gridTop, NULL);
            LineTo(g_cacheDC, gridX, gridBottom);
        }
    }

    SelectObject(g_cacheDC, origPen);
    DeleteObject(sixteenthPen);
    DeleteObject(beatPen);
    DeleteObject(barPen);

    seq_lock();
    for (int i = 0; i < g_Seq.clipCount; ++i) {
        Clip *c = &g_Seq.clips[i];
        if (c->track >= g_Seq.trackCount || c->startBeat >= total_beats()) continue;
        if (c->sampleIndex < 0 || c->sampleIndex >= g_Seq.sampleCount) continue;

        bool isHovered = (!g_Seq.isDraggingClip && !g_Seq.isMarqueeSelecting && g_Seq.hoveredClip == i);

        bool isOverlapped = false;
        for (int j = i + 1; j < g_Seq.clipCount; ++j) {
            if (g_Seq.clips[j].track == c->track && g_Seq.clips[j].startBeat < total_beats()) {
                float s1 = c->startBeat, e1 = c->startBeat + c->lengthBeats;
                float s2 = g_Seq.clips[j].startBeat, e2 = g_Seq.clips[j].startBeat + g_Seq.clips[j].lengthBeats;
                if (max(s1, s2) < min(e1, e2)) {
                    isOverlapped = true;
                    break;
                }
            }
        }

        int clipX1 = TRACK_HEADER_WIDTH - g_Seq.scrollX + (int)(c->startBeat * ppb);
        float visibleLen = c->lengthBeats;
        if (c->startBeat + visibleLen > total_beats()) visibleLen = total_beats() - c->startBeat;
        if (visibleLen <= 0.0f) continue;
        int clipX2 = clipX1 + (int)(visibleLen * ppb);

        int clipY1 = HEADER_HEIGHT - g_Seq.scrollY + c->track * TRACK_HEIGHT;
        int clipY2 = clipY1 + TRACK_HEIGHT;

        if (clipY2 <= viewportTop || clipY1 >= viewportBottom) continue;
        if (clipX2 <= TRACK_HEADER_WIDTH || clipX1 >= w) continue;

        RECT clipRect = {clipX1, clipY1, clipX2, clipY2};
        draw_waveform_clip(g_cacheDC, c, &clipRect, isHovered, isOverlapped);
    }
    seq_unlock();

    SelectClipRgn(g_cacheDC, NULL);
    DeleteObject(timelineClip);

    HPEN trackDivPen = CreatePen(PS_SOLID, 1, RGB(24, 27, 34));
    HGDIOBJ oldDivPen = SelectObject(g_cacheDC, trackDivPen);

    for (int t = 0; t < g_Seq.trackCount && t < MAX_TRACKS; ++t) {
        int trackY = HEADER_HEIGHT - g_Seq.scrollY + t * TRACK_HEIGHT;
        if (trackY + TRACK_HEIGHT <= viewportTop || trackY >= viewportBottom) continue;

        bool isMuted = g_Seq.trackMuted[t];
        RECT thRect = {0, trackY, TRACK_HEADER_WIDTH, trackY + TRACK_HEIGHT};
        HBRUSH thBrush = CreateSolidBrush(isMuted ? RGB(36, 22, 22) : RGB(22, 25, 30));
        FillRect(g_cacheDC, &thRect, thBrush);
        DeleteObject(thBrush);

        float tVol = (g_Seq.trackVolume[t] > 1.0f) ? 1.0f : g_Seq.trackVolume[t];
        if (tVol < 0.0f) tVol = 0.0f;
        int barHeight = (int)(tVol * (float)TRACK_HEIGHT);

        RECT colBar = {0, trackY + (TRACK_HEIGHT - barHeight), 5, trackY + TRACK_HEIGHT};
        HBRUSH colBrush = CreateSolidBrush(isMuted ? RGB(200, 50, 50) : g_Seq.trackThemes[t % MAX_TRACKS].waveColor);
        FillRect(g_cacheDC, &colBar, colBrush);
        DeleteObject(colBrush);

        char trackName[32];
        snprintf(trackName, sizeof(trackName), "Track %d", t + 1);
        SetBkMode(g_cacheDC, TRANSPARENT);
        SetTextColor(g_cacheDC, isMuted ? RGB(220, 90, 90) : RGB(165, 175, 190));
        TextOutA(g_cacheDC, 10, trackY + TRACK_HEIGHT / 2 - 14, trackName, (int)strlen(trackName));

        if (isMuted) {
            RECT muteRect = {10, trackY + TRACK_HEIGHT / 2 + 2, TRACK_HEADER_WIDTH - 12, trackY + TRACK_HEIGHT / 2 + 18};
            HBRUSH muteBg = CreateSolidBrush(RGB(180, 40, 40));
            FillRect(g_cacheDC, &muteRect, muteBg);
            DeleteObject(muteBg);
            SetTextColor(g_cacheDC, RGB(255, 255, 255));
            DrawTextA(g_cacheDC, "MUTED", -1, &muteRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        } else {
            char volBuf[16];
            snprintf(volBuf, sizeof(volBuf), "%d%%", (int)(g_Seq.trackVolume[t] * 100.0f + 0.5f));
            SetTextColor(g_cacheDC, RGB(95, 105, 120));
            TextOutA(g_cacheDC, 10, trackY + TRACK_HEIGHT / 2 + 2, volBuf, (int)strlen(volBuf));
        }

        MoveToEx(g_cacheDC, 0, trackY + TRACK_HEIGHT, NULL);
        LineTo(g_cacheDC, w, trackY + TRACK_HEIGHT);
    }

    SelectObject(g_cacheDC, oldDivPen);
    DeleteObject(trackDivPen);

    RECT headerRect = {0, 0, w, HEADER_HEIGHT};
    HBRUSH headerBrush = CreateSolidBrush(RGB(24, 27, 34));
    FillRect(g_cacheDC, &headerRect, headerBrush);
    DeleteObject(headerBrush);

    HPEN headerSepPen = CreatePen(PS_SOLID, 1, RGB(42, 48, 60));
    HGDIOBJ oldHdrPen = SelectObject(g_cacheDC, headerSepPen);
    MoveToEx(g_cacheDC, 0, HEADER_HEIGHT - 1, NULL);
    LineTo(g_cacheDC, w, HEADER_HEIGHT - 1);
    SelectObject(g_cacheDC, oldHdrPen);
    DeleteObject(headerSepPen);

    float secondsPerBeat = 60.0f / (g_Seq.bpm > 1.0f ? g_Seq.bpm : 1.0f);
    for (int b = 0; b <= g_Seq.barCount; ++b) {
        int barX = TRACK_HEADER_WIDTH - g_Seq.scrollX + (int)(b * 4 * ppb);
        if (barX > w) break;
        if (barX < TRACK_HEADER_WIDTH - 60) continue;

        if (b < g_Seq.barCount) {
            char barText[32];
            snprintf(barText, sizeof(barText), "Bar %d", b + 1);
            SetBkMode(g_cacheDC, TRANSPARENT);
            SetTextColor(g_cacheDC, RGB(165, 175, 190));
            TextOutA(g_cacheDC, barX + 4, HEADER_HEIGHT - 19, barText, (int)strlen(barText));

            char secText[32];
            float barSeconds = (float)(b * 4) * secondsPerBeat;
            snprintf(secText, sizeof(secText), "%.1fs", barSeconds);
            SetTextColor(g_cacheDC, RGB(90, 100, 115));
            TextOutA(g_cacheDC, barX + 44, HEADER_HEIGHT - 19, secText, (int)strlen(secText));
        }
    }

    g_timelineDirty = false;
}

/* ============================================================
 * MAIN UI RENDER ENTRY POINT
 * ============================================================ */
static inline void render_ui(HDC hdc, const RECT *clientRect) {
    int w = clientRect->right - clientRect->left;
    int h = clientRect->bottom - clientRect->top;
    if (w <= 0 || h <= 0) return;

    if (is_timeline_dirty(w, h)) {
        update_timeline_cache(hdc, w, h);
    }

    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, w, h);
    HGDIOBJ oldBmp = SelectObject(memDC, memBitmap);

    BitBlt(memDC, 0, 0, w, h, g_cacheDC, 0, 0, SRCCOPY);

    int viewportTop = HEADER_HEIGHT;
    int viewportBottom = h - BOTTOM_DOCK_HEIGHT;
    int trackAreaEndY = HEADER_HEIGHT - g_Seq.scrollY + g_Seq.trackCount * TRACK_HEIGHT;
    float ppb = get_pixels_per_beat();

    // Volume Popup
    seq_lock();
    if (g_Seq.volumePopupClip >= 0 && g_Seq.volumePopupClip < g_Seq.clipCount && GetTickCount() < g_Seq.volumePopupExpiry) {
        Clip *c = &g_Seq.clips[g_Seq.volumePopupClip];
        int clipX1 = TRACK_HEADER_WIDTH - g_Seq.scrollX + (int)(c->startBeat * ppb);
        int clipY2 = HEADER_HEIGHT - g_Seq.scrollY + (c->track + 1) * TRACK_HEIGHT;
        float volVal = c->volume;
        seq_unlock();

        RECT vPopRect = {clipX1 + 4, clipY2 - 20, clipX1 + 72, clipY2 - 2};
        HBRUSH vBg = CreateSolidBrush(RGB(15, 18, 24));
        HPEN vBorder = CreatePen(PS_SOLID, 1, get_volume_gradient_color(volVal));
        HGDIOBJ oldPopB = SelectObject(memDC, vBg);
        HGDIOBJ oldPopP = SelectObject(memDC, vBorder);
        RoundRect(memDC, vPopRect.left, vPopRect.top, vPopRect.right, vPopRect.bottom, 4, 4);

        char vText[32];
        snprintf(vText, sizeof(vText), "Vol: %d%%", (int)(volVal * 100.0f + 0.5f));
        SetBkMode(memDC, TRANSPARENT);
        SetTextColor(memDC, get_volume_gradient_color(volVal));
        DrawTextA(memDC, vText, -1, &vPopRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(memDC, oldPopP);
        SelectObject(memDC, oldPopB);
        DeleteObject(vBorder);
        DeleteObject(vBg);
    } else {
        seq_unlock();
    }

    LONG pFrame = InterlockedCompareExchange(&g_Seq.playbackFrame, 0, 0);
    float currentBeat = frame_to_beat((ma_uint64)pFrame, g_Seq.bpm, g_Seq.swing);
    int playheadX = TRACK_HEADER_WIDTH - g_Seq.scrollX + (int)(currentBeat * ppb);
    int playheadTop = viewportTop;
    int playheadBottom = min(viewportBottom, trackAreaEndY);

    if (playheadBottom > playheadTop && playheadX >= TRACK_HEADER_WIDTH && playheadX <= w) {
        HPEN playheadPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
        HGDIOBJ oldPhPen = SelectObject(memDC, playheadPen);
        MoveToEx(memDC, playheadX, playheadTop, NULL);
        LineTo(memDC, playheadX, playheadBottom);
        SelectObject(memDC, oldPhPen);
        DeleteObject(playheadPen);

        POINT arrowPts[3] = {
            {playheadX - 4, HEADER_HEIGHT},
            {playheadX + 4, HEADER_HEIGHT},
            {playheadX, HEADER_HEIGHT + 7}
        };

        HBRUSH arrowBrush = CreateSolidBrush(RGB(255, 255, 255));
        HPEN arrowPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
        HGDIOBJ oldABrush = SelectObject(memDC, arrowBrush);
        HGDIOBJ oldAPen = SelectObject(memDC, arrowPen);
        Polygon(memDC, arrowPts, 3);
        SelectObject(memDC, oldAPen);
        SelectObject(memDC, oldABrush);
        DeleteObject(arrowPen);
        DeleteObject(arrowBrush);
    }

    if (g_Seq.isMarqueeSelecting) {
        int mX1 = min(g_Seq.marqueeStartX, g_Seq.marqueeCurX);
        int mX2 = max(g_Seq.marqueeStartX, g_Seq.marqueeCurX);
        int mY1 = min(g_Seq.marqueeStartY, g_Seq.marqueeCurY);
        int mY2 = max(g_Seq.marqueeStartY, g_Seq.marqueeCurY);

        draw_alpha_box(memDC, mX1, mY1, mX2 - mX1, mY2 - mY1, RGB(60, 140, 240), 65, RGB(110, 190, 255));
    }

    // Top Badges
    if (g_Seq.isPlaying) draw_fixed_badge(memDC, 0, 10, "PLAY", RGB(95, 220, 160));
    else draw_fixed_badge(memDC, 0, 10, "PAUSE", RGB(240, 80, 80));

    char bpmBuf[32];
    snprintf(bpmBuf, sizeof(bpmBuf), "%.0f BPM", g_Seq.bpm);
    draw_fixed_badge(memDC, 1, 10, bpmBuf, RGB(215, 220, 230));

    char barBuf[32];
    snprintf(barBuf, sizeof(barBuf), "%d BARS", g_Seq.barCount);
    draw_fixed_badge(memDC, 2, 10, barBuf, RGB(215, 220, 230));

    char swingBuf[32];
    snprintf(swingBuf, sizeof(swingBuf), "SWING %d%%", (int)(g_Seq.swing * 100.0f + 0.5f));
    draw_fixed_badge(memDC, 3, 10, swingBuf, g_Seq.swing > 0.001f ? RGB(255, 205, 110) : RGB(170, 178, 190));

    char snapBuf[32];
    snprintf(snapBuf, sizeof(snapBuf), "SNAP %s", g_Seq.quantizeEnabled ? "1/16" : "OFF");
    draw_fixed_badge(memDC, 4, 10, snapBuf, g_Seq.quantizeEnabled ? RGB(110, 220, 240) : RGB(130, 140, 155));

    draw_fixed_badge(memDC, 5, 10,
                     g_Seq.playFromStartOnPlay ? "FROM START" : "FROM CURSOR",
                     g_Seq.playFromStartOnPlay ? RGB(110, 200, 240) : RGB(130, 140, 155));

    char lofiBuf[32];
    snprintf(lofiBuf, sizeof(lofiBuf), "LO-FI %s", g_Seq.isLofi ? "ON" : "OFF");
    draw_fixed_badge(memDC, 6, 10, lofiBuf, g_Seq.isLofi ? RGB(255, 175, 80) : RGB(130, 140, 155));

    draw_fixed_badge(memDC, 7, 10, "IMPORT", RGB(140, 230, 210));
    draw_fixed_badge(memDC, 8, 10, "EXPORT", RGB(160, 215, 255));
    draw_fixed_badge(memDC, 9, 10, "SAVE", RGB(120, 230, 180));
    draw_fixed_badge(memDC, 10, 10, "LOAD", RGB(245, 190, 100));
    draw_fixed_badge(memDC, 11, 10, "KEYBINDS", RGB(200, 180, 240));

    // Bottom Dock
    RECT bottomDockRect = {0, h - BOTTOM_DOCK_HEIGHT, w, h};
    HBRUSH bottomDockBrush = CreateSolidBrush(RGB(22, 25, 30));
    FillRect(memDC, &bottomDockRect, bottomDockBrush);
    DeleteObject(bottomDockBrush);

    HPEN bottomDockDivPen = CreatePen(PS_SOLID, 1, RGB(32, 36, 45));
    HGDIOBJ oldDockPen = SelectObject(memDC, bottomDockDivPen);
    MoveToEx(memDC, 0, h - BOTTOM_DOCK_HEIGHT, NULL);
    LineTo(memDC, w, h - BOTTOM_DOCK_HEIGHT);
    MoveToEx(memDC, TRACK_HEADER_WIDTH, h - BOTTOM_DOCK_HEIGHT, NULL);
    LineTo(memDC, TRACK_HEADER_WIDTH, h);
    SelectObject(memDC, oldDockPen);
    DeleteObject(bottomDockDivPen);

    int btnY = h - 38;
    int btnW = 33, btnH = 20, startX = 8;
    bool isAddHover = (g_Seq.mouseX >= startX && g_Seq.mouseX <= startX + btnW &&
                       g_Seq.mouseY >= btnY && g_Seq.mouseY <= btnY + btnH);
    bool isDelHover = (g_Seq.mouseX >= startX + btnW + 3 && g_Seq.mouseX <= startX + btnW * 2 + 3 &&
                       g_Seq.mouseY >= btnY && g_Seq.mouseY <= btnY + btnH);

    RECT addBtn = {startX, btnY, startX + btnW, btnY + btnH};
    HBRUSH addBrush = CreateSolidBrush(isAddHover ? RGB(45, 56, 68) : RGB(28, 34, 42));
    HPEN addPen = CreatePen(PS_SOLID, 1, isAddHover ? RGB(90, 150, 120) : RGB(48, 58, 72));
    HGDIOBJ oldAddB = SelectObject(memDC, addBrush);
    HGDIOBJ oldAddP = SelectObject(memDC, addPen);
    RoundRect(memDC, addBtn.left, addBtn.top, addBtn.right, addBtn.bottom, 4, 4);
    SetTextColor(memDC, isAddHover ? RGB(160, 245, 195) : RGB(110, 200, 150));
    DrawTextA(memDC, "+", -1, &addBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(memDC, oldAddP);
    SelectObject(memDC, oldAddB);
    DeleteObject(addPen);
    DeleteObject(addBrush);

    RECT delBtn = {startX + btnW + 3, btnY, startX + btnW * 2 + 3, btnY + btnH};
    bool canDelete = (g_Seq.trackCount > MIN_TRACKS);
    HBRUSH delBrush = CreateSolidBrush(isDelHover && canDelete ? RGB(56, 42, 45) : RGB(28, 34, 42));
    HPEN delPen = CreatePen(PS_SOLID, 1, isDelHover && canDelete ? RGB(160, 80, 80) : RGB(48, 58, 72));
    HGDIOBJ oldDelB = SelectObject(memDC, delBrush);
    HGDIOBJ oldDelP = SelectObject(memDC, delPen);
    RoundRect(memDC, delBtn.left, delBtn.top, delBtn.right, delBtn.bottom, 4, 4);
    SetTextColor(memDC, canDelete ? (isDelHover ? RGB(255, 140, 140) : RGB(215, 100, 100)) : RGB(75, 80, 90));
    DrawTextA(memDC, "-", -1, &delBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(memDC, oldDelP);
    SelectObject(memDC, oldDelB);
    DeleteObject(delPen);
    DeleteObject(delBrush);

    // Bottom Beat Pulses
    float beatFrac = fmodf(currentBeat, 1.0f);
    int activeBeat = (int)fmodf(currentBeat, 4.0f);
    float pulse = g_Seq.isPlaying ? (1.0f - beatFrac * 0.65f) : 0.0f;
    int lineStartX = 8;
    int blockW = 15;
    int blockH = 5;
    int lineY = h - 13;

    for (int b = 0; b < 4; ++b) {
        int sqX = lineStartX + b * (blockW + 3);
        RECT sqRect = {sqX, lineY, sqX + blockW, lineY + blockH};

        COLORREF sqCol = RGB(28, 33, 42);
        if (g_Seq.isPlaying && b == activeBeat) {
            COLORREF baseCol = (b == 0) ? RGB(255, 200, 70) : RGB(80, 240, 180);
            sqCol = RGB((BYTE)(GetRValue(baseCol) * pulse),
                        (BYTE)(GetGValue(baseCol) * pulse),
                        (BYTE)(GetBValue(baseCol) * pulse));
        }

        HBRUSH sqBrush = CreateSolidBrush(sqCol);
        HPEN sqPen = CreatePen(PS_SOLID, 1, (b == activeBeat && g_Seq.isPlaying) ? RGB(160, 255, 220) : RGB(42, 48, 60));
        HGDIOBJ oldSqB = SelectObject(memDC, sqBrush);
        HGDIOBJ oldSqP = SelectObject(memDC, sqPen);
        RoundRect(memDC, sqRect.left, sqRect.top, sqRect.right, sqRect.bottom, 2, 2);
        SelectObject(memDC, oldSqP);
        SelectObject(memDC, oldSqB);
        DeleteObject(sqPen);
        DeleteObject(sqBrush);
    }

    if (g_Seq.exportMsgActive) {
        if ((int)(GetTickCount() - g_Seq.exportMsgExpiry) >= 0) {
            g_Seq.exportMsgActive = false;
        } else {
            SetBkMode(memDC, TRANSPARENT);
            SetTextColor(memDC, RGB(180, 195, 220));
            TextOutA(memDC, TRACK_HEADER_WIDTH + 14, h - 26, g_Seq.exportMsg, (int)strlen(g_Seq.exportMsg));
        }
    }

    if (fabsf(g_Seq.zoom - 1.0f) > 0.001f) {
        int zoomBtnW = 86, zoomBtnH = 22;
        int zoomX = w - zoomBtnW - 14, zoomY = h - 33;
        bool isZoomHover = (g_Seq.mouseX >= zoomX && g_Seq.mouseX <= zoomX + zoomBtnW &&
                            g_Seq.mouseY >= zoomY && g_Seq.mouseY <= zoomY + zoomBtnH);

        RECT zRect = {zoomX, zoomY, zoomX + zoomBtnW, zoomY + zoomBtnH};
        HBRUSH zBrush = CreateSolidBrush(isZoomHover ? RGB(36, 46, 60) : RGB(26, 32, 42));
        HPEN zPen = CreatePen(PS_SOLID, 1, isZoomHover ? RGB(90, 160, 240) : RGB(45, 65, 90));
        HGDIOBJ oldZB = SelectObject(memDC, zBrush);
        HGDIOBJ oldZP = SelectObject(memDC, zPen);
        RoundRect(memDC, zRect.left, zRect.top, zRect.right, zRect.bottom, 4, 4);

        char zBuf[32];
        snprintf(zBuf, sizeof(zBuf), "%d%% ZOOM", (int)(g_Seq.zoom * 100.0f + 0.5f));
        SetTextColor(memDC, isZoomHover ? RGB(180, 220, 255) : RGB(130, 175, 225));
        DrawTextA(memDC, zBuf, -1, &zRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(memDC, oldZP);
        SelectObject(memDC, oldZB);
        DeleteObject(zPen);
        DeleteObject(zBrush);
    }

    // Center Modal Overlay when Saving
    if (g_Seq.isSaving) {
        draw_alpha_box(memDC, 0, 0, w, h, RGB(10, 13, 18), 170, RGB(0, 0, 0));

        int cardW = 280, cardH = 92;
        int cardX = (w - cardW) / 2;
        int cardY = (h - cardH) / 2;

        RECT cRect = { cardX, cardY, cardX + cardW, cardY + cardH };
        HBRUSH cBg = CreateSolidBrush(RGB(22, 26, 34));
        HPEN cBorder = CreatePen(PS_SOLID, 1, RGB(60, 180, 230));
        HGDIOBJ oldCBg = SelectObject(memDC, cBg);
        HGDIOBJ oldCPen = SelectObject(memDC, cBorder);
        RoundRect(memDC, cRect.left, cRect.top, cRect.right, cRect.bottom, 8, 8);
        SelectObject(memDC, oldCPen);
        SelectObject(memDC, oldCBg);
        DeleteObject(cBorder);
        DeleteObject(cBg);

        char saveMsg[64];
        snprintf(saveMsg, sizeof(saveMsg), "SAVING PROJECT... %d%%", g_Seq.saveProgress);
        SetBkMode(memDC, TRANSPARENT);
        SetTextColor(memDC, RGB(120, 225, 255));
        RECT tRc = { cardX, cardY + 18, cardX + cardW, cardY + 38 };
        DrawTextA(memDC, saveMsg, -1, &tRc, DT_CENTER | DT_SINGLELINE | DT_NOPREFIX);

        // Progress Bar
        int barX = cardX + 24, barY = cardY + 52, barW = cardW - 48, barH = 10;
        RECT railRc = { barX, barY, barX + barW, barY + barH };
        HBRUSH railBg = CreateSolidBrush(RGB(14, 16, 22));
        FillRect(memDC, &railRc, railBg);
        DeleteObject(railBg);

        int fillW = (int)((float)barW * ((float)g_Seq.saveProgress / 100.0f));
        if (fillW > 0) {
            RECT fillRc = { barX, barY, barX + fillW, barY + barH };
            HBRUSH fillBg = CreateSolidBrush(RGB(80, 240, 180));
            FillRect(memDC, &fillRc, fillBg);
            DeleteObject(fillBg);
        }
    }

    BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);
    SelectObject(memDC, oldBmp);
    DeleteObject(memBitmap);
    DeleteDC(memDC);
}

/* ============================================================
 * ICON GENERATION & EXPORT
 * ============================================================ */
static inline void draw_app_icon_to_buffer(HDC memDC, int s, DWORD *pPixels) {
    for (int y = 0; y < s; ++y) {
        float t = (float)y / (float)(s - 1);
        BYTE r = (BYTE)(28.0f * (1.0f - t) + 16.0f * t);
        BYTE g = (BYTE)(32.0f * (1.0f - t) + 18.0f * t);
        BYTE b = (BYTE)(40.0f * (1.0f - t) + 24.0f * t);
        for (int x = 0; x < s; ++x) pPixels[y * s + x] = RGB(r, g, b);
    }

    int pad = max(3, (int)((float)s * 0.20f));
    int innerW = s - 2 * pad, innerH = s - 2 * pad;
    int barH = max(2, (int)((float)innerH * 0.24f));
    int gapY = max(2, (innerH - 3 * barH) / 2);
    int pillRad = max(2, barH);

    HPEN nullPen = (HPEN)GetStockObject(NULL_PEN);
    HGDIOBJ oldPen = SelectObject(memDC, nullPen);

    HBRUSH cyan = CreateSolidBrush(RGB(60, 200, 255));
    SelectObject(memDC, cyan);
    RoundRect(memDC, pad, pad, pad + innerW, pad + barH, pillRad, pillRad);
    DeleteObject(cyan);

    int midY = pad + barH + gapY;
    int midW = max(barH, (int)((float)innerW * 0.38f));
    HBRUSH mint = CreateSolidBrush(RGB(80, 240, 180));
    SelectObject(memDC, mint);
    RoundRect(memDC, pad, midY, pad + midW, midY + barH, pillRad, pillRad);
    DeleteObject(mint);

    int botY = pad + 2 * (barH + gapY);
    HBRUSH blue = CreateSolidBrush(RGB(55, 155, 245));
    SelectObject(memDC, blue);
    RoundRect(memDC, pad, botY, pad + innerW, botY + barH, pillRad, pillRad);
    DeleteObject(blue);

    if (s >= 24) {
        int ghostX = pad + midW + max(2, (int)((float)innerW * 0.12f));
        HBRUSH ghost = CreateSolidBrush(RGB(40, 48, 62));
        SelectObject(memDC, ghost);
        RoundRect(memDC, ghostX, midY, pad + innerW, midY + barH, pillRad, pillRad);
        DeleteObject(ghost);
    }

    HPEN bPen = CreatePen(PS_SOLID, 1, RGB(55, 68, 88));
    SelectObject(memDC, bPen);
    SelectObject(memDC, GetStockObject(NULL_BRUSH));
    int outerRad = max(4, s / 4);
    RoundRect(memDC, 0, 0, s, s, outerRad, outerRad);
    DeleteObject(bPen);
    SelectObject(memDC, oldPen);

    float cRad = (float)outerRad * 0.5f;
    for (int y = 0; y < s; ++y) {
        for (int x = 0; x < s; ++x) {
            int idx = y * s + x;
            DWORD rgb = pPixels[idx] & 0x00FFFFFF;
            float cx = (x < cRad) ? cRad : ((x > (s - 1 - cRad)) ? (s - 1 - cRad) : (float)x);
            float cy = (y < cRad) ? cRad : ((y > (s - 1 - cRad)) ? (s - 1 - cRad) : (float)y);
            float dist = sqrtf((x - cx) * (x - cx) + (y - cy) * (y - cy));
            BYTE alpha = (dist > cRad) ? (dist - cRad >= 1.0f ? 0 : (BYTE)((1.0f - (dist - cRad)) * 255.0f)) : 255;
            pPixels[idx] = ((DWORD)alpha << 24) | ((DWORD)(((rgb >> 16) & 0xFF) * alpha / 255) << 16) | ((DWORD)(((rgb >> 8) & 0xFF) * alpha / 255) << 8) | (DWORD)((rgb & 0xFF) * alpha / 255);
        }
    }
}

static inline HICON create_app_icon(int size) {
    if (size <= 0) size = 32;
    HDC screenDC = GetDC(NULL), memDC = CreateCompatibleDC(screenDC), maskDC = CreateCompatibleDC(screenDC);
    BITMAPINFO bmi = { {sizeof(BITMAPINFOHEADER), size, -size, 1, 32, BI_RGB} };
    DWORD *pPixels = NULL;
    HBITMAP hColorBmp = CreateDIBSection(memDC, &bmi, DIB_RGB_COLORS, (void **)&pPixels, NULL, 0);
    HBITMAP hMaskBmp = CreateBitmap(size, size, 1, 1, NULL);
    HGDIOBJ oldColor = SelectObject(memDC, hColorBmp), oldMask = SelectObject(maskDC, hMaskBmp);
    RECT rc = {0, 0, size, size};
    FillRect(maskDC, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));

    draw_app_icon_to_buffer(memDC, size, pPixels);

    SelectObject(memDC, oldColor);
    SelectObject(maskDC, oldMask);
    DeleteDC(memDC);
    DeleteDC(maskDC);
    ReleaseDC(NULL, screenDC);

    ICONINFO ii = { TRUE, 0, 0, hMaskBmp, hColorBmp };
    HICON hIcon = CreateIconIndirect(&ii);
    DeleteObject(hColorBmp);
    DeleteObject(hMaskBmp);
    return hIcon;
}

static inline bool export_app_ico_file(const char *filename) {
    const int sizes[] = {16, 32, 48};
    FILE *fp = fopen(filename, "wb");
    if (!fp) return false;

    struct { WORD idReserved, idType, idCount; } header = {0, 1, 3};
    fwrite(&header, sizeof(header), 1, fp);

    DWORD offset = sizeof(header) + 3 * 16;
    for (int i = 0; i < 3; ++i) {
        int s = sizes[i], imageBytes = 40 + (s * s * 4) + (((s + 31) / 32) * 4 * s);
        struct { BYTE bW, bH, bCol, bRes; WORD wPl, wBpp; DWORD dwBytes, dwOff; } ent = { (BYTE)s, (BYTE)s, 0, 0, 1, 32, (DWORD)imageBytes, offset };
        fwrite(&ent, 16, 1, fp);
        offset += imageBytes;
    }

    for (int i = 0; i < 3; ++i) {
        int s = sizes[i];
        HDC memDC = CreateCompatibleDC(NULL);
        BITMAPINFO bmi = { {sizeof(BITMAPINFOHEADER), s, -s, 1, 32, BI_RGB} };
        DWORD *pPixels = NULL;
        HBITMAP hBmp = CreateDIBSection(memDC, &bmi, DIB_RGB_COLORS, (void **)&pPixels, NULL, 0);
        HGDIOBJ old = SelectObject(memDC, hBmp);

        draw_app_icon_to_buffer(memDC, s, pPixels);

        bmi.bmiHeader.biHeight = s * 2;
        fwrite(&bmi.bmiHeader, 40, 1, fp);
        for (int y = s - 1; y >= 0; --y) fwrite(&pPixels[y * s], 4, s, fp);
        BYTE *mask = (BYTE*)calloc(1, ((s + 31) / 32) * 4 * s);
        if (mask) {
            fwrite(mask, 1, ((s + 31) / 32) * 4 * s, fp);
            free(mask);
        }

        SelectObject(memDC, old);
        DeleteObject(hBmp);
        DeleteDC(memDC);
    }
    fclose(fp);
    return true;
}