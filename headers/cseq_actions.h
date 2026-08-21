#pragma once
#include "cseq_globals.h"
#include "cseq_dsp.h"
#include "cseq_state.h"
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

/* Forward declarations */
static inline int get_clip_under_mouse(int mx, int my);
static inline void deselect_all_clips(void);
static inline void update_scrollbar(HWND hwnd);
static inline void init_track_theme(int trackIdx);

static inline ma_uint64 find_nearest_zero_crossing(const AudioSample *s, ma_uint64 targetFrame, ma_uint64 maxWindow) {
    if (!s || !s->pFrames || s->frameCount == 0) return 0;
    if (targetFrame >= s->frameCount) targetFrame = s->frameCount - 1;

    ma_uint64 start = (targetFrame > maxWindow) ? (targetFrame - maxWindow) : 0;
    ma_uint64 end = (targetFrame + maxWindow < s->frameCount) ? (targetFrame + maxWindow) : s->frameCount;

    ma_uint64 bestFrame = targetFrame;
    float minAbs = 10.0f;

    for (ma_uint64 f = start; f < end; ++f) {
        float mono = (s->pFrames[f * 2 + 0] + s->pFrames[f * 2 + 1]) * 0.5f;

        if (f > start) {
            float prevMono = (s->pFrames[(f - 1) * 2 + 0] + s->pFrames[(f - 1) * 2 + 1]) * 0.5f;
            if ((prevMono <= 0.0f && mono >= 0.0f) || (prevMono >= 0.0f && mono <= 0.0f)) {
                return f;
            }
        }

        if (fabsf(mono) < minAbs) {
            minAbs = fabsf(mono);
            bestFrame = f;
        }
    }
    return bestFrame;
}

static inline void generate_peak_cache(AudioSample *s) {
    if (!s || !s->pFrames || s->frameCount == 0) return;
    ma_uint64 framesPerPeak = s->frameCount / PEAK_CACHE_SIZE;
    if (framesPerPeak == 0) framesPerPeak = 1;

    for (int i = 0; i < PEAK_CACHE_SIZE; ++i) {
        ma_uint64 start = (ma_uint64)i * framesPerPeak;
        ma_uint64 end = start + framesPerPeak;
        if (end > s->frameCount) end = s->frameCount;

        float minVal = 0.0f;
        float maxVal = 0.0f;

        for (ma_uint64 f = start; f < end; ++f) {
            float mono = (s->pFrames[f * 2 + 0] + s->pFrames[f * 2 + 1]) * 0.5f;
            if (mono < minVal) minVal = mono;
            if (mono > maxVal) maxVal = mono;
        }
        s->peaks[i].min = minVal;
        s->peaks[i].max = maxVal;
    }
}

/* ============================================================
 * AUDIO LOADING & CLIP CREATION
 * ============================================================ */
static inline int load_audio_file(const char *filepath) {
    seq_lock();
    if (g_Seq.sampleCount >= MAX_SAMPLES) {
        seq_unlock();
        return -1;
    }
    seq_unlock();

    ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_f32, NUM_CHANNELS, SAMPLE_RATE);
    ma_decoder decoder;
    ma_result result = ma_decoder_init_file(filepath, &decoderConfig, &decoder);
    if (result != MA_SUCCESS) return -1;

    ma_uint64 totalFrames = 0;
    ma_decoder_get_length_in_pcm_frames(&decoder, &totalFrames);
    if (totalFrames == 0) {
        ma_decoder_uninit(&decoder);
        return -1;
    }

    float *pFrames = (float *)malloc(sizeof(float) * totalFrames * NUM_CHANNELS);
    if (!pFrames) {
        ma_decoder_uninit(&decoder);
        return -1;
    }

    ma_uint64 framesRead = 0;
    ma_decoder_read_pcm_frames(&decoder, pFrames, totalFrames, &framesRead);
    ma_decoder_uninit(&decoder);

    seq_lock();
    if (g_Seq.sampleCount >= MAX_SAMPLES) {
        seq_unlock();
        free(pFrames);
        return -1;
    }

    int idx = g_Seq.sampleCount;
    AudioSample *sample = &g_Seq.samples[idx];
    memset(sample, 0, sizeof(AudioSample));
    strncpy(sample->filename, filepath, MAX_PATH - 1);
    sample->filename[MAX_PATH - 1] = '\0';

    const char *baseName = strrchr(filepath, '\\');
    baseName = baseName ? baseName + 1 : filepath;
    strncpy(sample->name, baseName, sizeof(sample->name) - 1);
    sample->name[sizeof(sample->name) - 1] = '\0';

    sample->pFrames = pFrames;
    sample->frameCount = framesRead;
    generate_peak_cache(sample);
    sample->loaded = true;
    g_Seq.sampleCount++;
    seq_unlock();

    return idx;
}

