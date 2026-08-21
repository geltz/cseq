#pragma once
#include "cseq_globals.h"
#include "cseq_codec.h"
#include "cseq_ui.h"
#include "cseq_state.h"
#include "cseq_actions.h"
#include <windows.h>
#include <commdlg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* ============================================================
 * .CSQ PROJECT SERIALIZATION & DESERIALIZATION (ASYNC WORKER)
 * ============================================================ */
static DWORD WINAPI SaveProjectThreadProc(LPVOID lpParam) {
    char *path = (char*)lpParam;
    FILE *fp = fopen(path, "wb");
    if (!fp) {
        MessageBoxA(g_hWnd, "Could not open file for writing.", "Save Error", MB_ICONERROR);
        g_Seq.isSaving = false;
        free(path);
        return 0;
    }

    seq_lock();

    CSQHeader hdr;
    memset(&hdr, 0, sizeof(hdr));
    memcpy(hdr.magic, "CSQ1", 4);
    hdr.bpm = g_Seq.bpm;
    hdr.swing = g_Seq.swing;
    hdr.barCount = g_Seq.barCount;
    hdr.trackCount = g_Seq.trackCount;
    hdr.sampleCount = g_Seq.sampleCount;
    hdr.clipCount = g_Seq.clipCount;
    hdr.isLofi = g_Seq.isLofi ? 1 : 0;
    hdr.quantizeEnabled = g_Seq.quantizeEnabled ? 1 : 0;
    fwrite(&hdr, sizeof(hdr), 1, fp);

    for (int t = 0; t < g_Seq.trackCount; ++t) {
        CSQTrack trk;
        memset(&trk, 0, sizeof(trk));
        trk.trackIndex = t;
        trk.isMuted = g_Seq.trackMuted[t] ? 1 : 0;
        trk.volume = g_Seq.trackVolume[t];
        trk.eqLow = g_Seq.trackEqLow[t];
        trk.eqMid = g_Seq.trackEqMid[t];
        trk.eqHigh = g_Seq.trackEqHigh[t];
        memcpy(trk.eqFreq, g_Seq.trackEqFreq[t], sizeof(float) * 3);
        memcpy(trk.eqQ, g_Seq.trackEqQ[t], sizeof(float) * 3);
        fwrite(&trk, sizeof(trk), 1, fp);
    }

    for (int c = 0; c < g_Seq.clipCount; ++c) {
        fwrite(&g_Seq.clips[c], sizeof(Clip), 1, fp);
    }

    int totalSamples = g_Seq.sampleCount;
    g_Seq.saveProgress = totalSamples > 0 ? 5 : 50;

    for (int s = 0; s < totalSamples; ++s) {
        AudioSample *as = &g_Seq.samples[s];
        CSQSampleHeader shdr;
        memset(&shdr, 0, sizeof(shdr));
        strncpy(shdr.name, as->name, sizeof(shdr.name) - 1);
        shdr.frameCount = as->frameCount;
        size_t rawSize = (size_t)as->frameCount * sizeof(float) * NUM_CHANNELS;
        shdr.rawBytes = (DWORD)rawSize;

        if (rawSize > 0 && as->pFrames) {
            size_t compSize = 0;
            unsigned char *compData = csq_compress_lz((const unsigned char*)as->pFrames, rawSize, &compSize);
            if (compData) {
                shdr.compBytes = (DWORD)compSize;
                fwrite(&shdr, sizeof(shdr), 1, fp);
                fwrite(compData, 1, compSize, fp);
                free(compData);
            } else {
                shdr.compBytes = (DWORD)rawSize;
                fwrite(&shdr, sizeof(shdr), 1, fp);
                fwrite(as->pFrames, 1, rawSize, fp);
            }
        } else {
            shdr.rawBytes = 0;
            shdr.compBytes = 0;
            fwrite(&shdr, sizeof(shdr), 1, fp);
        }

        g_Seq.saveProgress = 10 + (int)(((float)(s + 1) / (float)totalSamples) * 85.0f);
    }

    seq_unlock();
    fclose(fp);

    g_Seq.saveProgress = 100;
    snprintf(g_Seq.exportMsg, sizeof(g_Seq.exportMsg), "Project saved to .csq successfully.");
    g_Seq.exportMsgActive = true;
    g_Seq.exportMsgExpiry = GetTickCount() + 4000;
    g_Seq.isSaving = false;

    free(path);
    if (g_hWnd) InvalidateRect(g_hWnd, NULL, FALSE);
    return 0;
}

