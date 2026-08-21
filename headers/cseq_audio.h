#pragma once
#include "cseq_globals.h"
#include "cseq_dsp.h"
#include <stdio.h>

/* ============================================================
 * AUDIO CALLBACK
 * ============================================================ */
static inline void audio_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    (void)pDevice; (void)pInput;
    float* out = (float*)pOutput; 
    memset(out, 0, frameCount * NUM_CHANNELS * sizeof(float));

    static float s_masterFade = 0.0f;
    const float kRampStep = 1.0f / 128.0f;

    bool shouldPlay = g_Seq.isPlaying && (g_Seq.sampleCount > 0) && (g_Seq.clipCount > 0);

    if (!shouldPlay && s_masterFade <= 0.0001f) {
        s_masterFade = 0.0f;
        return;
    }

    const float bpm = g_Seq.bpm, swing = g_Seq.swing, fpb = frames_per_beat(bpm), maxBeats = total_beats();
    const ma_uint64 loopTotalFrames = (ma_uint64)((double)maxBeats * (double)fpb);
    const ma_uint64 startFrame = (ma_uint64)InterlockedCompareExchange(&g_Seq.playbackFrame, 0, 0);
    float trackL[MAX_TRACKS], trackR[MAX_TRACKS];

    seq_lock();

    for (ma_uint32 f = 0; f < frameCount; ++f) {
        if (shouldPlay) {
            if (s_masterFade < 1.0f) {
                s_masterFade += kRampStep;
                if (s_masterFade > 1.0f) s_masterFade = 1.0f;
            }
        } else {
            if (s_masterFade > 0.0f) {
                s_masterFade -= kRampStep;
                if (s_masterFade < 0.0f) s_masterFade = 0.0f;
            }
        }

        ma_uint64 currentFrame = (loopTotalFrames > 0) ? ((startFrame + f) % loopTotalFrames) : 0;
        float curBeat = frame_to_beat(currentFrame, bpm, swing);
        memset(trackL, 0, sizeof(trackL)); 
        memset(trackR, 0, sizeof(trackR));

        for (int i = 0; i < g_Seq.clipCount; ++i) {
            Clip* c = &g_Seq.clips[i];
            if (c->track < 0 || c->track >= g_Seq.trackCount || g_Seq.trackMuted[c->track] || c->startBeat >= maxBeats) 
                continue;

            float swungStart = apply_clip_swing(c->startBeat, swing);
            if (curBeat < swungStart || curBeat >= swungStart + c->lengthBeats) 
                continue;
            
            if (c->sampleIndex < 0 || c->sampleIndex >= g_Seq.sampleCount)
                continue;

            AudioSample* s = &g_Seq.samples[c->sampleIndex];
            if (!s->loaded || !s->pFrames || s->frameCount == 0) continue;
            
            float pRate = (c->playbackRate > 0.01f) ? c->playbackRate : 1.0f;
            float localBeat = curBeat - swungStart;
            double srcPos = (double)c->sampleOffsetFrames + (double)localBeat * fpb * pRate;
            ma_uint64 srcFrame = (ma_uint64)srcPos;
            if (srcFrame + 1 >= s->frameCount) continue;
            
            float frac = (float)(srcPos - (double)srcFrame);
            float sampleL = s->pFrames[srcFrame * 2 + 0] + (s->pFrames[(srcFrame + 1) * 2 + 0] - s->pFrames[srcFrame * 2 + 0]) * frac;
            float sampleR = s->pFrames[srcFrame * 2 + 1] + (s->pFrames[(srcFrame + 1) * 2 + 1] - s->pFrames[srcFrame * 2 + 1]) * frac;
            
            sampleL *= c->volume; 
            sampleR *= c->volume;

            // User Fade-in and Fade-out envelopes
            if (c->fadeInBeats > 0.0001f && localBeat < c->fadeInBeats) { 
                sampleL *= (localBeat / c->fadeInBeats); 
                sampleR *= (localBeat / c->fadeInBeats); 
            }
            if (c->fadeOutBeats > 0.0001f && (c->lengthBeats - localBeat) < c->fadeOutBeats) { 
                float g = (c->lengthBeats - localBeat) / c->fadeOutBeats;
                if (g < 0.0f) g = 0.0f;
                sampleL *= g; 
                sampleR *= g; 
            }

            // Anti-click micro-ramp (64 samples) at clip boundaries
            float framesFromStart = (float)(localBeat * fpb);
            float framesFromEnd = (float)((c->lengthBeats - localBeat) * fpb);
            float microFade = 1.0f;
            if (framesFromStart < (float)FADE_SAMPLES && framesFromStart >= 0.0f) {
                microFade = framesFromStart / (float)FADE_SAMPLES;
            }
            if (framesFromEnd < (float)FADE_SAMPLES && framesFromEnd >= 0.0f) {
                float endFade = framesFromEnd / (float)FADE_SAMPLES;
                if (endFade < microFade) microFade = endFade;
            }
            sampleL *= microFade;
            sampleR *= microFade;
            
            trackL[c->track] += sampleL; 
            trackR[c->track] += sampleR;
        }

        float finalL = 0.0f, finalR = 0.0f;
        for (int t = 0; t < g_Seq.trackCount && t < MAX_TRACKS; ++t) {
            if (g_Seq.trackMuted[t]) continue;
            float L = trackL[t] * g_Seq.trackVolume[t], R = trackR[t] * g_Seq.trackVolume[t];
            if (g_Seq.trackEqActive[t]) smooth_eq3_process_float(&g_Seq.trackEQ[t], &L, &R, &L, &R, 1);
            for (int b = 0; b < 3; ++b) peak_biquad_process(&g_Seq.trackPeak[t][b], &L, &R);
            finalL += L; 
            finalR += R;
        }

        if (g_Seq.isLofi) {
            g_Seq.lofiLpL += 0.18f * (finalL - g_Seq.lofiLpL); 
            g_Seq.lofiLpR += 0.18f * (finalR - g_Seq.lofiLpR);
            finalL = tanhf(g_Seq.lofiLpL * 1.4f) * 0.85f; 
            finalR = tanhf(g_Seq.lofiLpR * 1.4f) * 0.85f;
        }

        finalL *= s_masterFade;
        finalR *= s_masterFade;

        if (finalL > 1.0f) finalL = 1.0f; if (finalL < -1.0f) finalL = -1.0f;
        if (finalR > 1.0f) finalR = 1.0f; if (finalR < -1.0f) finalR = -1.0f;
        out[f * 2 + 0] = finalL; 
        out[f * 2 + 1] = finalR;
    }

    seq_unlock();

    if (shouldPlay) {
        ma_uint64 nextFrame = (loopTotalFrames > 0) ? ((startFrame + frameCount) % loopTotalFrames) : 0;
        InterlockedExchange(&g_Seq.playbackFrame, (LONG)nextFrame);
    }
}

