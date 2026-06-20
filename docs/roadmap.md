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
- 🔭 **2a scene model (runtime)** — code-complete but unverified live: create a
  2nd canvas + scene + current-scene, restart, confirm it survives (no collapse
  onto main); Add Source defaults to the existing source; Settings/plugin-added
  canvases get a default scene. Scene-link sync stays dormant until the 2b UI
  populates the link map.

---

## Phase 2 — Multi-destination 🔧 2a–2e + 2b code-complete, live broadcast owed

The goal: each canvas streams to its own destination(s) simultaneously — one
encode per canvas, fanned out to many platforms (Twitch + YouTube + Kick + …).
This closes the Phase 1 seam where additional canvases are definitions only.

**Model decided: per-canvas scenes over shared sources** (Model 1). Each canvas
owns its own scenes (normal OBS scenes); sources stay global/shared, so a canvas
scene references and positions them. Layout is per-canvas (transform/visibility);
content is shared (properties/filters/name). "Link scene" = activation sync
(switch main program scene → linked canvases follow). Rejected Model 2 (one base
scene + per-canvas overrides) as a high-cost custom layer. This **revises** the
Phase 1-era "canvas references one shared scene" decision (which only held for
same-aspect canvases). Full design: `docs/superpowers/specs/2026-06-14-canvas-
multidestination-design.md`. Surfaced as **dockable canvas windows** (preview +
scene tree + 🔗 link + per-canvas output), all dockable, phased visibility.

SE.Live failure modes explicitly designed against: broken Link scene; broken
canvas CRUD; a non-switchable "primary" destination forcing re-encodes; GPU
blowout/FPS drops from redundant encodes; frame-skips invisible to OBS Stats.
Our fixes: working uuid-keyed link; reuse Phase 1 CanvasManager CRUD; no
privileged primary; one encode per canvas, shared where settings match; native
`obs_output_t` per destination so OBS Stats see everything.

**Build decomposition (each: spec → plan → implement):**

- ✅ **2a — Scene model** — per-canvas scenes over shared sources, scene-link
  activation sync, Add-Source "use existing" default. Plan:
  `docs/superpowers/plans/2026-06-14-canvas-scene-model.md` (built on libobs's
  `obs_canvas_scene_create`/`obs_canvas_set_channel`). **Code-complete**
  (build-green + two-stage reviewed; GUI runtime verification owed). Capstone
  fix: a Phase-1 carry-forward bug had additional canvases created in `OBSInit`
  then destroyed by `ClearSceneData` before the collection loaded, so their
  scenes/bindings restored onto the main canvas. Now recreated inside `LoadData`
  (after the clear, before `obs_load_sources`); bindings applied after sources
  load; default scenes seeded only for still-empty canvases. **2b/2c must keep
  additional-canvas creation in the load path, not `OBSInit`.**
  Phase 2 was re-decomposed into ordered sub-phases 2c → 2d → 2e → 2b; all are
  now on `canvas-foundation`:
- ✅ **2c — Streams tab** — global, reusable stream profiles (`streams.json`),
  master-detail Streams settings page, primary-only mirror into OBS's single
  `obs_service`. Build-green + GUI-verified (persistence, primary mirror,
  Cancel-discards).
- ✅ **2d — Outputs tab** — an output = (stream profile × canvas) + `enabled`,
  persisted per scene-collection (`output_bindings`). Settings → Outputs routing
  page grouped by canvas with a searchable profile dropdown and a single-key
  in-use guard. Build-green + GUI-verified (persist across restart, in-use block).