static inline int add_clip(int sampleIndex, int track, float startBeat) {
    if (sampleIndex < 0 || sampleIndex >= g_Seq.sampleCount) return -1;
    if (track < 0 || track >= g_Seq.trackCount) track = 0;

    push_undo_state();

    AudioSample *s = &g_Seq.samples[sampleIndex];
    float fpb = frames_per_beat(g_Seq.bpm);
    float lengthBeats = (float)s->frameCount / fpb;
    if (lengthBeats < MIN_CLIP_LENGTH_BEATS) lengthBeats = MIN_CLIP_LENGTH_BEATS;

    if (startBeat + lengthBeats > total_beats()) {
        lengthBeats = total_beats() - startBeat;
        if (lengthBeats < MIN_CLIP_LENGTH_BEATS) lengthBeats = MIN_CLIP_LENGTH_BEATS;
    }

    Clip newClip;
    memset(&newClip, 0, sizeof(Clip));
    newClip.sampleIndex = sampleIndex;
    newClip.track = track;
    newClip.startBeat = startBeat;
    newClip.lengthBeats = lengthBeats;
    newClip.sampleOffsetFrames = find_nearest_zero_crossing(s, 0, 128);
    newClip.volume = 1.0f;
    newClip.playbackRate = 1.0f;
    newClip.fadeInBeats = 0.0f;
    newClip.fadeOutBeats = 0.0f;
    newClip.isSelected = true;

    seq_lock();
    if (g_Seq.clipCount >= MAX_CLIPS) {
        seq_unlock();
        return -1;
    }
    int idx = g_Seq.clipCount;
    g_Seq.clips[idx] = newClip;
    g_Seq.clipCount++;
    seq_unlock();

    return idx;
}

/* ============================================================
 * CLIP SPLITTING, SELECTION & CLIPBOARD
 * ============================================================ */
static inline void split_clips_at_playhead(void) {
    ma_uint64 currentFrame = (ma_uint64)InterlockedCompareExchange(&g_Seq.playbackFrame, 0, 0);
    float curBeat = frame_to_beat(currentFrame, g_Seq.bpm, g_Seq.swing);
    curBeat = quantize_beat_16th(curBeat);
    float fpb = frames_per_beat(g_Seq.bpm);
    float maxTimelineBeats = total_beats();

    if (curBeat >= maxTimelineBeats) return;

    push_undo_state();

    seq_lock();
    int originalCount = g_Seq.clipCount;
    bool hasSelection = false;
    for (int i = 0; i < originalCount; ++i) {
        if (g_Seq.clips[i].isSelected) {
            hasSelection = true;
            break;
        }
    }

    for (int i = 0; i < originalCount; ++i) {
        Clip *c = &g_Seq.clips[i];
        if (c->startBeat >= maxTimelineBeats) continue;
        if (hasSelection && !c->isSelected) continue;
        if (c->sampleIndex < 0 || c->sampleIndex >= g_Seq.sampleCount) continue;

        float effectiveEnd = c->startBeat + c->lengthBeats;
        if (effectiveEnd > maxTimelineBeats) {
            c->lengthBeats = maxTimelineBeats - c->startBeat;
        }
        if (c->lengthBeats < MIN_CLIP_LENGTH_BEATS) {
            c->lengthBeats = MIN_CLIP_LENGTH_BEATS;
            continue;
        }

        const float halfMin = MIN_CLIP_LENGTH_BEATS * 0.5f;
        if (curBeat <= c->startBeat + halfMin ||
            curBeat >= (c->startBeat + c->lengthBeats - halfMin)) {
            continue;
        }

        if (g_Seq.clipCount >= MAX_CLIPS) break;

        AudioSample *s = &g_Seq.samples[c->sampleIndex];
        float firstPartLen = curBeat - c->startBeat;
        float secondPartLen = c->lengthBeats - firstPartLen;

        if (firstPartLen < MIN_CLIP_LENGTH_BEATS || secondPartLen < MIN_CLIP_LENGTH_BEATS)
            continue;

        float pRate = (c->playbackRate > 0.01f) ? c->playbackRate : 1.0f;
        ma_uint64 splitDeltaFrames = (ma_uint64)(firstPartLen * fpb * pRate);
        ma_uint64 rawSecondOffset = c->sampleOffsetFrames + splitDeltaFrames;

        if (rawSecondOffset >= s->frameCount) {
            continue;
        }

        ma_uint64 secondOffset = find_nearest_zero_crossing(s, rawSecondOffset, 256);
        float origFadeOut = c->fadeOutBeats;

        c->lengthBeats = firstPartLen;
        if (c->fadeInBeats > c->lengthBeats) c->fadeInBeats = c->lengthBeats;
        c->fadeOutBeats = 0.0f;
        c->isSelected = true;

        Clip n;
        memset(&n, 0, sizeof(Clip));
        n.sampleIndex = c->sampleIndex;
        n.track = c->track;
        n.startBeat = curBeat;
        n.lengthBeats = secondPartLen;
        if (n.lengthBeats < MIN_CLIP_LENGTH_BEATS) n.lengthBeats = MIN_CLIP_LENGTH_BEATS;
        n.sampleOffsetFrames = secondOffset;
        n.volume = c->volume;
        n.playbackRate = c->playbackRate;
        n.fadeInBeats = 0.0f;
        n.fadeOutBeats = (origFadeOut > n.lengthBeats) ? n.lengthBeats : origFadeOut;
        n.isSelected = false;

        g_Seq.clips[g_Seq.clipCount++] = n;
    }
    seq_unlock();
}

