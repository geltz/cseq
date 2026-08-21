#pragma once
#include "cseq_globals.h"
#include <math.h>

/* Timing Conversions */
static inline float get_pixels_per_beat(void) { return PIXELS_PER_BEAT * (g_Seq.zoom > 0.05f ? g_Seq.zoom : 1.0f); }
static inline float total_beats(void) { return (float)(g_Seq.barCount * 4); }
static inline float frames_per_beat(float bpm) { return (float)SAMPLE_RATE * (60.0f / (bpm > 1.0f ? bpm : 1.0f)); }
static inline float quantize_beat_16th(float beat) { return floorf(beat * 4.0f + 0.5f) / 4.0f; }

static inline float apply_clip_swing(float beat, float swing) {
    if (swing <= 0.001f) return beat;
    float pair = floorf(beat * 2.0f) * 0.5f, off = beat - pair;
    if (off >= 0.10f && off < 0.40f) return pair + off + (swing * 0.1667f);
    return beat;
}

static inline float frame_to_beat(ma_uint64 frame, float bpm, float swing) {
    (void)swing;
    float fpb = frames_per_beat(bpm);
    return (fpb > 0.0001f) ? (float)((double)frame / (double)fpb) : 0.0f;
}

static inline ma_uint64 beat_to_frame(float beat, float bpm, float swing) {
    if (beat < 0.0f) beat = 0.0f;
    float fpb = frames_per_beat(bpm), sub = beat - floorf(beat), shift = 0.0f;
    if (fabsf(sub - 0.25f) < 0.05f || fabsf(sub - 0.75f) < 0.05f) shift = swing * 0.12f;
    else if (fabsf(sub - 0.50f) < 0.05f) shift = swing * 0.16f;
    return (ma_uint64)((double)(beat + shift) * (double)fpb);
}

/* BiQuad Filtering */
static inline void peak_biquad_clear(PeakBiquad *b) {
    b->z1L = b->z2L = b->z1R = b->z2R = 0.0f;
}

static inline void peak_biquad_set(PeakBiquad *b, float freqHz, float Q, float gainDb, float sampleRate) {
    if (Q < 0.1f) Q = 0.1f; if (Q > 8.0f) Q = 8.0f;
    if (freqHz < 20.0f) freqHz = 20.0f; if (freqHz > sampleRate * 0.45f) freqHz = sampleRate * 0.45f;
    
    float A = powf(10.0f, gainDb / 40.0f), w0 = 2.0f * 3.14159265f * freqHz / sampleRate;
    float alpha = sinf(w0) / (2.0f * Q), cosw0 = cosf(w0);
    float a0 = 1.0f + alpha / A;
    
    b->b0 = (1.0f + alpha * A) / a0;
    b->b1 = (-2.0f * cosw0) / a0;
    b->b2 = (1.0f - alpha * A) / a0;
    b->a1 = (-2.0f * cosw0) / a0;
    b->a2 = (1.0f - alpha / A) / a0;
}

static inline void peak_biquad_process(PeakBiquad *b, float *L, float *R) {
    float inL = *L, inR = *R;
    float outL = b->b0 * inL + b->z1L;
    b->z1L = b->b1 * inL - b->a1 * outL + b->z2L;
    b->z2L = b->b2 * inL - b->a2 * outL;
    float outR = b->b0 * inR + b->z1R;
    b->z1R = b->b1 * inR - b->a1 * outR + b->z2R;
    b->z2R = b->b2 * inR - b->a2 * outR;
    *L = outL; *R = outR;
}