- ✅ **2e — Multistream engine + dock** — encode-once-per-canvas fan-out
  (`MultistreamOutput`), one `obs_output` per enabled binding driven by its
  profile's `obs_service`, handler-level single-key guard, and a Multistream dock
  (per-canvas groups, status dots, Go Live / Stop All). Build-green + GUI-verified
  to the connection-failure (Error) path; a real sustained broadcast is owed
  (needs live creds — see `docs/issues.md` #2).
- ✅ **2b — Dockable canvas windows** — one dock per additional canvas: a scene
  list + an `OBSQTDisplay` of that canvas's mix, splitter auto-orienting from the
  dock shape, layout persisted by canvas uuid. Build-green + GUI-verified
  (scene add scoped to canvas, persistence across restart, clean teardown).

### Canvas-dock UX iteration (batch 2) ✅ 2026-06-16

Reworked the canvas-preview UX around **Settings → Outputs as the single source
of truth**, plus supporting fixes. All build-green and GUI-verified by the user.

- ✅ **Output-gated previews** — a canvas renders a preview only when it has ≥1
  enabled output binding (`OutputBindings::AnyEnabledForCanvas`, pushed via the
  `OBSBasic::OutputBindingsChanged` signal). The Default canvas keeps the central
  preview and swaps to a **canvas-agnostic** "Preview Disabled — enable a canvas
  in Settings → Outputs" placeholder until enabled.
- ✅ **Additional canvas docks appear-on-enable** — each additional canvas's dock
  is created **floating** only while it has an enabled output and is destroyed
  when disabled, reconciled live off `OutputBindingsChanged`.
- ✅ **Canvas-dock anatomy** — in-dock per-scene source tree, section
  headers/divider/toolbars, a footer (canvas-editor `⚙` + polled live-status `●`),
  and per-scene `🔗` link toggles (Model-1 activation sync to the program scene).
- ✅ **Point-4 considered and reverted** — briefly made the Default preview a
  dockable panel; it collapsed the central widget and starved the layout, so it
  was reverted. Default preview stays the central widget; stock Scenes/Sources
  docks untouched.
- ✅ **Stream-profile duplicate validation** — block a profile that reuses
  another's stream key (exact) or display name (case-insensitive `Platform -
  Name`) at every commit point; fixed the panel button clipping and the
  stale-list-on-Apply.
- ✅ **Canvas editor / cards** — resolution presets up to 4K (landscape +
  vertical, combo still editable); cards show friendly encoder names via
  `obs_encoder_get_display_name`.
- ✅ **Theme cleanup** — removed the dangling `qproperty-videoIcon` (no matching
  Q_PROPERTY since the Video tab was folded into Canvas) that logged on every
  Settings open.

### Hardening & cleanup pass ✅ 2026-06-19

A low-risk cleanup + the high-value items from the Phase-2 holistic review
(`docs/issues.md` #3). All build-green; runtime-unverified items noted.

- ✅ **Cleanup pass** — deduped the State→color status palette into
  `MultistreamOutput::StateColor`; removed dead members (`SetPrimary`,
  `AnyActive`, 2-arg `Unset`) and two dead locale keys.
- ✅ **R1 — `OBSBasic::ForEachStreamableCanvas`** — hoisted the duplicated
  "Default-first then non-ephemeral canvases" walk; routed the two true-dup sites
  (Multistream dock, Outputs tab) through it. The Stream-tab / SceneCollections
  walks were left alone (drifted intent).
- ✅ **C2 — deleted-profile surfaced distinctly** — a binding pointing at a
  deleted stream profile now reads as "profile deleted" (dock row + Outputs badge)
  rather than identically to an unset one.
- ✅ **P2 — incremental dock refresh** — split `MultistreamDock` into structural
  `Refresh` vs. status-only `UpdateStatuses`; status ticks no longer rebuild the
  tree. Also fixed a latent staleness bug (dock didn't refresh on binding edits
  while visible) via a new `OutputBindingsChanged → Refresh` connection.
- ✅ **H1 — live canvas-edit crash guard** — editing an additional canvas's
  resolution while it was live on a multistream output called
  `obs_canvas_reset_video` unguarded (UAF). Added `MultistreamOutput::IsCanvasLive`
  and guarded both reset sites (direct edit + Default-branch follower loop);
  warn/revert gated to actual resolution/FPS changes. The editor dialog also
  **locks** the resolution/FPS inputs while the canvas is live, with the guard as a
  backstop. Runtime-unverified (needs a live output, same constraint as #2).

---

## Phase 3 — Per-canvas editing & studio mode 🔭 PLANNED (not started)

The Phase-2 model lets an additional canvas be *populated* but not visually
*laid out*, and studio mode / the central editable stage are Default-only. Phase
3 closes that. Each item needs a scoping spike before planning — the central
preview (`OBSBasicPreview`) is OBS's most load-bearing widget and assumes the
main program canvas in several places.

- ✅ **3a — Editable canvas-dock previews (keystone) — IMPLEMENTED (build-green +
  reviewed; GUI acceptance owed).** The canvas dock now hosts an `OBSBasicPreview`
  bound to its canvas, so sources in a canvas dock are draggable / selectable /
  stretch / crop / rotate, scoped to that canvas's scene. Done via **Approach A**:
  parameterized `OBSBasicPreview` with a `targetCanvas` member (default `nullptr` =
  the central preview's exact prior behavior); `TargetScene()`/`TargetVideoInfo()`
  route scene + base-resolution, a per-surface `surface*` viewport decouples
  paint/hit-test, and a self-contained `CanvasPreviewRender` draw callback paints
  `obs_canvas_render` + the editing overlays. Plan:
  `docs/superpowers/plans/2026-06-20-editable-canvas-previews.md` (5 tasks, commits
  `69f4d59e`..`cbf227c5`). Per-task spec+quality reviews + a holistic final review
  all passed (null-path invariant, multi-dock isolation, no stale-viewport window).
  **Owed:** the manual GUI acceptance pass (drag isolation across docks, teardown,
  live-canvas edit) — can't be driven headlessly. Out of scope (→ 3b/3d): per-canvas
  undo, Studio Mode, nudge, scrollbars, source-list context menu. Unblocks 3b.
- 🔭 **3b — Per-canvas Studio Mode.** Make preview/program staging + transitions
  work per canvas, not Default-only. Superset of 3a (adds per-canvas transition +
  program staging on top of an editable surface); the hardest item. Do after 3a.
- 💭 **3c — Default-preview / canvas-in-center layout (decision deferred).** Two
  related questions about the QMainWindow **central widget** (which dock widgets
  can't occupy):
  - When the Default canvas is output-gated (disabled), should the central
    "Preview Disabled" placeholder *collapse* (`hide()` it — Qt's `display:none`
    equivalent) so the space is reclaimed, accepting that surrounding docks reflow
    into it (Scenes/Sources may balloon — the point-4 "starved layout" risk)? Left
    as today's in-place placeholder for now; **decide later**, ideally with a quick
    reversible `hide()` experiment to judge the reflow in a real layout.
  - **"First-enabled canvas wins the center" — shelved.** Letting any canvas occupy
    the central editable stage via runtime reparenting + a primary-canvas state
    machine. Rejected as invasive surgery for marginal gain; the real need (editing
    a non-default canvas) is better served by 3a's editable docks. Default-in-center
    + additionals-floating is the shipped model.
- ✅ **3d — Canvas-dock editor parity (scenes & sources lists) — IMPLEMENTED
  (build-green + reviewed, GUI acceptance owed) 2026-06-20.** Commits `873846694`
  (add-source core gains optional `targetScene`), `6f1e99ec1` (`OBSBasicSourceSelect`
  3-arg ctor + canvas-aware undo — `SetCurrentScene` gated behind `!isCanvas`),
  `3537d61f` (dock `+` opens the full picker scoped to the canvas scene), `69f011d82`
  (`OBSBasicTransform` derives its scene from the item, not `GetCurrentScene`;
  item-scoped Reset; canvas detection via owned `obs_source_get_canvas` refs),
  `570a4268c` (source + scene right-click menus on `CanvasDock`), `c624fdca7`
  (extracted shared `ResetSceneItemTransform` into `frontend/utility/`). Plan:
  `docs/superpowers/plans/2026-06-20-canvas-dock-editor-parity.md` (gitignored).
  Reuse-not-rewrite: threaded an optional target scene through OBS's existing
  add-source/transform dialogs (program path byte-for-byte unchanged) and
  reimplemented only the context-menu shells, all canvas-scoped via 3a's
  `SelectedSceneItem()` / `GetCanvasCurrentScene(canvas)` seam. Holistic review =
  SHIP (no program-scene leak — every `GetCurrentScene`-family hit is a program
  fallback or `!isCanvas`-gated; multi-dock isolation sound; teardown safe). **GUI
  acceptance owed** (headless-undriveable): two-dock isolation, canvas add undo/redo,
  Edit-Transform-open-then-destroy-dock, full type-picker attaches to canvas scene.
  Excluded by design (main-window-only): Multiview, projectors, grouping/multi-select,
  copy/paste, interact, grid mode, Studio Mode. Original scope below:
- 🔭 **3d (original scope).** The canvas dock's
  scene/source lists are stripped down vs the main window's; bring them to parity:
  - **Add Source** — the dock's `+` (`CanvasDock::AddSource`) only lists *existing*
    global input sources to attach (Model 1: shared sources), so it shows however
    many inputs already exist. The main window's `+` opens the source-*type* picker
    to *create* a new source. Add a "create new source" path to the dock (likely
    reuse `OBSBasicSourceSelect`, scoped to the canvas scene) so new sources can be
    made from the dock, not just attached.
  - **Source-list context menu** — the dock's `sourceList` has no
    `CustomContextMenu` wired at all (only `sceneList` does), so right-click is dead.
    Add a context menu reaching parity with the main Sources tree: Edit Transform /
    reset / fit / center / flip, order, scale filtering, blending, deinterlacing,
    Filters, Properties, Rename, Remove. Most of this is tied to the main window's
    current scene today, so it shares 3a's parameterization work.
  - **Edit Transform** — numeric transform dialog wired to the dock's selected
    scene item (complements 3a's drag-to-transform in the preview).
  - **Scene-list context menu** — the dock's scene menu is only Add/Rename/Remove;
    the main Scenes menu adds Duplicate, copy/paste filters, order, Filters,
    Projector, Show in Multiview, etc. Bring to parity where it makes sense per
    canvas.
  Sequence after 3a (the editable preview is the foundation; the context menus and
  add-source reuse the same main-scene-decoupling).
- 🔭 **3e — Stats dock: multi-stream monitoring (needs design).** OBS's stock Stats
  dock reports a single output (the main stream/recording). With the encode-once
  fan-out engine, the user wants to monitor **all** outputs at once — per-binding
  bitrate / dropped frames / reconnects / CPU-GPU, grouped by canvas, plus the
  shared per-canvas encode stats. **Open design question (no approach yet):**
  whether to extend/replace the stock `OBSBasicStats` dock or build a new
  multistream-stats panel reading `MultistreamOutput`'s per-`obs_output` handles
  (it already tracks them); how to present N outputs compactly; and what libobs
  per-output stats (`obs_output_get_total_frames`/`_frames_dropped`/congestion) map
  cleanly per binding. Brainstorm before planning.

---

## Backlog & deferred decisions ⏸

- ⏸ **GoLive / Multitrack Video** — currently dormant. It's Twitch Enhanced
  Broadcasting (many quality renditions → one Twitch ingest), orthogonal to our
  many-platforms goal. Decision at the multi-destination phase: delete the
  Twitch-specific `GoLiveAPI`/config, or harvest `MultitrackVideoOutput`'s
  multi-output/encoder-sharing plumbing for our fan-out handler.
- ⏸ **WHIP simulcast encoder-id divergence** — conscious deferral from 4b.
- ✅ **C1 — output-binding edits now honor Settings → Cancel.** Edits still commit
  live (previews react immediately), but the dialog snapshots the bindings on open,
  reverts on Cancel/Esc, and re-baselines on Apply/OK — mirroring the Stream tab.
  See `docs/issues.md` #3 C1.
- ✅ **Canvas-editor input clamping** — FPS num/den (`setRange(1, …)`), SDR white
  level (`setRange(80, 480)`), and the `WxH` parse (`cx > 0 && cy > 0`) already
  floor every numeric input above 0 through the dialog; no separate guard needed.
- 🔭 **Finish the `canvas-foundation` branch** — merge/PR/cleanup once the open
  items above are resolved.