static inline void export_timeline_to_wav(const char *outputPath) {
    DWORD startTick = GetTickCount(); float fpb = frames_per_beat(g_Seq.bpm);
    ma_uint64 totalExportFrames = (ma_uint64)((double)total_beats() * (double)fpb);
    if (totalExportFrames == 0) return;

    float *mixBuf = (float*)calloc((size_t)totalExportFrames * NUM_CHANNELS, sizeof(float));
    if (!mixBuf) { MessageBoxA(g_hWnd, "OOM during audio export buffer allocation.", "Error", MB_ICONERROR); return; }

    seq_lock();

    for (int t = 0; t < g_Seq.trackCount && t < MAX_TRACKS; ++t) {
        if (g_Seq.trackMuted[t]) continue;
        float *trkL = (float*)calloc((size_t)totalExportFrames, sizeof(float)), *trkR = (float*)calloc((size_t)totalExportFrames, sizeof(float));
        if (!trkL || !trkR) {
            free(trkL); free(trkR);
            continue;
        }
        
        for (int c = 0; c < g_Seq.clipCount; ++c) {
            Clip *clip = &g_Seq.clips[c]; 
            if (clip->track != t || clip->startBeat >= total_beats()) continue;
            if (clip->sampleIndex < 0 || clip->sampleIndex >= g_Seq.sampleCount) continue;
            AudioSample *s = &g_Seq.samples[clip->sampleIndex]; 
            if (!s->pFrames || s->frameCount == 0) continue;
            
            float pRate = (clip->playbackRate > 0.01f) ? clip->playbackRate : 1.0f;
            ma_uint64 stFrame = (ma_uint64)((double)apply_clip_swing(clip->startBeat, g_Seq.swing) * (double)fpb);
            ma_uint64 endFrame = stFrame + (ma_uint64)((double)clip->lengthBeats * (double)fpb);
            ma_uint64 inFrames = max((ma_uint64)FADE_SAMPLES, (ma_uint64)(clip->fadeInBeats * fpb));
            ma_uint64 outFrames = max((ma_uint64)FADE_SAMPLES, (ma_uint64)(clip->fadeOutBeats * fpb));

            for (ma_uint64 i = stFrame; i < endFrame && i < totalExportFrames; ++i) {
                ma_uint64 tOff = i - stFrame;
                double sPos = (double)clip->sampleOffsetFrames + (double)tOff * (double)pRate;
                if (g_Seq.isLofi) sPos = floor(sPos / 3.0) * 3.0;
                
                ma_uint64 sF0 = (ma_uint64)sPos, sF1 = sF0 + 1;
                if (sF0 < s->frameCount) {
                    float frac = g_Seq.isLofi ? 0.0f : (float)(sPos - (double)sF0);
                    float l = s->pFrames[sF0 * 2 + 0] + frac * ((sF1 < s->frameCount ? s->pFrames[sF1 * 2 + 0] : s->pFrames[sF0 * 2 + 0]) - s->pFrames[sF0 * 2 + 0]);
                    float r = s->pFrames[sF0 * 2 + 1] + frac * ((sF1 < s->frameCount ? s->pFrames[sF1 * 2 + 1] : s->pFrames[sF0 * 2 + 1]) - s->pFrames[sF0 * 2 + 1]);
                    
                    float fade = (tOff < inFrames && inFrames) ? (float)tOff / (float)inFrames : (((endFrame - stFrame) > outFrames && ((endFrame - stFrame) - tOff) < outFrames && outFrames) ? (float)((endFrame - stFrame) - tOff) / (float)outFrames : 1.0f);
                    trkL[i] += l * clip->volume * g_Seq.trackVolume[clip->track] * fade;
                    trkR[i] += r * clip->volume * g_Seq.trackVolume[clip->track] * fade;
                }
            }
        }
        
        if (g_Seq.trackEqActive[t]) {
            PeakBiquad eq[3] = { g_Seq.trackPeak[t][0], g_Seq.trackPeak[t][1], g_Seq.trackPeak[t][2] };
            SmoothEQ3 seq = g_Seq.trackEQ[t];
            peak_biquad_clear(&eq[0]); peak_biquad_clear(&eq[1]); peak_biquad_clear(&eq[2]);
            
            smooth_eq3_process_float(&seq, trkL, trkR, trkL, trkR, (uint32_t)totalExportFrames);
            
            for (ma_uint64 i = 0; i < totalExportFrames; ++i) {
                peak_biquad_process(&eq[0], &trkL[i], &trkR[i]);
                peak_biquad_process(&eq[1], &trkL[i], &trkR[i]);
                peak_biquad_process(&eq[2], &trkL[i], &trkR[i]);
            }
        }
        for (ma_uint64 i = 0; i < totalExportFrames; ++i) { 
            mixBuf[i * 2 + 0] += trkL[i]; 
            mixBuf[i * 2 + 1] += trkR[i]; 
        }
        free(trkL); 
        free(trkR);
    }

    seq_unlock();
    
    float exportLpL = 0.0f, exportLpR = 0.0f;
    for (ma_uint64 i = 0; i < totalExportFrames; ++i) {
        float mixL = mixBuf[i * 2 + 0];
        float mixR = mixBuf[i * 2 + 1];

        if (g_Seq.isLofi) {
            mixL = floorf(mixL * 2048.0f) / 2048.0f;
            mixR = floorf(mixR * 2048.0f) / 2048.0f;
            exportLpL += 0.45f * (mixL - exportLpL);
            exportLpR += 0.45f * (mixR - exportLpR);
            mixL = exportLpL;
            mixR = exportLpR;
        }

        mixBuf[i * 2 + 0] = mixL;
        mixBuf[i * 2 + 1] = mixR;
    }

    float maxPeak = 0.0f;
    for (ma_uint64 i = 0; i < totalExportFrames * NUM_CHANNELS; ++i) { 
        if (fabsf(mixBuf[i]) > maxPeak) maxPeak = fabsf(mixBuf[i]); 
    }
    float norm = (maxPeak > 0.0001f) ? (0.98f / maxPeak) : 1.0f;
    for (ma_uint64 i = 0; i < totalExportFrames * NUM_CHANNELS; ++i) mixBuf[i] = tanhf(mixBuf[i] * norm);

    ma_encoder_config cfg = ma_encoder_config_init(ma_encoding_format_wav, ma_format_f32, NUM_CHANNELS, SAMPLE_RATE);
    ma_encoder enc;
    if (ma_encoder_init_file(outputPath, &cfg, &enc) == MA_SUCCESS) {
        ma_encoder_write_pcm_frames(&enc, mixBuf, totalExportFrames, NULL);
        ma_encoder_uninit(&enc);
    }
    free(mixBuf);
    snprintf(g_Seq.exportMsg, sizeof(g_Seq.exportMsg), "Exported in %.1fs.", (float)(GetTickCount() - startTick) / 1000.0f);
    g_Seq.exportMsgActive = true; 
    g_Seq.exportMsgExpiry = GetTickCount() + 5000;
}