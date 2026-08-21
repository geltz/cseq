#pragma once
#include "cseq_globals.h"
#include "cseq_dsp.h"
#include "cseq_ui.h"
#include "cseq_audio.h"
#include "cseq_state.h"
#include "cseq_actions.h"
#include "cseq_dialogs.h"
#include "cseq_project.h"
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <commdlg.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

/* ============================================================
 * TOPBAR CLICK DISPATCHER
 * ============================================================ */
static inline bool handle_topbar_click(HWND hwnd, int mx, int btnMode) {
    for (int b = 0; b < TOPBAR_SLOT_COUNT; ++b) {
        int bx = 0, bw = 0;
        get_topbar_slot_bounds(NULL, b, &bx, &bw);

        if (mx >= bx && mx <= bx + bw) {
        switch (b) {
            case 0:
                if (btnMode == 2) {
                    g_Seq.isPlaying = false;
                    InterlockedExchange(&g_Seq.playbackFrame, 0);
                } else {
                    toggle_playback();
                }
                break;
            case 1:
                if (btnMode == 0) {
                    g_Seq.bpm += 2.0f;
                    if (g_Seq.bpm > 300.0f) g_Seq.bpm = 40.0f;
                } else if (btnMode == 1) {
                    g_Seq.bpm -= 2.0f;
                    if (g_Seq.bpm < 40.0f) g_Seq.bpm = 300.0f;
                } else {
                    g_Seq.bpm = 120.0f;
                }
                break;
            case 2:
                if (btnMode == 0) change_bar_count(1);
                else if (btnMode == 1) change_bar_count(-1);
                else change_bar_count(4 - g_Seq.barCount);
                break;
            case 3:
                if (btnMode == 0) {
                    g_Seq.swing += 0.05f;
                    if (g_Seq.swing > 0.75f) g_Seq.swing = 0.0f;
                } else if (btnMode == 1) {
                    g_Seq.swing -= 0.05f;
                    if (g_Seq.swing < 0.0f) g_Seq.swing = 0.0f;
                    if (g_Seq.swing > 0.75f) g_Seq.swing = 0.75f;
                } else {
                    g_Seq.swing = 0.0f;
                }
                break;
            case 4:
                if (btnMode == 2) g_Seq.quantizeEnabled = true;
                else g_Seq.quantizeEnabled = !g_Seq.quantizeEnabled;
                break;
            case 5:
                if (btnMode == 2) g_Seq.playFromStartOnPlay = false;
                else g_Seq.playFromStartOnPlay = !g_Seq.playFromStartOnPlay;
                break;
            case 6:
                if (btnMode == 2) g_Seq.isLofi = false;
                else g_Seq.isLofi = !g_Seq.isLofi;
                break;
            case 7: {
                LONG currentFrame = InterlockedCompareExchange(&g_Seq.playbackFrame, 0, 0);
                float curBeat = frame_to_beat((ma_uint64)currentFrame, g_Seq.bpm, g_Seq.swing);
                open_sample_dialog(hwnd, 0, quantize_beat_16th(curBeat));
                break;
            }
            case 8:
                SendMessageA(hwnd, WM_KEYDOWN, 'E', 0);
                break;
            case 9:
                save_project_dialog(hwnd);
                break;
            case 10:
                load_project_dialog(hwnd);
                break;
            case 11:
                open_keybinds_dialog(hwnd);
                break;
            }
            InvalidateRect(hwnd, NULL, FALSE);
            return true;
        }
    }
    return false;
}

/* ============================================================
 * MAIN EVENT & WINDOW PROCEDURE HANDLER
 * ============================================================ */
