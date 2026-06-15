# Known Issues & Investigations

A running log of non-trivial issues hit during development, what was ruled
out, and how they were resolved. Newest first.

---

## #2 — Multistream engine (2e): verified failure path, live-broadcast path unverified

**Status:** OPEN (verification gap, not a defect) — engine and dock verified end-to-end
short of a real sustained broadcast.
**Date:** 2026-06-15
**Affected build:** `32.1.0-37` and later (canvas-foundation), commits `108a7bd4e`,
`0be82b481`, `b0fc93ddd`, `76355c446`, `51fbdd28c`.

### What was verified (portable sandbox, windows-mcp GUI)

- Multistream dock registers, groups bindings by canvas, shows per-row profile +
  status dot + live toggle, a per-group cascade toggle, and the Go Live / Stop All
  footer.
- "Go Live" / a row toggle drives the real encode-once fan-out: per-canvas x264
  video + AAC audio encoders are created, an RTMP service + output are created and
  started, and a connection to the destination ingest is attempted (OBS log
  confirms `Connecting to RTMP URL …` then the per-binding output name).
- With a deliberately invalid stream key the connection fails (`Connection … failed:
  -3`) and the row now shows **Error** (red dot + tooltip), via the
  `OnOutputStop` non-`OBS_OUTPUT_SUCCESS` path (commit `51fbdd28c`).
- Single-key guard: a second binding sharing one stream profile is refused while the
  first occupies it; the toggle bounces back to Idle (log: "profile already live").
- Stop All clears every live/errored output back to Idle.
- No crash through repeated start / async-fail / stop cycles — the `liveMutex`
  (commit `76355c446`) guards the off-thread start/stop signal handlers against the
  Qt thread's mutation of the `live` vector.

### The gap

A **successful, sustained broadcast** to a real destination was **not** verified —
that requires valid platform stream keys and actually pushing video to a live
service, which is out of scope for the isolated portable test (the constraint is to
never touch real accounts/credentials). The Connecting → **Live** transition
(`OnOutputStart`) is wired and the single-stream legacy path already exercises the
same libobs output mechanics, so this is a confidence gap on the last hop only, not
a known break.

### How to close it

Configure one real stream key on a throwaway account, Go Live for a single binding,
and confirm the row reaches **Live** (green) and the platform shows the feed; then
Stop All and confirm a clean return to Idle.

---

## #1 — Broken / mispositioned preview (native display) on a high-DPI display

**Status:** RESOLVED — environmental (Windows per-application DPI override), not a fork defect.
**Date investigated:** 2026-06-14
**Affected build at time of report:** `32.1.0-20-g734e6a4c4` and later (canvas-foundation branch)

### Symptom

On launch, the main program preview rendered incorrectly:

- The GPU-rendered preview (a native child window / `OBSQTDisplay`) was
  **mispositioned** — painted at the top-left while its Qt container/toolbar
  ("Scale to Window", "No source selected") sat far below it.
- Large **unpainted white/black regions** where the preview should be.
- A **stale framebuffer**: the preview area showed leftover snapshots of the
  desktop / OBS's own UI (e.g. a ghosted "File Edit View Docks…" menu) instead
  of live content.
- The native display **did not track window resizes** (stayed pinned through
  maximize / resize), and toggling Studio Mode produced equally broken
  Preview/Program displays.

The preview content itself was correct (an empty scene renders black); the
problem was purely the **native window geometry / presentation**.

### Environment

- GPU: NVIDIA GeForce RTX 5070 Ti (50-series), D3D11 backend (initialised fine
  per logs: `video settings reset: 1920x1080 @ 60`, `D3D11 loaded successfully`).
- Display: 4K (3840×2160) at **150% scale (Windows "Recommended")**, dual-monitor
  with mixed DPI between screens.
- Qt: bundled `Qt6 6.11.1` (from obs-deps `2026-06-02`); official OBS ships
  `Qt6 6.8.3`.

### What was ruled OUT (and how)

1. **Our canvas/scene code (Phase 2a) — NO.**
   A git bisect *appeared* to implicate the 2a commits, but the bisect was
   **confounded**: every "broken" run used `mage run` (a fresh portable config)
   and every "fine" run was a direct launch (a populated `%APPDATA%` config).
   The commit was never the variable.

2. **Portable mode (`--portable`) — NO.**
   Copying a populated config into the portable rundir and launching
   `--portable` rendered **fine**. Portable mode is innocent; it was simply
   always-fresh.

3. **The Qt 6.11 upgrade — NO.**
   `Qt6Core.dll` was dated 2026-06-12, i.e. Qt 6.11 was already in the build
   during a known-good livestream the day before. Decisively: **official OBS
   (Qt 6.8.3) reproduces the identical breakage** on a fresh config on this
   machine. Different build, different Qt, same bug → not Qt.

4. **Fresh vs. populated config content — INDIRECT, not causal.**
   A populated config worked and a fresh config broke, but this correlation was
   a side-effect of the real cause (below). The breakage did **not** self-heal
   after a clean `File → Exit` (which saves geometry) and reopen.

5. **W1 config separation — not the cause, only the exposure.**
   Pre-W1 a direct launch loaded the user's populated `%APPDATA%\obs-studio`
   (worked); post-W1 it loads a fresh `%APPDATA%\obs-multistream`. W1 only
   changed *which* config was loaded, surfacing the underlying issue more often.

### Root cause

A **Windows per-application "Override high DPI scaling behavior" compatibility
flag** had been set on `obs64.exe` (Properties → Compatibility → High DPI
settings → **System (Enhanced)**). The user applies this to apps that render
oddly at 4K@150%; it had been set on both this fork's `obs64.exe` and the
official OBS `obs64.exe` "some time back".

OBS already declares **Per-Monitor-V2 DPI awareness** and positions its native
display child windows accordingly. Forcing **System (Enhanced)** on top of that
overrides OBS's own DPI handling, so Windows bitmap-scales / mis-maps the native
display window's coordinates → the mispositioned, stale, non-tracking preview.

This is a per-exe **registry** setting
(`HKCU\…\AppCompatFlags\Layers`, keyed by exe path), which is why it **persisted
across rebuilds** and produced confusing, seemingly code-correlated results.

### Resolution

Uncheck **"Override high DPI scaling behavior"** on `obs64.exe`
(Properties → Compatibility → Change high DPI settings). With OBS left to its
own Per-Monitor-V2 DPI awareness, the preview renders correctly. Confirmed fix.

### Implications / fork actions

- **No code change required.** Nothing in the fork (canvas model, output
  rewire, rebrand, portable handling) causes or worsens this.
- It affects **official OBS identically** — it is an OBS/Qt + Windows-DPI
  interaction triggered only by the manual per-exe DPI override.
- **Support note for users:** if the preview is mispositioned / shows stale
  content on a high-DPI display, ensure `obs64.exe` does **not** have a manual
  "Override high DPI scaling behavior" compatibility flag set. OBS manages DPI
  itself; overriding it breaks the native preview.
- No reason to pin Qt back to 6.8.x for this issue (6.8 reproduces it too).
  A Qt-version pin for upstream *parity* remains a separate, optional decision.