static inline void save_project_to_csq(const char *path) {
    if (g_Seq.isSaving) return;
    g_Seq.isSaving = true;
    g_Seq.saveProgress = 0;

    char *pathCopy = _strdup(path);
    HANDLE hThread = CreateThread(NULL, 0, SaveProjectThreadProc, (LPVOID)pathCopy, 0, NULL);
    if (hThread) CloseHandle(hThread);
}

static inline void load_project_from_csq(const char *path) {
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        MessageBoxA(g_hWnd, "Could not open .csq project file.", "Load Error", MB_ICONERROR);
        return;
    }

    CSQHeader hdr;
    if (fread(&hdr, sizeof(hdr), 1, fp) != 1 || memcmp(hdr.magic, "CSQ1", 4) != 0) {
        fclose(fp);
        MessageBoxA(g_hWnd, "Invalid or unsupported .csq file format.", "Load Error", MB_ICONERROR);
        return;
    }

    if (hdr.trackCount < MIN_TRACKS) hdr.trackCount = MIN_TRACKS;
    if (hdr.trackCount > MAX_TRACKS) hdr.trackCount = MAX_TRACKS;
    if (hdr.barCount < MIN_BARS) hdr.barCount = MIN_BARS;
    if (hdr.barCount > MAX_BARS) hdr.barCount = MAX_BARS;
    if (hdr.sampleCount < 0) hdr.sampleCount = 0;
    if (hdr.sampleCount > MAX_SAMPLES) hdr.sampleCount = MAX_SAMPLES;
    if (hdr.clipCount < 0) hdr.clipCount = 0;
    if (hdr.clipCount > MAX_CLIPS) hdr.clipCount = MAX_CLIPS;
    if (hdr.bpm < 20.0f || hdr.bpm > 400.0f) hdr.bpm = 120.0f;

    /* Stop audio device safely before mutating samples and clips */
    bool restartDevice = false;
    if (g_Seq.deviceInitialized && g_Seq.isPlaying) {
        g_Seq.isPlaying = false;
        ma_device_stop(&g_Seq.device);
        restartDevice = true;
    }

    push_undo_state();

    seq_lock();

    for (int i = 0; i < g_Seq.sampleCount; ++i) {
        if (g_Seq.samples[i].pFrames) {
            free(g_Seq.samples[i].pFrames);
            g_Seq.samples[i].pFrames = NULL;
        }
        g_Seq.samples[i].loaded = false;
    }
    g_Seq.sampleCount = 0;
    g_Seq.clipCount = 0;

    g_Seq.bpm = hdr.bpm;
    g_Seq.swing = hdr.swing;
    g_Seq.barCount = hdr.barCount;
    g_Seq.trackCount = hdr.trackCount;
    g_Seq.isLofi = hdr.isLofi != 0;
    g_Seq.quantizeEnabled = hdr.quantizeEnabled != 0;
    InterlockedExchange(&g_Seq.playbackFrame, 0);

    for (int t = 0; t < g_Seq.trackCount; ++t) {
        CSQTrack trk;
        if (fread(&trk, sizeof(trk), 1, fp) == 1) {
            g_Seq.trackMuted[t] = trk.isMuted != 0;
            g_Seq.trackVolume[t] = trk.volume;
            g_Seq.trackEqLow[t] = trk.eqLow;
            g_Seq.trackEqMid[t] = trk.eqMid;
            g_Seq.trackEqHigh[t] = trk.eqHigh;
            memcpy(g_Seq.trackEqFreq[t], trk.eqFreq, sizeof(float) * 3);
            memcpy(g_Seq.trackEqQ[t], trk.eqQ, sizeof(float) * 3);
            init_track_theme(t);
        }
    }

    for (int c = 0; c < hdr.clipCount; ++c) {
        Clip clp;
        if (fread(&clp, sizeof(Clip), 1, fp) == 1) {
            if (g_Seq.clipCount < MAX_CLIPS) {
                if (clp.track < 0) clp.track = 0;
                if (clp.track >= g_Seq.trackCount) clp.track = g_Seq.trackCount - 1;
                g_Seq.clips[g_Seq.clipCount++] = clp;
            }
        }
    }

    for (int s = 0; s < hdr.sampleCount; ++s) {
        CSQSampleHeader shdr;
        if (fread(&shdr, sizeof(shdr), 1, fp) != 1) break;

        if (shdr.rawBytes == 0 || shdr.frameCount == 0) continue;
        if (g_Seq.sampleCount >= MAX_SAMPLES) {
            fseek(fp, shdr.compBytes, SEEK_CUR);
            continue;
        }

        AudioSample *as = &g_Seq.samples[g_Seq.sampleCount++];
        memset(as, 0, sizeof(AudioSample));
        strncpy(as->name, shdr.name, sizeof(as->name) - 1);
        strncpy(as->filename, shdr.name, sizeof(as->filename) - 1);
        as->frameCount = shdr.frameCount;

        size_t rawSize = shdr.rawBytes;
        as->pFrames = (float*)malloc(rawSize);
        if (!as->pFrames) {
            fseek(fp, shdr.compBytes, SEEK_CUR);
            g_Seq.sampleCount--;
            continue;
        }

        if (shdr.compBytes == 0) {
            memset(as->pFrames, 0, rawSize);
            as->loaded = true;
            generate_peak_cache(as);
            continue;
        }

        unsigned char *compBuf = (unsigned char*)malloc(shdr.compBytes);
        if (!compBuf) {
            fseek(fp, shdr.compBytes, SEEK_CUR);
            free(as->pFrames);
            as->pFrames = NULL;
            g_Seq.sampleCount--;
            continue;
        }

        if (fread(compBuf, 1, shdr.compBytes, fp) != shdr.compBytes) {
            free(compBuf);
            free(as->pFrames);
            as->pFrames = NULL;
            g_Seq.sampleCount--;
            continue;
        }

        if (shdr.compBytes == shdr.rawBytes) {
            memcpy(as->pFrames, compBuf, rawSize);
        } else {
            if (!csq_decompress_lz(compBuf, shdr.compBytes, (unsigned char*)as->pFrames, rawSize)) {
                memset(as->pFrames, 0, rawSize);
            }
        }
        free(compBuf);
        as->loaded = true;
        generate_peak_cache(as);
    }

    seq_unlock();

    fclose(fp);

    if (restartDevice && g_Seq.deviceInitialized) {
        ma_device_start(&g_Seq.device);
    }

    update_scrollbar(g_hWnd);
    InvalidateRect(g_hWnd, NULL, FALSE);
    snprintf(g_Seq.exportMsg, sizeof(g_Seq.exportMsg), "Loaded .csq module project successfully.");
    g_Seq.exportMsgActive = true;
    g_Seq.exportMsgExpiry = GetTickCount() + 4000;
}

/* ============================================================
 * WIN32 FILE DIALOGS
 * ============================================================ */
static inline void save_project_dialog(HWND hwnd) {
    OPENFILENAMEA ofn;
    char szFile[MAX_PATH] = "project.csq";
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = "cseq Module Project (*.csq)\0*.csq\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = "csq";

    if (GetSaveFileNameA(&ofn)) {
        save_project_to_csq(szFile);
    }
}

static inline void load_project_dialog(HWND hwnd) {
    OPENFILENAMEA ofn;
    char szFile[MAX_PATH] = "";
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = "cseq Module Project (*.csq)\0*.csq\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (GetOpenFileNameA(&ofn)) {
        load_project_from_csq(szFile);
    }
}