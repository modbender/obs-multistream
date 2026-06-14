# OBS MultiStreamer — Roadmap

Native multistreaming built into this OBS Studio fork via **canvas / output
multiplexing**: canvases (resolution + fps + encoder settings) become the unit
of streaming, each wired to one or more destinations. Built on OBS's existing
`obs_canvas_t` + output primitives rather than a rewrite, to keep the capture
and plugin ecosystem.

This is a **private fork**, not submitted upstream. The app is rebranded
"OBS MultiStreamer."

Status legend: ✅ done · 🔧 in progress · 🐛 open bug · 🔭 planned · 💭 in design
· ⏸ deferred

---

## Phase 1 — Canvas foundation ✅ COMPLETE

The Default Canvas is the single source of truth for the main video pipeline
and the one streaming output. Streaming-only (1 video + 1 audio encoder per
canvas); recording, replay buffer, and multitrack/GoLive are hidden and inert.

- ✅ **Data model + persistence** — `CanvasDefinition`, `CanvasManager`,
  global `canvases.json`, per-scene-collection `canvas_bindings`.
- ✅ **Settings → Canvas tab** — add/remove canvases (immediate + auto-saved,
  like profiles); Default card pinned, additional-canvas grid.
- ✅ **Canvas editor dialog** — name/resolution/fps + Video | Audio | Advanced
  tabs (`OBSPropertiesView`); "Use Default" inheritance toggles.
- ✅ **Inheritance & color resolution (4a)** — per-field inheritance from the
  Default Canvas resolved at encode time (`ToVideoInfo`).
- ✅ **Output rewire & tab removal (4b)** — Default Canvas drives
  `obs_reset_video` + the stream encoder via an always-Advanced output handler;
  Settings → Output and Video tabs removed; all color settings moved onto the
  canvas; recording/replay/GoLive UI hidden.
- ✅ **Full inert pass (4b T7)** — recording and GoLive made truly dormant
  (GoLive never constructed, no record/replay auto-start or hotkeys, encoder
  seeding validated). See `docs/superpowers/` specs/plans (gitignored).
- ✅ **Settings polish** — Canvas tab moved below Stream; themed Canvas list
  icon across all themes.

### Tooling ✅

- ✅ **`magefile.go`** — `mage build` / `buildAll` / `run` (portable) /
  `format` / `configure` / `deps` / `tag`. Captures the VS-bundled
  cmake/clang-format paths, portable-mode launch, version-tag fix, and the
  Norton TLS-revocation dependency prefetch.

---

## Open bugs 🐛

- 🐛 **"Use Default resolution" toggle stretched full-width** in the canvas
  editor — the idian `ToggleSwitch` fills its layout cell and reads like a
  slider, though it behaves as a toggle. Fix deferred pending the Phase 2
  editor/dock decision (the control may relocate).
- ⏸ **Pre-existing `verticalLayout_35` AUTOUIC name-collision warning** —
  cosmetic; uic auto-renames. Not introduced by this work.

## Verification owed (manual, can't be driven headlessly)

- 🔧 **Live streaming** — partially exercised; confirmed Phase 1 streams the
  Default Canvas only (additional canvases not yet wired — see Phase 2).
- 🔭 **GUI visual passes** — Settings tabs, canvas editor, control panel hide
  state across themes.

---

## Phase 2 — Multi-destination 💭 IN DESIGN

The goal: each canvas streams to its own destination(s) simultaneously — one
encode per canvas, fanned out to many platforms (Twitch + YouTube + Kick + …).
This closes the Phase 1 seam where additional canvases are definitions only.

**Proposed direction (under discussion, not decided): dockable canvas windows.**
Inspired by SE.Live, but avoiding its failure modes. Each canvas surfaces as a
dockable window holding a resolution-matched preview plus its scene binding
(with optional auto-follow of the current scene). The Default preview becomes
dockable too. Later, only canvases connected to a destination surface as docks.

SE.Live failure modes to explicitly avoid:

- Broken canvas CRUD (rename/save/delete that silently do nothing or error).
- A non-switchable "primary" destination that forces redundant re-encodes.
- GPU blowout and game FPS drops from redundant encodes.
- Frame-skips invisible to OBS's native Stats.
- Dead "link scene" feature.

Design intents to carry in:

- One encode per canvas; share/reuse encode where resolution matches instead of
  re-encoding per destination.
- Native `obs_output_t` per destination so OBS's own Stats reflect everything.
- All canvas CRUD actually works and is reflected live.

> This section is a captured proposal, not an approved design. The full design
> will be brainstormed and written to `docs/superpowers/specs/` before any
> implementation.

---

## Backlog & deferred decisions ⏸

- ⏸ **GoLive / Multitrack Video** — currently dormant. It's Twitch Enhanced
  Broadcasting (many quality renditions → one Twitch ingest), orthogonal to our
  many-platforms goal. Decision at the multi-destination phase: delete the
  Twitch-specific `GoLiveAPI`/config, or harvest `MultitrackVideoOutput`'s
  multi-output/encoder-sharing plumbing for our fan-out handler.
- ⏸ **WHIP simulcast encoder-id divergence** — conscious deferral from 4b.
- ⏸ **Canvas-editor input clamping** — guard FPS and SDR-white-level against 0.
- 🔭 **Finish the `canvas-foundation` branch** — merge/PR/cleanup once the open
  items above are resolved.
