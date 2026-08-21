#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <shellapi.h>

#pragma warning(push)
#pragma warning(disable: 4244)
#include "miniaudio.h"
#pragma warning(pop)

#include "smootheq.h"

#include "cseq_config.h"
#include "cseq_types.h"

SequencerState g_Seq;
HWND g_hWnd = NULL;

#include "cseq_globals.h"
#include "cseq_dsp.h"
#include "cseq_codec.h"
#include "cseq_ui.h"
#include "cseq_audio.h"
#include "cseq_project.h"
#include "cseq_state.h"
#include "cseq_actions.h"
#include "cseq_dialogs.h"
#include "cseq_events.h"

/* Wrapper bridging the event handler to the Win32 callback */
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        render_ui(hdc, &rc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    return cseq_main_wndproc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance; (void)lpCmdLine;
    
    // Auto-generate the app icon if missing.
    if (GetFileAttributesA("app.ico") == INVALID_FILE_ATTRIBUTES) export_app_ico_file("app.ico");

    memset(&g_Seq, 0, sizeof(g_Seq)); 
    InitializeCriticalSection(&g_Seq.lock);

    g_Seq.bpm = 120.0f; g_Seq.trackCount = 4; g_Seq.barCount = 4; g_Seq.zoom = 1.0f;
    g_Seq.quantizeEnabled = true;

    // Initialize all tracks with DSP filters
    for (int t = 0; t < MAX_TRACKS; ++t) {
        init_track_theme(t);
        g_Seq.trackVolume[t] = 1.0f;
        g_Seq.trackMuted[t] = false;
        
        g_Seq.trackEqLow[t] = 0.5f; 
        g_Seq.trackEqMid[t] = 0.5f; 
        g_Seq.trackEqHigh[t] = 0.5f;
        g_Seq.trackEqActive[t] = false;

        // Normalized relative offsets matching dialog curves
        g_Seq.trackEqFreq[t][0] = 0.20f;
        g_Seq.trackEqFreq[t][1] = 0.50f;
        g_Seq.trackEqFreq[t][2] = 0.80f;
        g_Seq.trackEqQ[t][0] = g_Seq.trackEqQ[t][1] = g_Seq.trackEqQ[t][2] = 0.7f;

        for (int b = 0; b < 3; ++b) {
            peak_biquad_clear(&g_Seq.trackPeak[t][b]);
            peak_biquad_set(&g_Seq.trackPeak[t][b], 1000.0f, 0.7f, 0.0f, (float)SAMPLE_RATE);
        }
        
        smooth_eq3_init(&g_Seq.trackEQ[t], (double)SAMPLE_RATE);
        smooth_eq3_set_params(&g_Seq.trackEQ[t], 0.5f, 0.5f, 0.5f);
    }
    
    // Boot Miniaudio Engine
    ma_device_config devCfg = ma_device_config_init(ma_device_type_playback);
    devCfg.playback.format = ma_format_f32; devCfg.playback.channels = NUM_CHANNELS; devCfg.sampleRate = SAMPLE_RATE;
    devCfg.dataCallback = audio_callback;
    if (ma_device_init(NULL, &devCfg, &g_Seq.device) == MA_SUCCESS) { 
        g_Seq.deviceInitialized = true; 
        ma_device_start(&g_Seq.device); 
    }

    // Register Window Class
    WNDCLASSA wc = {0}; 
    wc.lpfnWndProc = WndProc; 
    wc.hInstance = hInstance; 
    wc.lpszClassName = "CSeqMainWindow"; 
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    
    // Load Dynamic Icons
    wc.hIcon = (HICON)LoadImageA(hInstance, MAKEINTRESOURCEA(1), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
    if (!wc.hIcon) wc.hIcon = create_app_icon(32);
    RegisterClassA(&wc);
    
    g_hWnd = CreateWindowExA(0, "CSeqMainWindow", "cseq", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1152, 480, NULL, NULL, hInstance, NULL);
    ShowWindow(g_hWnd, nCmdShow); 
    UpdateWindow(g_hWnd);
    
    // Main Event Loop
    MSG msg; 
    while (GetMessageA(&msg, NULL, 0, 0)) { 
        TranslateMessage(&msg); 
        DispatchMessageA(&msg); 
    }
    
    // Cleanup Memory and Hardware
    if (g_Seq.deviceInitialized) { 
        ma_device_stop(&g_Seq.device); 
        ma_device_uninit(&g_Seq.device); 
    }
    for (int i = 0; i < g_Seq.sampleCount; ++i) {
        if (g_Seq.samples[i].pFrames) free(g_Seq.samples[i].pFrames);
    }
    DeleteCriticalSection(&g_Seq.lock);
    
    return (int)msg.wParam;
}