static inline void select_all_clips_on_track(int trackIdx) {
    seq_lock();
    for (int i = 0; i < g_Seq.clipCount; ++i) {
        g_Seq.clips[i].isSelected = (g_Seq.clips[i].track == trackIdx);
    }
    seq_unlock();
}

static inline void deselect_all_clips(void) {
    seq_lock();
    for (int i = 0; i < g_Seq.clipCount; ++i) {
        g_Seq.clips[i].isSelected = false;
    }
    seq_unlock();
}

static inline void delete_selected_clips(void) {
    bool hasSelection = false;
    seq_lock();
    for (int i = 0; i < g_Seq.clipCount; ++i) {
        if (g_Seq.clips[i].isSelected) {
            hasSelection = true;
            break;
        }
    }
    seq_unlock();

    if (!hasSelection && g_Seq.hoveredClip >= 0 && g_Seq.hoveredClip < g_Seq.clipCount) {
        push_undo_state();
        seq_lock();
        int del = g_Seq.hoveredClip;
        for (int j = del; j < g_Seq.clipCount - 1; ++j) {
            g_Seq.clips[j] = g_Seq.clips[j + 1];
        }
        g_Seq.clipCount--;
        g_Seq.hoveredClip = -1;
        seq_unlock();
        return;
    }

    if (hasSelection) {
        push_undo_state();
        seq_lock();
        for (int i = 0; i < g_Seq.clipCount;) {
            if (g_Seq.clips[i].isSelected) {
                for (int j = i; j < g_Seq.clipCount - 1; ++j) {
                    g_Seq.clips[j] = g_Seq.clips[j + 1];
                }
                g_Seq.clipCount--;
            } else {
                i++;
            }
        }
        seq_unlock();
    }
}

static inline void copy_selected_clips(void) {
    seq_lock();
    g_Seq.clipboardCount = 0;
    float minBeat = 1e9f;
    int minTrack = 9999;

    for (int i = 0; i < g_Seq.clipCount; ++i) {
        if (g_Seq.clips[i].isSelected) {
            if (g_Seq.clips[i].startBeat < minBeat) minBeat = g_Seq.clips[i].startBeat;
            if (g_Seq.clips[i].track < minTrack) minTrack = g_Seq.clips[i].track;
        }
    }

    if (minTrack == 9999) {
        seq_unlock();
        return;
    }

    for (int i = 0; i < g_Seq.clipCount; ++i) {
        if (g_Seq.clips[i].isSelected && g_Seq.clipboardCount < MAX_CLIPS) {
            ClipboardItem *item = &g_Seq.clipboard[g_Seq.clipboardCount++];
            item->sampleIndex = g_Seq.clips[i].sampleIndex;
            item->trackOffset = g_Seq.clips[i].track - minTrack;
            item->beatOffset = g_Seq.clips[i].startBeat - minBeat;
            item->lengthBeats = g_Seq.clips[i].lengthBeats;
            item->sampleOffsetFrames = g_Seq.clips[i].sampleOffsetFrames;
            item->volume = g_Seq.clips[i].volume;
            item->playbackRate = g_Seq.clips[i].playbackRate;
            item->fadeInBeats = g_Seq.clips[i].fadeInBeats;
            item->fadeOutBeats = g_Seq.clips[i].fadeOutBeats;
        }
    }
    seq_unlock();
}

