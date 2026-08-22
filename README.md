<p>
  <img src="icon.png" alt="cseq icon">
</p>

**cseq** is an audio sampler and timeline sequencer. One standalone binary written in pure C99 and Win32.  

Pairs nicely with [audiomap](https://github.com/geltz/audiomap) for quick drag‑and‑drop sample arrangement.

---

### Supported filetypes

- Audio: `wav`, `flac`, `mp3`
- Project: `csq` (self‑contained)

---

### Keyboard shortcuts

| Key | Action |
| :--- | :--- |
| `Space` / `Enter` | Play / pause |
| `Home` / `End` | Jump to start / Stop & reset |
| `Shift+Home` | Toggle playhead mode (start vs cursor) |
| `+` / `-` | BPM ±2 |
| `<` / `>` | Bars ±1 |
| `[` / `]` | Swing ±5% |
| `Q` | Toggle 1/16 snap |
| `S` | Split selected clips at playhead |
| `L` | Toggle lo‑fi |
| `Ctrl+C` / `V` / `X` | Copy / paste / cut |
| `Ctrl+Z` / `Y` | Undo / redo |
| `Ctrl+A` / `D` | Select all / deselect all |
| `Ctrl+T` / `Shift+T` | Add / remove track |
| `Ins` | Import audio at playhead |
| `Del` | Delete selected clips |
| `PgUp` / `PgDn` | Zoom timeline |
| `Arrows` | Scroll / navigate |
| `Ctrl+S` / `O` | Save / load project (`.csq`) |
| `E` | Export loop as 32‑bit WAV |

---

### Mouse controls

| Interaction | Action |
| :--- | :--- |
| Left‑drag clip | Move clip across timeline / tracks |
| Edge drag (left/right) | Trim clip start / end |
| Ctrl + drag clip | Duplicate clip |
| Alt + drag clip | Slip‑edit sample offset |
| Shift + drag / wheel | Adjust clip volume |
| Shift + scroll clip | Adjust clip playback rate |
| Drag fade handle | Adjust fade‑in / fade‑out envelopes |
| Right‑click clip | Clip context menu (rate, fades, reset volume, delete) |
| Right‑click track | Track menu (EQ, mute, batch rate/fades, clear) |
| Left/right drag (empty area) | Marquee selection |
| Middle‑click drag | Pan viewport |
| Drop audio file | Import sample directly onto a track |

---

### Features

| Component | Description |
| :--- | :--- |
| **Parametric track EQ** | Per‑track 3‑band interactive EQ with live curve & Q adjustment |
| **Fast `.csq` project format** | Self‑contained saving with background compressed audio bundling |
| **Interactive fade envelopes** | Drag‑and‑drop fade handles directly on clip waveforms |
| **Zero‑crossing snap** | Automatic click‑free slicing, boundary ramps, and split alignment |
| **Non‑destructive slip** | Slide audio within clip boundaries without moving timeline triggers |
| **Custom rate scaling** | Per‑clip and per‑track speed/pitch from 0.01× to 2.00× |
| **Dynamic swing & snap** | MPC‑style 16th‑note timing with grid snap |
| **Lo‑fi engine** | Bit‑depth reduction + soft reconstruction filter |
| **Master export** | Normalised 32‑bit float WAV rendering with soft‑clamp limiter |

---

### Libraries

- [miniaudio](https://github.com/mackron/miniaudio) – audio resampling / I/O  
- [Airwindows](https://github.com/airwindows/airwindows) – SmoothEQ3 parametric EQ (ported to C)

---

*This program was primarily written using [Gemini 3.7 Flash](https://deepmind.google/models/model-cards/gemini-3-7-flash/).*
