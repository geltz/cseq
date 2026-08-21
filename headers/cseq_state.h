#pragma once
#include "cseq_globals.h"

static inline void push_undo_state(void) {
    seq_lock();
    if (g_Seq.undoCount >= MAX_UNDO_STATES) {
        memmove(&g_Seq.undoStack[0], &g_Seq.undoStack[1], sizeof(UndoSnapshot) * (MAX_UNDO_STATES - 1));
        g_Seq.undoCount = MAX_UNDO_STATES - 1;
    }
    g_Seq.undoStack[g_Seq.undoCount].clipCount = g_Seq.clipCount;
    memcpy(g_Seq.undoStack[g_Seq.undoCount++].clips, g_Seq.clips, sizeof(Clip) * g_Seq.clipCount);
    g_Seq.redoCount = 0;
    seq_unlock();
}

static inline void undo_last_action(void) {
    seq_lock();
    if (g_Seq.undoCount <= 0) {
        seq_unlock();
        return;
    }
    if (g_Seq.redoCount >= MAX_UNDO_STATES) {
        memmove(&g_Seq.redoStack[0], &g_Seq.redoStack[1], sizeof(UndoSnapshot) * (MAX_UNDO_STATES - 1));
        g_Seq.redoCount = MAX_UNDO_STATES - 1;
    }
    g_Seq.redoStack[g_Seq.redoCount].clipCount = g_Seq.clipCount;
    memcpy(g_Seq.redoStack[g_Seq.redoCount++].clips, g_Seq.clips, sizeof(Clip) * g_Seq.clipCount);
    UndoSnapshot *s = &g_Seq.undoStack[--g_Seq.undoCount];
    g_Seq.clipCount = s->clipCount; 
    memcpy(g_Seq.clips, s->clips, sizeof(Clip) * s->clipCount);
    seq_unlock();
}

static inline void redo_last_action(void) {
    seq_lock();
    if (g_Seq.redoCount <= 0) {
        seq_unlock();
        return;
    }
    if (g_Seq.undoCount >= MAX_UNDO_STATES) {
        memmove(&g_Seq.undoStack[0], &g_Seq.undoStack[1], sizeof(UndoSnapshot) * (MAX_UNDO_STATES - 1));
        g_Seq.undoCount = MAX_UNDO_STATES - 1;
    }
    g_Seq.undoStack[g_Seq.undoCount].clipCount = g_Seq.clipCount;
    memcpy(g_Seq.undoStack[g_Seq.undoCount++].clips, g_Seq.clips, sizeof(Clip) * g_Seq.clipCount);
    UndoSnapshot *r = &g_Seq.redoStack[--g_Seq.redoCount];
    g_Seq.clipCount = r->clipCount; 
    memcpy(g_Seq.clips, r->clips, sizeof(Clip) * r->clipCount);
    seq_unlock();
}