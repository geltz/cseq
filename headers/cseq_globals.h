#pragma once
#include "cseq_types.h"

extern SequencerState g_Seq;
extern HWND g_hWnd;

static inline void seq_lock(void) {
    EnterCriticalSection(&g_Seq.lock);
}

static inline void seq_unlock(void) {
    LeaveCriticalSection(&g_Seq.lock);
}