static inline void paste_clipboard_clips(void) {
    if (g_Seq.clipboardCount == 0) return;

    push_undo_state();

    float curBeat = frame_to_beat((ma_uint64)InterlockedCompareExchange(&g_Seq.playbackFrame, 0, 0), g_Seq.bpm, g_Seq.swing);
    float baseBeat = quantize_beat_16th(curBeat);

    int baseTrack = 0;
    if (g_Seq.mouseY >= HEADER_HEIGHT) {
        baseTrack = (g_Seq.mouseY - HEADER_HEIGHT + g_Seq.scrollY) / TRACK_HEIGHT;
    }
    if (baseTrack < 0) baseTrack = 0;
    if (baseTrack >= g_Seq.trackCount) baseTrack = g_Seq.trackCount - 1;

    seq_lock();
    for (int i = 0; i < g_Seq.clipCount; ++i) g_Seq.clips[i].isSelected = false;

    for (int k = 0; k < g_Seq.clipboardCount; ++k) {
        if (g_Seq.clipCount >= MAX_CLIPS) break;
        ClipboardItem *item = &g_Seq.clipboard[k];
        if (item->sampleIndex < 0 || item->sampleIndex >= g_Seq.sampleCount) continue;

        int targetTrack = baseTrack + item->trackOffset;
        if (targetTrack < 0) targetTrack = 0;
        if (targetTrack >= g_Seq.trackCount) targetTrack = g_Seq.trackCount - 1;

        float targetBeat = quantize_beat_16th(baseBeat + item->beatOffset);
        if (targetBeat < 0.0f) targetBeat = 0.0f;
        if (targetBeat >= total_beats()) continue;

        float fitLen = item->lengthBeats;
        if (targetBeat + fitLen > total_beats()) {
            fitLen = total_beats() - targetBeat;
        }
        if (fitLen < MIN_CLIP_LENGTH_BEATS) fitLen = MIN_CLIP_LENGTH_BEATS;

        Clip c;
        memset(&c, 0, sizeof(Clip));
        c.sampleIndex = item->sampleIndex;
        c.track = targetTrack;
        c.startBeat = targetBeat;
        c.lengthBeats = fitLen;
        c.sampleOffsetFrames = item->sampleOffsetFrames;
        c.volume = item->volume;
        c.playbackRate = item->playbackRate;
        c.fadeInBeats = item->fadeInBeats;
        c.fadeOutBeats = item->fadeOutBeats;
        c.isSelected = true;

        g_Seq.clips[g_Seq.clipCount++] = c;
    }
    seq_unlock();
}

/* ============================================================
 * TIMELINE HIT TESTING & TRANSPORT ACTIONS
 * ============================================================ */
static inline int get_clip_under_mouse(int mx, int my) {
    if (my <= HEADER_HEIGHT || mx < TRACK_HEADER_WIDTH || g_Seq.clipCount <= 0) return -1;

    RECT rcClient;
    if (g_hWnd) {
        GetClientRect(g_hWnd, &rcClient);
        if (my >= (rcClient.bottom - rcClient.top - BOTTOM_DOCK_HEIGHT)) return -1;
    }

    float ppb = get_pixels_per_beat();
    seq_lock();
    for (int i = g_Seq.clipCount - 1; i >= 0; --i) {
        Clip *c = &g_Seq.clips[i];
        if (c->track >= g_Seq.trackCount || c->startBeat >= total_beats()) continue;

        float vLen = c->lengthBeats;
        if (c->startBeat + vLen > total_beats()) {
            vLen = total_beats() - c->startBeat;
        }
        if (vLen <= 0.0f) continue;

        int x1 = TRACK_HEADER_WIDTH - g_Seq.scrollX + (int)(c->startBeat * ppb);
        int x2 = x1 + (int)(vLen * ppb);
        int y1 = HEADER_HEIGHT - g_Seq.scrollY + c->track * TRACK_HEIGHT;
        int y2 = y1 + TRACK_HEIGHT;

        if (mx >= x1 && mx <= x2 && my >= y1 && my <= y2) {
            seq_unlock();
            return i;
        }
    }
    seq_unlock();
    return -1;
}

static inline void toggle_playback(void) {
    if (!g_Seq.isPlaying && g_Seq.playFromStartOnPlay) {
        InterlockedExchange(&g_Seq.playbackFrame, 0);
    }
    g_Seq.isPlaying = !g_Seq.isPlaying;
}

