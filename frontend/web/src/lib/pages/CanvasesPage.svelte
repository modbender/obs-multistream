<script lang="ts">
  import {
    obs,
    type CanvasInfo,
    type EncoderType,
    type OutputBindingInfo,
    type StreamProfileInfo,
    type MultistreamStatus,
    type MultistreamState,
  } from "../bridge";
  import CanvasEditor from "../CanvasEditor.svelte";
  import CollectionDialog, { type DialogSpec } from "../CollectionDialog.svelte";

  // Canvases page (master-detail): a left list of canvases + a right detail pane
  // showing the selected canvas's editor plus its destinations as toggle-only rows.
  // Output bindings live here read-only-of-control (enable/disable + bind/unbind);
  // going live is the Studio GO-LIVE bar / Destinations page, never per-row here.
  let canvases = $state<CanvasInfo[]>([]);
  let bindings = $state<OutputBindingInfo[]>([]);
  let profiles = $state<StreamProfileInfo[]>([]);
  let live = $state<MultistreamStatus[]>([]);
  let videoEncoders = $state<EncoderType[]>([]);
  let audioEncoders = $state<EncoderType[]>([]);
  let loaded = $state(false);
  let error = $state<string | null>(null);

  let selectedUuid = $state<string | null>(null);

  // Add-canvas name prompt (reuses the Studio inline canvas.create flow).
  let dialog = $state<DialogSpec | null>(null);

  // Inline "bind destination" picker for the selected canvas (the chosen profile
  // uuid; "" => an unset/placeholder binding).
  let adding = $state(false);
  let addProfile = $state("");

  function reconcileSelection(): void {
    if (canvases.length === 0) {
      selectedUuid = null;
      return;
    }
    if (!selectedUuid || !canvases.some((c) => c.uuid === selectedUuid)) {
      selectedUuid = canvases.find((c) => c.isDefault)?.uuid ?? canvases[0].uuid;
    }
  }

  async function loadCanvases(): Promise<void> {
    try {
      canvases = await obs.call("canvas.list");
      reconcileSelection();
    } catch (e) {
      error = (e as Error).message;
    }
  }
  function loadBindings(): void {
    obs
      .call("outputBinding.list")
      .then((l) => (bindings = l))
      .catch(() => {});
  }
  function loadProfiles(): void {
    obs
      .call("streamProfile.list")
      .then((l) => (profiles = l))
      .catch(() => {});
  }
  function loadLive(): void {
    obs
      .call("multistream.status")
      .then((r) => (live = r.outputs))
      .catch(() => {});
  }

  async function loadAll(): Promise<void> {
    error = null;
    try {
      const [list, venc, aenc, binds, profs, status] = await Promise.all([
        obs.call("canvas.list"),
        obs.call("encoderTypes.list", { kind: "video" }),
        obs.call("encoderTypes.list", { kind: "audio" }),
        obs.call("outputBinding.list"),
        obs.call("streamProfile.list"),
        obs.call("multistream.status"),
      ]);
      canvases = list;
      videoEncoders = venc;
      audioEncoders = aenc;
      bindings = binds;
      profiles = profs;
      live = status.outputs;
      reconcileSelection();
    } catch (e) {
      error = (e as Error).message;
    } finally {
      loaded = true;
    }
  }

  $effect(() => {
    void loadAll();
    const offCanvas = obs.on("canvas.changed", () => void loadCanvases());
    const offBindings = obs.on("outputBinding.changed", () => {
      loadBindings();
      loadLive();
    });
    const offMulti = obs.on("multistream.changed", (p) => (live = p.outputs));
    const offProfiles = obs.on("streamProfile.changed", loadProfiles);
    return () => {
      offCanvas();
      offBindings();
      offMulti();
      offProfiles();
    };
  });

  const selectedCanvas = $derived(selectedUuid ? (canvases.find((c) => c.uuid === selectedUuid) ?? null) : null);
  const canvasBindings = $derived(bindings.filter((b) => b.canvasUuid === selectedUuid));

  // bindingUuid -> live status row.
  const statusByBinding = $derived.by<Map<string, MultistreamStatus>>(() => {
    const m = new Map<string, MultistreamStatus>();
    for (const o of live) {
      m.set(o.bindingUuid, o);
    }
    return m;
  });

  function fpsText(c: CanvasInfo): string {
    if (!(c.fpsDen > 0)) return String(c.fpsNum);
    return c.fpsDen > 1 ? (c.fpsNum / c.fpsDen).toFixed(2) : String(c.fpsNum);
  }
  function encName(list: EncoderType[], id: string): string {
    return list.find((e) => e.id === id)?.name ?? id ?? "—";
  }

  // The effective state of a destination: disabled bindings never go live.
  function rowState(b: OutputBindingInfo): MultistreamState | "disabled" {
    if (!b.enabled) {
      return "disabled";
    }
    return statusByBinding.get(b.uuid)?.state ?? "idle";
  }
  const STATE_COLOR: Record<MultistreamState | "disabled", string> = {
    disabled: "var(--color-muted)",
    idle: "var(--color-muted)",
    connecting: "var(--meter-yellow)",
    live: "var(--meter-green)",
    error: "var(--color-live)",
  };
  const STATE_TAG_BG: Record<MultistreamState | "disabled", string> = {
    disabled: "color-mix(in srgb, var(--color-muted) 10%, transparent)",
    idle: "color-mix(in srgb, var(--color-muted) 12%, transparent)",
    connecting: "color-mix(in srgb, var(--meter-yellow) 14%, transparent)",
    live: "color-mix(in srgb, var(--meter-green) 14%, transparent)",
    error: "color-mix(in srgb, var(--color-live) 14%, transparent)",
  };
  function titleState(s: MultistreamState | "disabled"): string {
    return s.charAt(0).toUpperCase() + s.slice(1);
  }

  function isDangling(label: string): boolean {
    return label === "(deleted)";
  }
  function isUnset(label: string): boolean {
    return label === "(unset)";
  }
  function rowName(b: OutputBindingInfo): string {
    if (isUnset(b.profileLabel)) {
      return "No destination";
    }
    return b.profileLabel;
  }

  // Per-canvas enabled = any binding enabled; the master toggle sets them all.
  const canvasEnabled = $derived(canvasBindings.some((b) => b.enabled));
  async function toggleCanvas(): Promise<void> {
    const rows = canvasBindings;
    if (rows.length === 0) return;
    const target = !canvasEnabled;
    try {
      await Promise.all(rows.map((b) => obs.call("outputBinding.setEnabled", { uuid: b.uuid, enabled: target })));
      loadBindings();
    } catch (e) {
      error = (e as Error).message;
    }
  }
  async function toggleRow(b: OutputBindingInfo, enabled: boolean): Promise<void> {
    try {
      await obs.call("outputBinding.setEnabled", { uuid: b.uuid, enabled });
      loadBindings();
    } catch (e) {
      error = (e as Error).message;
    }
  }
  async function removeRow(b: OutputBindingInfo): Promise<void> {
    try {
      await obs.call("outputBinding.remove", { uuid: b.uuid });
      loadBindings();
    } catch (e) {
      error = (e as Error).message;
    }
  }

  function startAdd(): void {
    adding = true;
    addProfile = profiles[0]?.uuid ?? "";
  }
  function cancelAdd(): void {
    adding = false;
  }
  async function confirmAdd(): Promise<void> {
    if (!selectedUuid) return;
    try {
      await obs.call("outputBinding.create", {
        canvasUuid: selectedUuid,
        ...(addProfile ? { profileUuid: addProfile } : {}),
      });
      adding = false;
      loadBindings();
    } catch (e) {
      error = (e as Error).message;
    }
  }

  // Inline add-canvas: a name prompt -> canvas.create seeded with the Default
  // canvas's resolution/FPS (mirrors the Studio CANVASES-bar "+"). The new canvas
  // appears via the canvas.changed subscription; select it once it lands.
  function addCanvas(): void {
    dialog = {
      kind: "prompt",
      title: "New Canvas",
      confirmLabel: "Create",
      onCommit: (name) => {
        if (!name) return;
        const def = canvases.find((c) => c.isDefault);
        obs
          .call("canvas.create", {
            name,
            baseWidth: def?.baseWidth ?? 1920,
            baseHeight: def?.baseHeight ?? 1080,
            outputWidth: def?.outputWidth,
            outputHeight: def?.outputHeight,
            fpsNum: def?.fpsNum ?? 60,
            fpsDen: def?.fpsDen ?? 1,
          })
          .then((r) => (selectedUuid = r.uuid))
          .catch(() => {});
      },
    };
  }

  function noop(): void {}
