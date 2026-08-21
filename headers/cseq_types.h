#pragma once
#include "cseq_config.h"
#include <windows.h>
#include "miniaudio.h"
#include "smootheq.h"

typedef struct { float min, max; } Peak;

typedef struct {
    char filename[MAX_PATH], name[64];
    float *pFrames;
    ma_uint64 frameCount;
    Peak peaks[PEAK_CACHE_SIZE];
    bool loaded;
} AudioSample;

typedef struct {
    int sampleIndex, track;
    float startBeat, lengthBeats;
    ma_uint64 sampleOffsetFrames;
    float volume, playbackRate, fadeInBeats, fadeOutBeats;
    bool isSelected;
    float dragStartBeatOrig;
    int dragStartTrackOrig;
    ma_uint64 dragStartOffsetOrig;
} Clip;

typedef struct {
    int sampleIndex, trackOffset;
    float beatOffset, lengthBeats;
    ma_uint64 sampleOffsetFrames;
    float volume, playbackRate, fadeInBeats, fadeOutBeats;
} ClipboardItem;

typedef struct { COLORREF waveColor, bgColor, selectWaveColor, selectBgColor, borderColor; } TrackTheme;
typedef struct { Clip clips[MAX_CLIPS]; int clipCount; } UndoSnapshot;

typedef struct {
    float b0, b1, b2, a1, a2, z1L, z2L, z1R, z2R;
} PeakBiquad;

typedef struct {
    CRITICAL_SECTION lock;

    AudioSample samples[MAX_SAMPLES];
    int sampleCount;
    Clip clips[MAX_CLIPS];
    int clipCount;

    int trackCount;
    bool trackMuted[MAX_TRACKS];
    float trackVolume[MAX_TRACKS];
    TrackTheme trackThemes[MAX_TRACKS];
    
    ClipboardItem clipboard[MAX_CLIPS];
    int clipboardCount;
    UndoSnapshot undoStack[MAX_UNDO_STATES], redoStack[MAX_UNDO_STATES];
    int undoCount, redoCount;

    float bpm, swing, zoom;
    bool quantizeEnabled, isPlaying, isLofi, playFromStartOnPlay;
    int barCount;

    volatile LONG playbackFrame;

    ma_device device;
    bool deviceInitialized;

    int scrollX, scrollY;
    char exportMsg[96];
    bool exportMsgActive;
    DWORD exportMsgExpiry;
    
    /* Async Save State */
    volatile bool isSaving;
    volatile int saveProgress;
    char saveFilePath[MAX_PATH];

    int draggedClipIndex, dragOrigTrack, marqueeStartX, marqueeStartY, marqueeCurX, marqueeCurY;
    float dragStartBeatOffset, resizeOrigStartBeat, resizeOrigLengthBeats, dragStartVolume;
    ma_uint64 resizeOrigOffsetFrames, dragStartSampleOffset;
    int dragStartX, dragStartY;
    bool isDraggingClip, isCtrlDuplicating, hasMovedPastThreshold, isVolumeDragging, isSlipDragging, isMarqueeSelecting;
    bool isResizingLeft, isResizingRight, isFadeInDragging, isFadeOutDragging;

    bool isMiddlePanning;
    int panStartX, panStartY, panOrigScrollX, panOrigScrollY;
    int volumePopupClip, hoveredClip, mouseX, mouseY;
    DWORD volumePopupExpiry;

    float lofiLpL, lofiLpR;
    
    /* DSP Modules */
    SmoothEQ3 trackEQ[MAX_TRACKS];
    float trackEqLow[MAX_TRACKS], trackEqMid[MAX_TRACKS], trackEqHigh[MAX_TRACKS];
    bool trackEqActive[MAX_TRACKS];
    PeakBiquad trackPeak[MAX_TRACKS][3];
    float trackEqFreq[MAX_TRACKS][3], trackEqQ[MAX_TRACKS][3];
} SequencerState;