static inline void open_sample_dialog(HWND hwnd, int track, float dropBeat) {
    OPENFILENAMEA ofn;
    char szFile[MAX_PATH] = "";
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = "All Supported Audio (*.wav;*.mp3;*.flac)\0*.wav;*.mp3;*.flac\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (GetOpenFileNameA(&ofn)) {
        int sampleIdx = load_audio_file(szFile);
        if (sampleIdx != -1) {
            add_clip(sampleIdx, track, dropBeat);
            InvalidateRect(hwnd, NULL, FALSE);
        }
    }
}

/* ============================================================
 * VIEWPORT & TRACK ADJUSTMENTS
 * ============================================================ */
static inline void update_scrollbar(HWND hwnd) {
    RECT rc;
    GetClientRect(hwnd, &rc);
    int clientH = rc.bottom - rc.top;
    int clientW = rc.right - rc.left;
    int visibleH = clientH - HEADER_HEIGHT - BOTTOM_DOCK_HEIGHT;
    int totalContentH = g_Seq.trackCount * TRACK_HEIGHT;

    if (totalContentH > visibleH && visibleH > 0) {
        int maxScroll = totalContentH - visibleH;
        if (g_Seq.scrollY > maxScroll) g_Seq.scrollY = maxScroll;
        if (g_Seq.scrollY < 0) g_Seq.scrollY = 0;

        SCROLLINFO si = {0};
        si.cbSize = sizeof(si);
        si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
        si.nMin = 0;
        si.nMax = totalContentH;
        si.nPage = visibleH;
        si.nPos = g_Seq.scrollY;
        SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
        ShowScrollBar(hwnd, SB_VERT, TRUE);
    } else {
        g_Seq.scrollY = 0;
        SCROLLINFO si = {0};
        si.cbSize = sizeof(si);
        si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
        si.nMin = 0;
        si.nMax = 0;
        si.nPage = 0;
        si.nPos = 0;
        SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
        ShowScrollBar(hwnd, SB_VERT, FALSE);
    }

    float ppb = get_pixels_per_beat();
    int totalTimelineWidth = (int)(total_beats() * ppb);
    int visibleWidth = clientW - TRACK_HEADER_WIDTH;
    int maxScrollX = totalTimelineWidth - visibleWidth;
    if (maxScrollX < 0) maxScrollX = 0;
    if (g_Seq.scrollX > maxScrollX) g_Seq.scrollX = maxScrollX;
    if (g_Seq.scrollX < 0) g_Seq.scrollX = 0;
}

static inline void add_track_action(HWND hwnd) {
    seq_lock();
    if (g_Seq.trackCount < MAX_TRACKS) {
        init_track_theme(g_Seq.trackCount);
        g_Seq.trackMuted[g_Seq.trackCount] = false;
        g_Seq.trackVolume[g_Seq.trackCount] = 1.0f;
        g_Seq.trackCount++;
        seq_unlock();
        update_scrollbar(hwnd);
        InvalidateRect(hwnd, NULL, FALSE);
    } else {
        seq_unlock();
    }
}

static inline void remove_track_action(HWND hwnd) {
    seq_lock();
    if (g_Seq.trackCount > MIN_TRACKS) {
        int removedTrack = g_Seq.trackCount - 1;
        for (int i = 0; i < g_Seq.clipCount;) {
            if (g_Seq.clips[i].track == removedTrack) {
                for (int j = i; j < g_Seq.clipCount - 1; ++j) {
                    g_Seq.clips[j] = g_Seq.clips[j + 1];
                }
                g_Seq.clipCount--;
            } else {
                i++;
            }
        }
        g_Seq.trackCount--;
        seq_unlock();
        update_scrollbar(hwnd);
        InvalidateRect(hwnd, NULL, FALSE);
    } else {
        seq_unlock();
    }
}

static inline void change_bar_count(int delta) {
    long long newCount = (long long)g_Seq.barCount + delta;

    if (newCount < MIN_BARS) newCount = MIN_BARS;
    else if (newCount > MAX_BARS) newCount = MAX_BARS;

    g_Seq.barCount = (int)newCount;

    double beats = total_beats();
    double fpb = frames_per_beat(g_Seq.bpm);

    if (beats <= 0.0 || fpb <= 0.0) return;

    double total = beats * fpb;
    if (total > (double)UINT64_MAX) total = (double)UINT64_MAX;

    ma_uint64 loopTotalFrames = (ma_uint64)total;
    if (loopTotalFrames > 0) {
        LONG cur = InterlockedCompareExchange(&g_Seq.playbackFrame, 0, 0);
        InterlockedExchange(&g_Seq.playbackFrame, (LONG)((ma_uint64)cur % loopTotalFrames));
    }
}