static inline LRESULT cseq_main_wndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    
    if (g_Seq.isSaving) {
        if (msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN ||
            msg == WM_KEYDOWN || msg == WM_DROPFILES || msg == WM_MOUSEWHEEL) {
            return 0; // Lock user actions until saving completes
        }
    }

    switch (msg) {
    case WM_ERASEBKGND:
        return 1;

    case WM_CREATE: {
        DragAcceptFiles(hwnd, TRUE);
        SetTimer(hwnd, 1, 16, NULL);
        update_scrollbar(hwnd);

        HICON hIconBig = (HICON)LoadImageA(GetModuleHandle(NULL), MAKEINTRESOURCEA(1), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
        HICON hIconSmall = (HICON)LoadImageA(GetModuleHandle(NULL), MAKEINTRESOURCEA(1), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

        if (!hIconBig) hIconBig = create_app_icon(32);
        if (!hIconSmall) hIconSmall = create_app_icon(16);

        SendMessageA(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIconBig);
        SendMessageA(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSmall);
        return 0;
    }

    case WM_SIZE: {
        update_scrollbar(hwnd);
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_TIMER:
        if (wParam == 1) {
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;

    case WM_VSCROLL: {
        SCROLLINFO si = {0};
        si.cbSize = sizeof(si);
        si.fMask = SIF_ALL;
        GetScrollInfo(hwnd, SB_VERT, &si);

        int oldPos = g_Seq.scrollY;
        switch (LOWORD(wParam)) {
        case SB_LINEUP:
            g_Seq.scrollY -= TRACK_HEIGHT / 2;
            break;
        case SB_LINEDOWN:
            g_Seq.scrollY += TRACK_HEIGHT / 2;
            break;
        case SB_PAGEUP:
            g_Seq.scrollY -= si.nPage;
            break;
        case SB_PAGEDOWN:
            g_Seq.scrollY += si.nPage;
            break;
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
            g_Seq.scrollY = HIWORD(wParam);
            break;
        }

        update_scrollbar(hwnd);
        if (g_Seq.scrollY != oldPos) {
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_MOUSEWHEEL: {
        int mx = GET_X_LPARAM(lParam);
        int my = GET_Y_LPARAM(lParam);
        POINT pt = {mx, my};
        ScreenToClient(hwnd, &pt);

        short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
        float deltaVol = (zDelta > 0) ? 0.05f : -0.05f;

        if (pt.y <= 28) {
            for (int b = 1; b <= 3; ++b) {
                int bx = 0, bw = 0;
                get_topbar_slot_bounds(NULL, b, &bx, &bw);
                if (pt.x >= bx && pt.x <= bx + bw) {
                    if (b == 1) {
                        g_Seq.bpm += (zDelta > 0) ? 2.0f : -2.0f;
                        if (g_Seq.bpm < 40.0f) g_Seq.bpm = 40.0f;
                        if (g_Seq.bpm > 300.0f) g_Seq.bpm = 300.0f;
                    } else if (b == 2) {
                        change_bar_count((zDelta > 0) ? 1 : -1);
                    } else if (b == 3) {
                        g_Seq.swing += (zDelta > 0) ? 0.05f : -0.05f;
                        if (g_Seq.swing < 0.0f) g_Seq.swing = 0.0f;
                        if (g_Seq.swing > 0.75f) g_Seq.swing = 0.75f;
                    }
                    InvalidateRect(hwnd, NULL, FALSE);
                    return 0;
                }
            }
        }

        RECT rcClient;
        GetClientRect(hwnd, &rcClient);
        int clientH = rcClient.bottom - rcClient.top;
        int clientW = rcClient.right - rcClient.left;

        if (pt.y >= clientH - BOTTOM_DOCK_HEIGHT) {
            float ppb = get_pixels_per_beat();
            int totalTimelineWidth = (int)(total_beats() * ppb);
            int visibleWidth = clientW - TRACK_HEADER_WIDTH;
            int maxScrollX = totalTimelineWidth - visibleWidth;
            if (maxScrollX < 0) maxScrollX = 0;

            int scrollStep = (int)ppb;
            g_Seq.scrollX += (zDelta > 0) ? -scrollStep : scrollStep;
            if (g_Seq.scrollX < 0) g_Seq.scrollX = 0;
            if (g_Seq.scrollX > maxScrollX) g_Seq.scrollX = maxScrollX;

            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        if (pt.y > HEADER_HEIGHT && pt.x < TRACK_HEADER_WIDTH) {
            int track = (pt.y - HEADER_HEIGHT + g_Seq.scrollY) / TRACK_HEIGHT;
            if (track >= 0 && track < g_Seq.trackCount && track < MAX_TRACKS) {
                seq_lock();
                g_Seq.trackVolume[track] += deltaVol;
                if (g_Seq.trackVolume[track] < 0.0f) g_Seq.trackVolume[track] = 0.0f;
                if (g_Seq.trackVolume[track] > 1.0f) g_Seq.trackVolume[track] = 1.0f;
                seq_unlock();
            }
        } else {
            int clipIdx = get_clip_under_mouse(pt.x, pt.y);
            if (clipIdx != -1) {
                bool shiftHeld = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                seq_lock();
                if (clipIdx >= 0 && clipIdx < g_Seq.clipCount) {
                    if (shiftHeld) {
                        float deltaRate = (zDelta > 0) ? 0.05f : -0.05f;
                        if (g_Seq.clips[clipIdx].isSelected) {
                            for (int i = 0; i < g_Seq.clipCount; ++i) {
                                if (g_Seq.clips[i].isSelected) {
                                    g_Seq.clips[i].playbackRate += deltaRate;
                                    if (g_Seq.clips[i].playbackRate < 0.01f) g_Seq.clips[i].playbackRate = 0.01f;
                                    if (g_Seq.clips[i].playbackRate > 2.00f) g_Seq.clips[i].playbackRate = 2.00f;
                                }
                            }
                        } else {
                            g_Seq.clips[clipIdx].playbackRate += deltaRate;
                            if (g_Seq.clips[clipIdx].playbackRate < 0.01f) g_Seq.clips[clipIdx].playbackRate = 0.01f;
                            if (g_Seq.clips[clipIdx].playbackRate > 2.00f) g_Seq.clips[clipIdx].playbackRate = 2.00f;
                        }
                    } else if (g_Seq.clips[clipIdx].isSelected) {
                        for (int i = 0; i < g_Seq.clipCount; ++i) {
                            if (g_Seq.clips[i].isSelected) {
                                g_Seq.clips[i].volume += deltaVol;
                                if (g_Seq.clips[i].volume < 0.0f) g_Seq.clips[i].volume = 0.0f;
                                if (g_Seq.clips[i].volume > 2.0f) g_Seq.clips[i].volume = 2.0f;
                            }
                        }
                        g_Seq.volumePopupClip = clipIdx;
                        g_Seq.volumePopupExpiry = GetTickCount() + 1200;
                    } else {
                        g_Seq.clips[clipIdx].volume += deltaVol;
                        if (g_Seq.clips[clipIdx].volume < 0.0f) g_Seq.clips[clipIdx].volume = 0.0f;
                        if (g_Seq.clips[clipIdx].volume > 2.0f) g_Seq.clips[clipIdx].volume = 2.0f;

                        g_Seq.volumePopupClip = clipIdx;
                        g_Seq.volumePopupExpiry = GetTickCount() + 1200;
                    }
                }
                seq_unlock();
            } else {
                g_Seq.scrollY -= (zDelta / WHEEL_DELTA) * (TRACK_HEIGHT / 2);
                update_scrollbar(hwnd);
            }
        }
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_DROPFILES: {
        HDROP hDrop = (HDROP)wParam;
        POINT pt;
        DragQueryPoint(hDrop, &pt);

        int track = (pt.y - HEADER_HEIGHT + g_Seq.scrollY) / TRACK_HEIGHT;
        if (track < 0) track = 0;
        if (track >= g_Seq.trackCount) track = g_Seq.trackCount - 1;

        float ppb = get_pixels_per_beat();
        float dropBeat = (float)(pt.x - TRACK_HEADER_WIDTH + g_Seq.scrollX) / ppb;
        if (dropBeat < 0.0f) dropBeat = 0.0f;
        dropBeat = quantize_beat_16th(dropBeat);

        UINT fileCount = DragQueryFileA(hDrop, 0xFFFFFFFF, NULL, 0);
        for (UINT i = 0; i < fileCount; ++i) {
            char filepath[MAX_PATH];
            if (DragQueryFileA(hDrop, i, filepath, MAX_PATH)) {
                int sampleIdx = load_audio_file(filepath);
                if (sampleIdx != -1) {
                    add_clip(sampleIdx, track, dropBeat);
                    dropBeat += 1.0f;
                }
            }
        }
        DragFinish(hDrop);
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_KILLFOCUS: {
        if (g_Seq.isDraggingClip || g_Seq.isVolumeDragging || g_Seq.isSlipDragging ||
            g_Seq.isMarqueeSelecting || g_Seq.isResizingLeft || g_Seq.isResizingRight ||
            g_Seq.isFadeInDragging || g_Seq.isFadeOutDragging || g_Seq.isMiddlePanning) {
            g_Seq.isDraggingClip = false;
            g_Seq.isCtrlDuplicating = false;
            g_Seq.isVolumeDragging = false;
            g_Seq.isSlipDragging = false;
            g_Seq.isMarqueeSelecting = false;
            g_Seq.isResizingLeft = false;
            g_Seq.isResizingRight = false;
            g_Seq.isFadeInDragging = false;
            g_Seq.isFadeOutDragging = false;
            g_Seq.isMiddlePanning = false;
            g_Seq.draggedClipIndex = -1;
            if (GetCapture() == hwnd) ReleaseCapture();
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_MBUTTONDOWN: {
        int mx = GET_X_LPARAM(lParam);
        int my = GET_Y_LPARAM(lParam);

        if (my <= 28) {
            if (handle_topbar_click(hwnd, mx, 2)) return 0;
        }

        if (my > HEADER_HEIGHT && mx < TRACK_HEADER_WIDTH) {
            int track = (my - HEADER_HEIGHT + g_Seq.scrollY) / TRACK_HEIGHT;
            if (track >= 0 && track < g_Seq.trackCount) {
                LONG curFrame = InterlockedCompareExchange(&g_Seq.playbackFrame, 0, 0);
                float curBeat = frame_to_beat((ma_uint64)curFrame, g_Seq.bpm, g_Seq.swing);
                open_sample_dialog(hwnd, track, quantize_beat_16th(curBeat));
            }
            return 0;
        }

        if (my > HEADER_HEIGHT && mx >= TRACK_HEADER_WIDTH) {
            g_Seq.isMiddlePanning = true;
            g_Seq.panStartX = mx;
            g_Seq.panStartY = my;
            g_Seq.panOrigScrollX = g_Seq.scrollX;
            g_Seq.panOrigScrollY = g_Seq.scrollY;
            SetCapture(hwnd);
            SetCursor(LoadCursor(NULL, IDC_SIZEALL));
        }
        return 0;
    }

    case WM_MBUTTONUP: {
        if (g_Seq.isMiddlePanning) {
            g_Seq.isMiddlePanning = false;
            ReleaseCapture();
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_LBUTTONDOWN: {
        int mx = GET_X_LPARAM(lParam);
        int my = GET_Y_LPARAM(lParam);

        if (my <= 28) {
            if (handle_topbar_click(hwnd, mx, 0)) return 0;
        }

        if (my > HEADER_HEIGHT) {
            RECT rcClient;
            GetClientRect(hwnd, &rcClient);
            int clientH = rcClient.bottom - rcClient.top;
            int clientW = rcClient.right - rcClient.left;

            if (fabsf(g_Seq.zoom - 1.0f) > 0.001f) {
                int zoomBtnW = 86, zoomBtnH = 22;
                int zoomX = clientW - zoomBtnW - 14;
                int zoomY = clientH - 33;
                if (mx >= zoomX && mx <= zoomX + zoomBtnW && my >= zoomY && my <= zoomY + zoomBtnH) {
                    g_Seq.zoom = 1.0f;
                    update_scrollbar(hwnd);
                    InvalidateRect(hwnd, NULL, FALSE);
                    return 0;
                }
            }

            int btnY = clientH - 38;
            if (my >= btnY && my <= btnY + 20) {
                if (mx >= 8 && mx <= 41) {
                    add_track_action(hwnd);
                    return 0;
                }
                if (mx >= 44 && mx <= 77) {
                    remove_track_action(hwnd);
                    return 0;
                }
            }

            int clipIdx = get_clip_under_mouse(mx, my);
            float ppb = get_pixels_per_beat();

            if (mx < (TRACK_HEADER_WIDTH - 6) && clipIdx == -1) {
                int track = (my - HEADER_HEIGHT + g_Seq.scrollY) / TRACK_HEIGHT;
                if (track >= 0 && track < g_Seq.trackCount) {
                    if (GetKeyState(VK_SHIFT) & 0x8000) {
                        select_all_clips_on_track(track);
                    } else {
                        seq_lock();
                        g_Seq.trackMuted[track] = !g_Seq.trackMuted[track];
                        seq_unlock();
                    }
                    InvalidateRect(hwnd, NULL, FALSE);
                }
                return 0;
            }

            if (clipIdx == -1 && mx >= (TRACK_HEADER_WIDTH - 6)) {
                float clickedBeat = (float)(mx - TRACK_HEADER_WIDTH + g_Seq.scrollX) / ppb;
                if (clickedBeat < 0.0f) clickedBeat = 0.0f;
                if (clickedBeat > total_beats()) clickedBeat = total_beats();
                ma_uint64 newFrame = beat_to_frame(clickedBeat, g_Seq.bpm, g_Seq.swing);
                InterlockedExchange(&g_Seq.playbackFrame, (LONG)newFrame);

                g_Seq.isMarqueeSelecting = true;
                g_Seq.marqueeStartX = mx;
                g_Seq.marqueeStartY = my;
                g_Seq.marqueeCurX = mx;
                g_Seq.marqueeCurY = my;

                if (!(GetKeyState(VK_SHIFT) & 0x8000) && !(GetKeyState(VK_CONTROL) & 0x8000)) {
                    deselect_all_clips();
                }
                SetCapture(hwnd);
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }

            if (clipIdx != -1) {
                seq_lock();
                if (clipIdx < 0 || clipIdx >= g_Seq.clipCount) {
                    seq_unlock();
                    return 0;
                }
                Clip *c = &g_Seq.clips[clipIdx];
                int cX1 = TRACK_HEADER_WIDTH - g_Seq.scrollX + (int)(c->startBeat * ppb);
                int cX2 = cX1 + (int)(c->lengthBeats * ppb);
                int cY1 = HEADER_HEIGHT - g_Seq.scrollY + c->track * TRACK_HEIGHT;

                bool ctrlHeld = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                bool shiftHeld = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                bool altHeld = (GetKeyState(VK_MENU) & 0x8000) != 0;

                if (ctrlHeld) {
                    c->isSelected = true;
                    g_Seq.isCtrlDuplicating = true;
                } else if (!shiftHeld && !altHeld) {
                    if (!c->isSelected) {
                        for (int i = 0; i < g_Seq.clipCount; ++i) g_Seq.clips[i].isSelected = false;
                        c->isSelected = true;
                    }
                } else if (!c->isSelected) {
                    c->isSelected = true;
                }

                int fadeInX = cX1 + (int)(c->fadeInBeats * ppb);
                int fadeOutX = cX2 - (int)(c->fadeOutBeats * ppb);

                bool onFadeIn = (abs(mx - fadeInX) <= 22 && (my - cY1) <= 32 && my >= cY1 - 4);
                bool onFadeOut = (abs(mx - fadeOutX) <= 22 && (my - cY1) <= 32 && my >= cY1 - 4);

                if (onFadeIn || onFadeOut) {
                    seq_unlock();
                    push_undo_state();
                    seq_lock();
                    if (onFadeIn) {
                        for (int i = 0; i < g_Seq.clipCount; ++i) {
                            if (g_Seq.clips[i].isSelected) {
                                g_Seq.clips[i].dragStartBeatOrig = g_Seq.clips[i].fadeInBeats;
                            }
                        }
                        g_Seq.isFadeInDragging = true;
                    } else {
                        for (int i = 0; i < g_Seq.clipCount; ++i) {
                            if (g_Seq.clips[i].isSelected) {
                                g_Seq.clips[i].dragStartBeatOrig = g_Seq.clips[i].fadeOutBeats;
                            }
                        }
                        g_Seq.isFadeOutDragging = true;
                    }
                    g_Seq.draggedClipIndex = clipIdx;
                    g_Seq.dragStartX = mx;
                    g_Seq.dragStartY = my;
                    seq_unlock();
                    SetCapture(hwnd);
                    return 0;
                }

                float clickedBeat = (float)(mx - TRACK_HEADER_WIDTH + g_Seq.scrollX) / ppb;
                if (clickedBeat < 0.0f) clickedBeat = 0.0f;
                if (clickedBeat > total_beats()) clickedBeat = total_beats();
                ma_uint64 newFrame = beat_to_frame(clickedBeat, g_Seq.bpm, g_Seq.swing);
                InterlockedExchange(&g_Seq.playbackFrame, (LONG)newFrame);

                g_Seq.dragStartX = mx;
                g_Seq.dragStartY = my;
                g_Seq.hasMovedPastThreshold = false;
                g_Seq.draggedClipIndex = clipIdx;

                if (mx <= cX1 + 7) {
                    g_Seq.isResizingLeft = true;
                    g_Seq.resizeOrigStartBeat = c->startBeat;
                    g_Seq.resizeOrigLengthBeats = c->lengthBeats;
                    g_Seq.resizeOrigOffsetFrames = c->sampleOffsetFrames;
                    seq_unlock();
                    SetCapture(hwnd);
                    return 0;
                } else if (mx >= cX2 - 7) {
                    g_Seq.isResizingRight = true;
                    g_Seq.resizeOrigStartBeat = c->startBeat;
                    g_Seq.resizeOrigLengthBeats = c->lengthBeats;
                    seq_unlock();
                    SetCapture(hwnd);
                    return 0;
                }

                if (altHeld) {
                    g_Seq.isSlipDragging = true;
                    g_Seq.dragStartSampleOffset = c->sampleOffsetFrames;
                    for (int i = 0; i < g_Seq.clipCount; ++i) {
                        if (g_Seq.clips[i].isSelected) {
                            g_Seq.clips[i].dragStartOffsetOrig = g_Seq.clips[i].sampleOffsetFrames;
                        }
                    }
                } else if (shiftHeld) {
                    g_Seq.isVolumeDragging = true;
                    g_Seq.dragStartVolume = c->volume;
                    g_Seq.volumePopupClip = clipIdx;
                    g_Seq.volumePopupExpiry = GetTickCount() + 1500;
                } else {
                    g_Seq.isDraggingClip = true;
                    g_Seq.dragStartBeatOffset = clickedBeat - c->startBeat;
                    g_Seq.dragOrigTrack = c->track;

                    for (int i = 0; i < g_Seq.clipCount; ++i) {
                        if (g_Seq.clips[i].isSelected) {
                            g_Seq.clips[i].dragStartBeatOrig = g_Seq.clips[i].startBeat;
                            g_Seq.clips[i].dragStartTrackOrig = g_Seq.clips[i].track;
                        }
                    }
                }
                seq_unlock();
                SetCapture(hwnd);
            }
        }
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_RBUTTONDOWN: {
        int mx = GET_X_LPARAM(lParam);
        int my = GET_Y_LPARAM(lParam);

        if (my <= 28) {
            if (handle_topbar_click(hwnd, mx, 1)) return 0;
        }

        if (my > HEADER_HEIGHT) {
            if (mx < TRACK_HEADER_WIDTH) {
                POINT screenPt = {mx, my};
                ClientToScreen(hwnd, &screenPt);
                int track = (my - HEADER_HEIGHT + g_Seq.scrollY) / TRACK_HEIGHT;
                if (track >= 0 && track < g_Seq.trackCount) {
                    show_track_context_menu(hwnd, track, screenPt.x, screenPt.y);
                }
                return 0;
            }

            g_Seq.dragStartX = mx;
            g_Seq.dragStartY = my;
            g_Seq.hasMovedPastThreshold = false;

            g_Seq.isMarqueeSelecting = true;
            g_Seq.marqueeStartX = mx;
            g_Seq.marqueeStartY = my;
            g_Seq.marqueeCurX = mx;
            g_Seq.marqueeCurY = my;

            if (!(GetKeyState(VK_SHIFT) & 0x8000) && !(GetKeyState(VK_CONTROL) & 0x8000)) {
                deselect_all_clips();
            }
            SetCapture(hwnd);
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        return 0;
    }

    case WM_RBUTTONUP: {
        int mx = GET_X_LPARAM(lParam);
        int my = GET_Y_LPARAM(lParam);

        if (g_Seq.isMarqueeSelecting) {
            g_Seq.isMarqueeSelecting = false;
            ReleaseCapture();

            if (!g_Seq.hasMovedPastThreshold && my > HEADER_HEIGHT && mx >= TRACK_HEADER_WIDTH) {
                int clipIdx = get_clip_under_mouse(mx, my);
                if (clipIdx != -1) {
                    deselect_all_clips();
                    seq_lock();
                    if (clipIdx >= 0 && clipIdx < g_Seq.clipCount) {
                        g_Seq.clips[clipIdx].isSelected = true;
                    }
                    seq_unlock();
                    POINT screenPt = {mx, my};
                    ClientToScreen(hwnd, &screenPt);
                    show_clip_context_menu(hwnd, clipIdx, screenPt.x, screenPt.y);
                }
            }
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_MOUSEMOVE: {
        int mx = GET_X_LPARAM(lParam);
        int my = GET_Y_LPARAM(lParam);

        TRACKMOUSEEVENT tme = {sizeof(TRACKMOUSEEVENT), TME_LEAVE, hwnd, 0};
        TrackMouseEvent(&tme);

        g_Seq.mouseX = mx;
        g_Seq.mouseY = my;

        if (g_Seq.isMiddlePanning) {
            int dx = mx - g_Seq.panStartX;
            int dy = my - g_Seq.panStartY;
            g_Seq.scrollX = g_Seq.panOrigScrollX - dx;
            g_Seq.scrollY = g_Seq.panOrigScrollY - dy;
            update_scrollbar(hwnd);
            SetCursor(LoadCursor(NULL, IDC_SIZEALL));
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        float ppb = get_pixels_per_beat();

        if (g_Seq.isDraggingClip || g_Seq.isVolumeDragging || g_Seq.isSlipDragging ||
            g_Seq.isResizingLeft || g_Seq.isResizingRight || g_Seq.isMarqueeSelecting ||
            g_Seq.isFadeInDragging || g_Seq.isFadeOutDragging) {
            g_Seq.hoveredClip = g_Seq.draggedClipIndex;
        } else {
            g_Seq.hoveredClip = get_clip_under_mouse(mx, my);
        }

        if (g_Seq.isFadeInDragging && g_Seq.draggedClipIndex >= 0) {
            seq_lock();
            if (g_Seq.draggedClipIndex < g_Seq.clipCount) {
                Clip *lead = &g_Seq.clips[g_Seq.draggedClipIndex];
                int cX1 = TRACK_HEADER_WIDTH - g_Seq.scrollX + (int)(lead->startBeat * ppb);
                float newFadeBeats = (float)(mx - cX1) / ppb;
                if (newFadeBeats < 0.0f) newFadeBeats = 0.0f;
                if (newFadeBeats > lead->lengthBeats - lead->fadeOutBeats)
                    newFadeBeats = lead->lengthBeats - lead->fadeOutBeats;
                if (newFadeBeats > lead->lengthBeats) newFadeBeats = lead->lengthBeats;

                float delta = newFadeBeats - lead->dragStartBeatOrig;
                for (int i = 0; i < g_Seq.clipCount; ++i) {
                    if (g_Seq.clips[i].isSelected) {
                        Clip *c = &g_Seq.clips[i];
                        float newVal = c->dragStartBeatOrig + delta;
                        if (newVal < 0.0f) newVal = 0.0f;
                        if (newVal > c->lengthBeats - c->fadeOutBeats)
                            newVal = c->lengthBeats - c->fadeOutBeats;
                        if (newVal > c->lengthBeats) newVal = c->lengthBeats;
                        c->fadeInBeats = newVal;
                    }
                }
            }
            seq_unlock();
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        if (g_Seq.isFadeOutDragging && g_Seq.draggedClipIndex >= 0) {
            seq_lock();
            if (g_Seq.draggedClipIndex < g_Seq.clipCount) {
                Clip *lead = &g_Seq.clips[g_Seq.draggedClipIndex];
                int cX2 = TRACK_HEADER_WIDTH - g_Seq.scrollX + (int)((lead->startBeat + lead->lengthBeats) * ppb);
                float newFadeBeats = (float)(cX2 - mx) / ppb;
                if (newFadeBeats < 0.0f) newFadeBeats = 0.0f;
                if (newFadeBeats > lead->lengthBeats - lead->fadeInBeats)
                    newFadeBeats = lead->lengthBeats - lead->fadeInBeats;
                if (newFadeBeats > lead->lengthBeats) newFadeBeats = lead->lengthBeats;

                float delta = newFadeBeats - lead->dragStartBeatOrig;
                for (int i = 0; i < g_Seq.clipCount; ++i) {
                    if (g_Seq.clips[i].isSelected) {
                        Clip *c = &g_Seq.clips[i];
                        float newVal = c->dragStartBeatOrig + delta;
                        if (newVal < 0.0f) newVal = 0.0f;
                        if (newVal > c->lengthBeats - c->fadeInBeats)
                            newVal = c->lengthBeats - c->fadeInBeats;
                        if (newVal > c->lengthBeats) newVal = c->lengthBeats;
                        c->fadeOutBeats = newVal;
                    }
                }
            }
            seq_unlock();
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        if (g_Seq.hoveredClip != -1 && !g_Seq.isDraggingClip && !g_Seq.isMarqueeSelecting) {
            seq_lock();
            if (g_Seq.hoveredClip >= 0 && g_Seq.hoveredClip < g_Seq.clipCount) {
                Clip *c = &g_Seq.clips[g_Seq.hoveredClip];
                int cX1 = TRACK_HEADER_WIDTH - g_Seq.scrollX + (int)(c->startBeat * ppb);
                int cX2 = cX1 + (int)(c->lengthBeats * ppb);
                int cY1 = HEADER_HEIGHT - g_Seq.scrollY + c->track * TRACK_HEIGHT;
                int inApexX = cX1 + (int)(c->fadeInBeats * ppb);
                int outApexX = cX2 - (int)(c->fadeOutBeats * ppb);
                seq_unlock();

                bool nearIn = (abs(mx - inApexX) <= 20 && (my - cY1) <= 32 && my >= cY1 - 4);
                bool nearOut = (abs(mx - outApexX) <= 20 && (my - cY1) <= 32 && my >= cY1 - 4);

                if (nearIn || nearOut) {
                    SetCursor(LoadCursor(NULL, IDC_HAND));
                } else if (mx <= cX1 + 7 || mx >= cX2 - 7) {
                    SetCursor(LoadCursor(NULL, IDC_SIZEWE));
                } else {
                    SetCursor(LoadCursor(NULL, IDC_ARROW));
                }
            } else {
                seq_unlock();
            }
        }

        if (g_Seq.isMarqueeSelecting) {
            g_Seq.marqueeCurX = mx;
            g_Seq.marqueeCurY = my;
            int mX1 = min(g_Seq.marqueeStartX, mx);
            int mX2 = max(g_Seq.marqueeStartX, mx);
            int mY1 = min(g_Seq.marqueeStartY, my);
            int mY2 = max(g_Seq.marqueeStartY, my);

            seq_lock();
            for (int i = 0; i < g_Seq.clipCount; ++i) {
                Clip *c = &g_Seq.clips[i];
                if (c->startBeat >= total_beats()) continue;
                int cX1 = TRACK_HEADER_WIDTH - g_Seq.scrollX + (int)(c->startBeat * ppb);
                int cX2 = cX1 + (int)(c->lengthBeats * ppb);
                int cY1 = HEADER_HEIGHT - g_Seq.scrollY + c->track * TRACK_HEIGHT;
                int cY2 = cY1 + TRACK_HEIGHT;

                c->isSelected = !(cX2 < mX1 || cX1 > mX2 || cY2 < mY1 || cY1 > mY2);
            }
            seq_unlock();
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        if (g_Seq.isResizingLeft && g_Seq.draggedClipIndex >= 0) {
            seq_lock();
            if (g_Seq.draggedClipIndex < g_Seq.clipCount) {
                Clip *c = &g_Seq.clips[g_Seq.draggedClipIndex];
                if (c->sampleIndex >= 0 && c->sampleIndex < g_Seq.sampleCount) {
                    AudioSample *s = &g_Seq.samples[c->sampleIndex];
                    float mouseBeat = (float)(mx - TRACK_HEADER_WIDTH + g_Seq.scrollX) / ppb;
                    if (g_Seq.quantizeEnabled) mouseBeat = quantize_beat_16th(mouseBeat);

                    float maxStart = g_Seq.resizeOrigStartBeat + g_Seq.resizeOrigLengthBeats - MIN_CLIP_LENGTH_BEATS;
                    if (mouseBeat > maxStart) mouseBeat = maxStart;
                    if (mouseBeat < 0.0f) mouseBeat = 0.0f;

                    float deltaBeats = mouseBeat - g_Seq.resizeOrigStartBeat;
                    c->startBeat = mouseBeat;
                    c->lengthBeats = g_Seq.resizeOrigLengthBeats - deltaBeats;

                    if (c->fadeInBeats > c->lengthBeats) c->fadeInBeats = c->lengthBeats;
                    if (c->fadeOutBeats > c->lengthBeats) c->fadeOutBeats = c->lengthBeats;

                    float fpb = frames_per_beat(g_Seq.bpm);
                    float pRate = (c->playbackRate > 0.01f) ? c->playbackRate : 1.0f;
                    long long newOffset = (long long)g_Seq.resizeOrigOffsetFrames + (long long)(deltaBeats * fpb * pRate);
                    if (newOffset < 0) newOffset = 0;
                    c->sampleOffsetFrames = find_nearest_zero_crossing(s, (ma_uint64)newOffset, 128);
                }
            }
            seq_unlock();
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        if (g_Seq.isResizingRight && g_Seq.draggedClipIndex >= 0) {
            seq_lock();
            if (g_Seq.draggedClipIndex < g_Seq.clipCount) {
                Clip *c = &g_Seq.clips[g_Seq.draggedClipIndex];
                float mouseBeat = (float)(mx - TRACK_HEADER_WIDTH + g_Seq.scrollX) / ppb;
                if (g_Seq.quantizeEnabled) mouseBeat = quantize_beat_16th(mouseBeat);

                float newLen = mouseBeat - c->startBeat;
                if (newLen < MIN_CLIP_LENGTH_BEATS) newLen = MIN_CLIP_LENGTH_BEATS;
                c->lengthBeats = newLen;

                if (c->fadeInBeats > c->lengthBeats) c->fadeInBeats = c->lengthBeats;
                if (c->fadeOutBeats > c->lengthBeats) c->fadeOutBeats = c->lengthBeats;
            }
            seq_unlock();
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        if (!g_Seq.hasMovedPastThreshold) {
            if (abs(mx - g_Seq.dragStartX) > 4 || abs(my - g_Seq.dragStartY) > 4) {
                g_Seq.hasMovedPastThreshold = true;
                push_undo_state();
                if (g_Seq.isDraggingClip && g_Seq.isCtrlDuplicating) {
                    seq_lock();
                    int originalCount = g_Seq.clipCount;
                    int newLeadIdx = g_Seq.draggedClipIndex;
                    for (int i = 0; i < originalCount; ++i) {
                        if (g_Seq.clips[i].isSelected && g_Seq.clipCount < MAX_CLIPS) {
                            int cloneIdx = g_Seq.clipCount++;
                            g_Seq.clips[cloneIdx] = g_Seq.clips[i];
                            g_Seq.clips[cloneIdx].isSelected = true;
                            g_Seq.clips[i].isSelected = false;
                            if (i == g_Seq.draggedClipIndex) newLeadIdx = cloneIdx;
                        }
                    }
                    g_Seq.draggedClipIndex = newLeadIdx;
                    g_Seq.isCtrlDuplicating = false;
                    seq_unlock();
                }
            }
        }

        if (g_Seq.isSlipDragging && g_Seq.draggedClipIndex >= 0 && g_Seq.hasMovedPastThreshold) {
            int dx = mx - g_Seq.dragStartX;
            float beatDelta = (float)dx / ppb;
            float fpb = frames_per_beat(g_Seq.bpm);

            seq_lock();
            for (int i = 0; i < g_Seq.clipCount; ++i) {
                if (g_Seq.clips[i].isSelected) {
                    Clip *c = &g_Seq.clips[i];
                    if (c->sampleIndex < 0 || c->sampleIndex >= g_Seq.sampleCount) continue;
                    AudioSample *s = &g_Seq.samples[c->sampleIndex];
                    float pRate = (c->playbackRate > 0.01f) ? c->playbackRate : 1.0f;
                    int frameDelta = (int)(beatDelta * fpb * pRate);

                    long long newOffset = (long long)c->dragStartOffsetOrig - frameDelta;
                    if (newOffset < 0) newOffset = 0;
                    if (newOffset >= (long long)s->frameCount && s->frameCount > 0) newOffset = s->frameCount - 1;

                    c->sampleOffsetFrames = find_nearest_zero_crossing(s, (ma_uint64)newOffset, 128);
                }
            }
            seq_unlock();
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (g_Seq.isVolumeDragging && g_Seq.draggedClipIndex >= 0) {
            int dy = g_Seq.dragStartY - my;
            float newVol = g_Seq.dragStartVolume + (float)dy * 0.01f;
            if (newVol < 0.0f) newVol = 0.0f;
            if (newVol > 2.0f) newVol = 2.0f;

            seq_lock();
            if (g_Seq.draggedClipIndex < g_Seq.clipCount) {
                g_Seq.clips[g_Seq.draggedClipIndex].volume = newVol;
            }
            seq_unlock();

            g_Seq.volumePopupClip = g_Seq.draggedClipIndex;
            g_Seq.volumePopupExpiry = GetTickCount() + 1500;
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (g_Seq.isDraggingClip && g_Seq.draggedClipIndex >= 0 && g_Seq.hasMovedPastThreshold) {
            float mouseBeat = (float)(mx - TRACK_HEADER_WIDTH + g_Seq.scrollX) / ppb;
            float newLeadBeat = mouseBeat - g_Seq.dragStartBeatOffset;
            if (g_Seq.quantizeEnabled) newLeadBeat = quantize_beat_16th(newLeadBeat);

            int newLeadTrack = (my - HEADER_HEIGHT + g_Seq.scrollY) / TRACK_HEIGHT;

            seq_lock();
            if (g_Seq.draggedClipIndex < g_Seq.clipCount) {
                float rawBeatDelta = newLeadBeat - g_Seq.clips[g_Seq.draggedClipIndex].dragStartBeatOrig;
                int rawTrackDelta = newLeadTrack - g_Seq.clips[g_Seq.draggedClipIndex].dragStartTrackOrig;

                float minAllowedBeatDelta = -1e9f;
                float maxAllowedBeatDelta = 1e9f;
                int minAllowedTrackDelta = -999;
                int maxAllowedTrackDelta = 999;

                for (int i = 0; i < g_Seq.clipCount; ++i) {
                    if (g_Seq.clips[i].isSelected) {
                        float leftBoundDelta = -g_Seq.clips[i].dragStartBeatOrig;
                        if (leftBoundDelta > minAllowedBeatDelta) minAllowedBeatDelta = leftBoundDelta;

                        float rightBoundDelta = total_beats() - (g_Seq.clips[i].dragStartBeatOrig + g_Seq.clips[i].lengthBeats);
                        if (rightBoundDelta < maxAllowedBeatDelta) maxAllowedBeatDelta = rightBoundDelta;

                        int minTDelta = -g_Seq.clips[i].dragStartTrackOrig;
                        int maxTDelta = (g_Seq.trackCount - 1) - g_Seq.clips[i].dragStartTrackOrig;

                        if (minTDelta > minAllowedTrackDelta) minAllowedTrackDelta = minTDelta;
                        if (maxTDelta < maxAllowedTrackDelta) maxAllowedTrackDelta = maxTDelta;
                    }
                }

                float finalBeatDelta = rawBeatDelta;
                if (finalBeatDelta < minAllowedBeatDelta) finalBeatDelta = minAllowedBeatDelta;
                if (finalBeatDelta > maxAllowedBeatDelta) finalBeatDelta = maxAllowedBeatDelta;

                int finalTrackDelta = rawTrackDelta;
                if (finalTrackDelta < minAllowedTrackDelta) finalTrackDelta = minAllowedTrackDelta;
                if (finalTrackDelta > maxAllowedTrackDelta) finalTrackDelta = maxAllowedTrackDelta;

                for (int i = 0; i < g_Seq.clipCount; ++i) {
                    if (g_Seq.clips[i].isSelected) {
                        float b = g_Seq.clips[i].dragStartBeatOrig + finalBeatDelta;
                        if (b < 0.0f) b = 0.0f;
                        int t = g_Seq.clips[i].dragStartTrackOrig + finalTrackDelta;
                        if (t < 0) t = 0;
                        if (t >= g_Seq.trackCount) t = g_Seq.trackCount - 1;

                        g_Seq.clips[i].startBeat = b;
                        g_Seq.clips[i].track = t;
                    }
                }
            }
            seq_unlock();
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_MOUSELEAVE: {
        g_Seq.hoveredClip = -1;
        g_Seq.mouseX = -1;
        g_Seq.mouseY = -1;
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_LBUTTONUP: {
        if (g_Seq.isCtrlDuplicating && !g_Seq.hasMovedPastThreshold && g_Seq.draggedClipIndex >= 0) {
            seq_lock();
            if (g_Seq.draggedClipIndex < g_Seq.clipCount) {
                g_Seq.clips[g_Seq.draggedClipIndex].isSelected = !g_Seq.clips[g_Seq.draggedClipIndex].isSelected;
            }
            seq_unlock();
        }

        if (g_Seq.isDraggingClip || g_Seq.isVolumeDragging || g_Seq.isSlipDragging ||
            g_Seq.isMarqueeSelecting || g_Seq.isResizingLeft || g_Seq.isResizingRight ||
            g_Seq.isFadeInDragging || g_Seq.isFadeOutDragging) {
            g_Seq.isDraggingClip = false;
            g_Seq.isCtrlDuplicating = false;
            g_Seq.isVolumeDragging = false;
            g_Seq.isSlipDragging = false;
            g_Seq.isMarqueeSelecting = false;
            g_Seq.isResizingLeft = false;
            g_Seq.isResizingRight = false;
            g_Seq.isFadeInDragging = false;
            g_Seq.isFadeOutDragging = false;
            g_Seq.draggedClipIndex = -1;
            ReleaseCapture();
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_KEYDOWN: {
        switch (wParam) {
        case 'X':
            if (GetKeyState(VK_CONTROL) & 0x8000) {
                copy_selected_clips();
                delete_selected_clips();
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;
        case 'Y':
            if (GetKeyState(VK_CONTROL) & 0x8000) {
                redo_last_action();
            }
            break;
        case 'Z':
            if (GetKeyState(VK_CONTROL) & 0x8000) {
                if (GetKeyState(VK_SHIFT) & 0x8000) {
                    redo_last_action();
                } else {
                    undo_last_action();
                }
            }
            break;
        case 'S':
            if (GetKeyState(VK_CONTROL) & 0x8000) {
                save_project_dialog(hwnd);
            } else {
                split_clips_at_playhead();
            }
            break;
        case 'O':
            if (GetKeyState(VK_CONTROL) & 0x8000) {
                load_project_dialog(hwnd);
            }
            break;
        case 'D':
            if (GetKeyState(VK_CONTROL) & 0x8000) {
                deselect_all_clips();
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;
        case 'E': {
            OPENFILENAMEA ofn;
            char szFile[MAX_PATH] = "export.wav";
            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hwnd;
            ofn.lpstrFilter = "WAV Audio (*.wav)\0*.wav\0All Files (*.*)\0*.*\0";
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = sizeof(szFile);
            ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
            ofn.lpstrDefExt = "wav";

            if (GetSaveFileNameA(&ofn)) {
                export_timeline_to_wav(szFile);
            }
            break;
        }
        case 'A':
            if (GetKeyState(VK_CONTROL) & 0x8000) {
                seq_lock();
                for (int i = 0; i < g_Seq.clipCount; ++i) {
                    g_Seq.clips[i].isSelected = true;
                }
                seq_unlock();
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;
        case VK_LEFT: {
            float ppb = get_pixels_per_beat();
            int totalTimelineWidth = (int)(total_beats() * ppb);
            RECT rc;
            GetClientRect(hwnd, &rc);
            int visibleWidth = (rc.right - rc.left) - TRACK_HEADER_WIDTH;
            int maxScrollX = totalTimelineWidth - visibleWidth;
            if (maxScrollX > 0 && g_Seq.scrollX > 0) {
                g_Seq.scrollX -= (int)ppb;
                if (g_Seq.scrollX < 0) g_Seq.scrollX = 0;
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;
        }
        case VK_RIGHT: {
            float ppb = get_pixels_per_beat();
            int totalTimelineWidth = (int)(total_beats() * ppb);
            RECT rc;
            GetClientRect(hwnd, &rc);
            int visibleWidth = (rc.right - rc.left) - TRACK_HEADER_WIDTH;
            int maxScrollX = totalTimelineWidth - visibleWidth;
            if (maxScrollX > 0 && g_Seq.scrollX < maxScrollX) {
                g_Seq.scrollX += (int)ppb;
                if (g_Seq.scrollX > maxScrollX) g_Seq.scrollX = maxScrollX;
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;
        }
        case VK_UP:
            g_Seq.scrollY -= TRACK_HEIGHT;
            update_scrollbar(hwnd);
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        case VK_DOWN:
            g_Seq.scrollY += TRACK_HEIGHT;
            update_scrollbar(hwnd);
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        case VK_SPACE:
        case VK_RETURN:
            toggle_playback();
            break;
        case 'Q':
            g_Seq.quantizeEnabled = !g_Seq.quantizeEnabled;
            break;
        case 'L':
            g_Seq.isLofi = !g_Seq.isLofi;
            break;
        case VK_PRIOR:
            g_Seq.zoom += 0.25f;
            if (g_Seq.zoom > 4.0f) g_Seq.zoom = 4.0f;
            update_scrollbar(hwnd);
            break;
        case VK_NEXT:
            g_Seq.zoom -= 0.25f;
            if (g_Seq.zoom < 0.25f) g_Seq.zoom = 0.25f;
            update_scrollbar(hwnd);
            break;
        case 'T':
            if (GetKeyState(VK_CONTROL) & 0x8000) {
                add_track_action(hwnd);
            } else if (GetKeyState(VK_SHIFT) & 0x8000) {
                remove_track_action(hwnd);
            }
            break;
        case VK_HOME:
            if (GetKeyState(VK_SHIFT) & 0x8000) {
                g_Seq.playFromStartOnPlay = !g_Seq.playFromStartOnPlay;
            } else {
                InterlockedExchange(&g_Seq.playbackFrame, 0);
            }
            break;
        case VK_END:
            g_Seq.isPlaying = false;
            InterlockedExchange(&g_Seq.playbackFrame, 0);
            break;
        case VK_DELETE:
            delete_selected_clips();
            break;
        case VK_INSERT: {
            int targetTrack = 0;
            if (g_Seq.mouseY >= HEADER_HEIGHT) {
                targetTrack = (g_Seq.mouseY - HEADER_HEIGHT + g_Seq.scrollY) / TRACK_HEIGHT;
            }
            if (targetTrack < 0) targetTrack = 0;
            if (targetTrack >= g_Seq.trackCount) targetTrack = g_Seq.trackCount - 1;

            LONG currentFrame = InterlockedCompareExchange(&g_Seq.playbackFrame, 0, 0);
            float curBeat = frame_to_beat((ma_uint64)currentFrame, g_Seq.bpm, g_Seq.swing);
            open_sample_dialog(hwnd, targetTrack, quantize_beat_16th(curBeat));
            break;
        }
        case VK_OEM_PLUS:
        case VK_ADD:
            g_Seq.bpm += 2.0f;
            if (g_Seq.bpm > 300.0f) g_Seq.bpm = 300.0f;
            break;
        case VK_OEM_MINUS:
        case VK_SUBTRACT:
            g_Seq.bpm -= 2.0f;
            if (g_Seq.bpm < 40.0f) g_Seq.bpm = 40.0f;
            break;
        case VK_OEM_PERIOD:
            change_bar_count(1);
            break;
        case VK_OEM_COMMA:
            change_bar_count(-1);
            break;
        case VK_OEM_4:
            g_Seq.swing -= 0.05f;
            if (g_Seq.swing < 0.0f) g_Seq.swing = 0.0f;
            break;
        case VK_OEM_6:
            g_Seq.swing += 0.05f;
            if (g_Seq.swing > 0.75f) g_Seq.swing = 0.75f;
            break;
        case 'C':
            if (GetKeyState(VK_CONTROL) & 0x8000) {
                copy_selected_clips();
            }
            break;
        case 'V':
            if (GetKeyState(VK_CONTROL) & 0x8000) {
                paste_clipboard_clips();
            }
            break;
        }
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_DESTROY: {
        KillTimer(hwnd, 1);
        if (g_cacheDC) {
            SelectObject(g_cacheDC, g_cacheOldBmp);
            DeleteObject(g_cacheBmp);
            DeleteDC(g_cacheDC);
            g_cacheDC = NULL;
        }
        PostQuitMessage(0);
        return 0;
    }
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}