</script>

<div class="page">
  <header class="head">
    <span class="title">Canvases</span>
    <span class="sub">encode targets · one per resolution/FPS</span>
  </header>

  <div class="body">
    <nav class="subnav" aria-label="Canvases">
      {#if loaded}
        {#each canvases as c (c.uuid)}
          <button
            class="nav-item"
            class:active={c.uuid === selectedUuid}
            aria-current={c.uuid === selectedUuid ? "page" : undefined}
            onclick={() => (selectedUuid = c.uuid)}
          >
            <span class="nav-name">
              {c.name}
              {#if c.isDefault}<span class="nav-badge">Default</span>{/if}
            </span>
            <span class="nav-sub">{c.outputWidth}×{c.outputHeight} · {fpsText(c)} fps</span>
          </button>
        {/each}
      {/if}
      <button class="addnav" onclick={addCanvas}>＋ Add canvas</button>
    </nav>

    <div class="pane">
      {#if error}<p class="err">{error}</p>{/if}

      {#if !loaded}
        <p class="dim">Loading canvases…</p>
      {:else if !selectedCanvas}
        <p class="dim">No canvases.</p>
      {:else}
        <section class="section">
          <h3 class="sec-head">Settings</h3>
          {#key selectedUuid}
            <CanvasEditor
              canvas={selectedCanvas}
              {videoEncoders}
              {audioEncoders}
              onClose={noop}
              onSaved={() => void loadCanvases()}
            />
          {/key}
        </section>

        <section class="section">
          <div class="sec-bar">
            <h3 class="sec-head">Destinations</h3>
            <span class="sec-spacer"></span>
            {#if canvasBindings.length > 0}
              <label class="switch" title={canvasEnabled ? "Disable all" : "Enable all"}>
                <input type="checkbox" checked={canvasEnabled} onchange={() => void toggleCanvas()} />
                <span class="track"><span class="thumb"></span></span>
              </label>
            {/if}
          </div>

          {#if canvasBindings.length === 0 && !adding}
            <p class="empty">No destinations bound to this canvas yet.</p>
          {/if}

          <div class="rows">
            {#each canvasBindings as b (b.uuid)}
              {@const s = rowState(b)}
              <div class="row" class:off={!b.enabled}>
                <label class="switch sm" title={b.enabled ? "Disable" : "Enable"}>
                  <input type="checkbox" checked={b.enabled} onchange={(e) => void toggleRow(b, e.currentTarget.checked)} />
                  <span class="track"><span class="thumb"></span></span>
                </label>
                <div class="row-col">
                  <div class="row-line1">
                    <span class="row-name" class:deleted={isDangling(b.profileLabel)} class:unset={isUnset(b.profileLabel)}>
                      {rowName(b)}
                    </span>
                    <span class="row-state" style:color={STATE_COLOR[s]} style:background={STATE_TAG_BG[s]}>
                      {titleState(s).toUpperCase()}
                    </span>
                  </div>
                  <div class="row-sub">{encName(videoEncoders, selectedCanvas.videoEncoder)}</div>
                </div>
                <button class="trash" title="Remove destination" aria-label="Remove destination" onclick={() => void removeRow(b)}>
                  <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.7" stroke-linecap="round">
                    <path d="M4 7h16M9 7V5h6v2M6 7l1 13h10l1-13" />
                  </svg>
                </button>
              </div>
            {/each}

            {#if adding}
              <div class="row add-form">
                <span class="add-label">New destination</span>
                <select bind:value={addProfile}>
                  <option value="">No destination (placeholder)</option>
                  {#each profiles as p (p.uuid)}
                    <option value={p.uuid}>{p.label || p.platform}</option>
                  {/each}
                </select>
                <div class="add-actions">
                  <button class="ghost" onclick={cancelAdd}>Cancel</button>
                  <button class="accent" onclick={() => void confirmAdd()}>Add</button>
                </div>
              </div>
            {:else}
              <button class="add-tile" onclick={startAdd}>
                <span class="add-plus">＋</span>
                <span>Bind destination</span>
              </button>
            {/if}
          </div>
        </section>
      {/if}
    </div>
  </div>
</div>

{#if dialog}
  <CollectionDialog {...dialog} onClose={() => (dialog = null)} />
{/if}

<style>
  .page {
    height: 100%;
    display: flex;
    flex-direction: column;
    min-height: 0;
    background: var(--color-base);
    color: var(--color-text);
  }
  .head {
    flex: 0 0 auto;
    height: 58px;
    display: flex;
    align-items: baseline;
    gap: 12px;
    padding: 0 24px;
    border-bottom: var(--border-weight) solid var(--color-border);
    background: var(--color-surface);
  }
  .title {
    font-family: var(--font-ui);
    font-size: 16px;
    font-weight: 600;
    letter-spacing: -0.01em;
  }
  .sub {
    font-family: var(--font-mono);
    font-size: 11px;
    color: var(--color-muted);
  }
  .body {
    flex: 1;
    min-height: 0;
    display: flex;
  }
  .subnav {
    flex: 0 0 220px;
    border-right: var(--border-weight) solid var(--color-border);
    background: var(--color-surface);
    padding: 10px 0;
    overflow-y: auto;
    display: flex;
    flex-direction: column;
  }
  .nav-item {
    display: flex;
    flex-direction: column;
    align-items: flex-start;
    gap: 2px;
    width: 100%;
    text-align: left;
    padding: 9px 16px;
    background: transparent;
    border: 0;
    border-left: 2.5px solid transparent;
    color: var(--color-dim);
    cursor: pointer;
  }
  .nav-item:hover {
    color: var(--color-text);
  }
  .nav-item.active {
    background: color-mix(in srgb, var(--color-accent) 10%, transparent);
    border-left-color: var(--color-accent);
  }
  .nav-name {
    display: flex;
    align-items: center;
    gap: 7px;
    font-family: var(--font-ui);
    font-size: 12px;
    font-weight: 500;
    color: inherit;
  }
  .nav-item.active .nav-name {
    color: var(--color-accent);
    font-weight: 600;
  }
  .nav-badge {
    font-size: 8px;
    text-transform: uppercase;
    letter-spacing: 0.05em;
    color: var(--color-accent-ink);
    background: var(--color-accent);
    padding: 1px 5px;
  }
  .nav-sub {
    font-family: var(--font-mono);
    font-size: 9px;
    color: var(--color-muted);
  }
  .addnav {
    margin: 8px 12px 4px;
    padding: 8px 10px;
    text-align: left;
    background: transparent;
    border: var(--border-weight) dashed var(--color-border);
    color: var(--color-dim);
    cursor: pointer;
    font-family: var(--font-ui);
    font-size: 12px;
  }
  .addnav:hover {
    border-color: var(--color-accent);
    color: var(--color-accent);
  }

  .pane {
    flex: 1;
    min-width: 0;
    overflow: auto;
    padding: 20px 28px 32px;
  }
  .section {
    margin-bottom: 26px;
  }
  .sec-head {
    margin: 0;
    font-family: var(--font-mono);
    font-size: 10px;
    text-transform: uppercase;
    letter-spacing: 0.08em;
    color: var(--color-muted);
  }
  .sec-bar {
    display: flex;
    align-items: center;
    gap: 12px;
    margin-bottom: 12px;
  }
  .sec-spacer {
    flex: 1;
  }
  .empty {
    margin: 4px 0 12px;
    font-family: var(--font-mono);
    font-size: 11px;
    color: var(--color-muted);
  }
  .rows {
    display: flex;
    flex-direction: column;
    gap: 10px;
  }
  .row {
    display: flex;
    align-items: center;
    gap: 14px;
    padding: 12px 16px;
    border: var(--border-weight) solid var(--color-border);
    background: var(--color-surface);
  }
  .row.off {
    background: var(--color-base);
  }
  .row-col {
    flex: 1;
    min-width: 0;
  }
  .row-line1 {
    display: flex;
    align-items: center;
    gap: 8px;
  }
  .row-name {
    font-size: 14px;
    font-weight: 500;
    color: var(--color-text);
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }
  .row-name.deleted {
    color: var(--color-live);
    font-style: italic;
  }
  .row-name.unset {
    color: var(--color-muted);
    font-style: italic;
    font-weight: 400;
  }
  .row-state {
    flex: 0 0 auto;
    font-family: var(--font-mono);
    font-size: 8px;
    letter-spacing: 0.06em;
    padding: 2px 6px;
  }
  .row-sub {
    margin-top: 3px;
    font-family: var(--font-mono);
    font-size: 10px;
    color: var(--color-muted);
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }
  .trash {
    flex: 0 0 auto;
    display: flex;
    align-items: center;
    justify-content: center;
    width: 28px;
    height: 26px;
    padding: 0;
    background: none;
    border: var(--border-weight) solid var(--color-border);
    color: var(--color-muted);
    cursor: pointer;
  }
  .trash:hover {
    color: var(--color-live);
    border-color: var(--color-live);
  }

  /* enable/disable switch (square thumb per the 0-radius rule). */
  .switch {
    flex: 0 0 auto;
    display: inline-flex;
    align-items: center;
    cursor: pointer;
  }
  .switch input {
    position: absolute;
    opacity: 0;
    width: 0;
    height: 0;
  }
  .track {
    display: block;
    width: 36px;
    height: 20px;
    background: var(--color-base);
    border: var(--border-weight) solid var(--color-border);
    position: relative;
    transition: background 0.12s ease;
  }
  .switch.sm .track {
    width: 32px;
    height: 18px;
  }
  .thumb {
    position: absolute;
    top: 50%;
    left: 3px;
    width: 12px;
    height: 12px;
    transform: translateY(-50%);
    background: var(--color-muted);
    transition:
      left 0.12s ease,
      background 0.12s ease;
  }
  .switch.sm .thumb {
    width: 10px;
    height: 10px;
  }
  .switch input:checked + .track {
    background: color-mix(in srgb, var(--color-accent) 30%, transparent);
    border-color: var(--color-accent);
  }
  .switch input:checked + .track .thumb {
    left: calc(100% - 12px - 3px);
    background: var(--color-accent);
  }
  .switch.sm input:checked + .track .thumb {
    left: calc(100% - 10px - 3px);
  }
  .switch input:focus-visible + .track {
    outline: 1px solid var(--color-accent);
    outline-offset: 1px;
  }

  .add-tile {
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 8px;
    padding: 12px 16px;
    border: var(--border-weight) dashed var(--color-border);
    background: transparent;
    color: var(--color-dim);
    cursor: pointer;
    font-family: var(--font-ui);
    font-size: 12px;
  }
  .add-tile:hover {
    border-color: var(--color-accent);
    color: var(--color-accent);
  }
  .add-plus {
    font-size: 14px;
    line-height: 1;
  }
  .add-form {
    flex-direction: column;
    align-items: stretch;
    gap: 10px;
  }
  .add-label {
    font-family: var(--font-mono);
    font-size: 9px;
    letter-spacing: 0.1em;
    text-transform: uppercase;
    color: var(--color-muted);
  }
  .add-form select {
    background: var(--color-base);
    border: var(--border-weight) solid var(--color-border);
    color: var(--color-text);
    font: inherit;
    font-size: 13px;
    padding: 7px 10px;
  }
  .add-actions {
    display: flex;
    justify-content: flex-end;
    gap: 8px;
  }
  .add-actions .ghost {
    padding: 7px 14px;
    background: none;
    border: var(--border-weight) solid var(--color-border);
    color: var(--color-dim);
    cursor: pointer;
    font: inherit;
    font-size: 12px;
  }
  .add-actions .ghost:hover {
    color: var(--color-text);
  }
  .add-actions .accent {
    padding: 7px 16px;
    background: var(--color-accent);
    border: 0;
    color: var(--color-accent-ink);
    cursor: pointer;
    font: inherit;
    font-size: 12px;
    font-weight: 600;
  }
  .dim {
    color: var(--color-muted);
    margin: 0;
  }
  .err {
    margin: 0 0 12px;
    color: var(--color-live);
    font-size: 12px;
  }
</style>
