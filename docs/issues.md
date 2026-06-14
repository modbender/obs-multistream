# Known Issues & Investigations

A running log of non-trivial issues hit during development, what was ruled
out, and how they were resolved. Newest first.

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
