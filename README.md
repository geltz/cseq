<p>
  <img src="icon.png" alt="cseq icon">
</p>

cseq is a sampler/sequencer/digital audio workstation.  

written in pure c and win32, packed into a standalone binary that's around 300 kilobytes in filesize.

there's a built-in menu with all the keyboard/mouse shortcuts.


pairs nicely with [audiomap](https://github.com/geltz/audiomap) for quick drag-and-drop sample arrangement.

**filetypes**

`wav`, `flac`, `mp3` (audio)

`csq` (module project)

**controls**

| input | action |
| :--- | :--- |
| left drag clip | move clip across timeline and tracks |
| left/right edge drag | trim clip start / end |
| ctrl + drag clip | duplicate clip |
| alt + drag clip | slip edit sample offset |
| shift + drag clip / wheel | adjust clip volume level |
| clip handle drag | adjust fade-in / fade-out envelopes |
| right click clip | clip context menu (playback rate, fades, reset volume, delete) |
| right click track | track menu (parametric eq, mute, batch rates, batch fades, clear) |
| left/right drag (empty) | marquee box select clips |
| middle click drag | pan viewport smoothly |
| scroll wheel (header) | adjust track volume level |
| drag and drop | drop audio files directly onto tracks |

**keys**

| key | action |
| :--- | :--- |
| space / enter | toggle playback |
| home / end | return to start / stop playback & reset |
| shift + home | toggle playhead mode (from start / cursor) |
| + / - | adjust tempo (+/- 2 bpm) |
| < / > | change loop bar count (+/- 1 bar) |
| [ / ] | adjust 16th-note swing amount |
| q | toggle 1/16 snap quantize |
| s | split selected clips at playhead |
| l | toggle lo-fi 12-bit effect |
| ctrl + c / v / x | copy / paste / cut selected clips |
| ctrl + z / y | undo / redo action |
| ctrl + a / d | select all / deselect all clips |
| ctrl + t / shift + t | add / remove track |
| ins | import audio file at playhead |
| del | delete selected clips |
| pgup / pgdn | zoom timeline in / out |
| arrow keys | scroll timeline / navigate tracks |
| ctrl + s / o | save / load project (`.csq`) |
| e | export loop to 32-bit float wav |

**features**

| component | description |
| :--- | :--- |
| parametric track eq | per-track interactive 3-band parametric eq with live curve rendering & q adjustment |
| fast `.csq` project format | self-contained project saving with background compressed audio bundling |
| interactive fade envelopes | visual drag-and-drop fade in and fade out handles directly on clip waveforms |
| zero-crossing snap | automatic click-free sample slicing, boundary ramping, and split alignment |
| non-destructive slip | slide audio within clip boundaries without moving timeline triggers |
| custom rate scaling | per-clip and per-track playback speed/pitch control from 0.01x to 2.00x |
| dynamic swing & snap | mpc-style 16th upbeat timing with 1/16th beat snap grid |
| lo-fi engine | bit-depth reduction and soft saturation reconstruction filter |
| master export | normalized 32-bit float wav rendering with soft saturation limiter |

this program was primarily written using [Gemini 3.7 Flash](https://deepmind.google/models/model-cards/gemini-3-7-flash/)

**libraries used**

[miniaudio](https://github.com/mackron/miniaudio) for all resampling/audio related work

[Airwindows](https://github.com/airwindows/airwindows) code ported to C for the parametric equalizer (SmoothEQ3) 
