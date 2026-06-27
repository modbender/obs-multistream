<script lang="ts">
  import {
    obs,
    type CanvasInfo,
    type OutputBindingInfo,
    type StreamProfileInfo,
    type MultistreamStatus,
    type MultistreamState,
    type Stats,
    type OutputStat,
  } from "../bridge";

  // Destinations page: the single place to manage which canvases + destinations are
  // enabled (the output bindings, moved here from Settings → Outputs) AND to observe
  // live status. Each canvas and each destination within it has an enable/disable
  // toggle; actually going live is the header GO LIVE (or the Studio GO-LIVE bar).
  let canvases = $state<CanvasInfo[]>([]);
  let bindings = $state<OutputBindingInfo[]>([]);
  let profiles = $state<StreamProfileInfo[]>([]);
  let live = $state<MultistreamStatus[]>([]);
  let stats = $state<Stats | null>(null);
  let busy = $state(false);
  let error = $state<string | null>(null);

  // Per-canvas inline "add destination" picker (canvas uuid being added to, + the
  // chosen profile uuid; "" => unset/no destination yet).
  let addingFor = $state<string | null>(null);
  let addProfile = $state("");

  function loadCanvases(): void {
    obs.call("canvas.list").then((l) => (canvases = l)).catch(() => {});
  }
  function loadBindings(): void {
    obs.call("outputBinding.list").then((l) => (bindings = l)).catch(() => {});
  }
  function loadProfiles(): void {
    obs.call("streamProfile.list").then((l) => (profiles = l)).catch(() => {});
  }
  function loadLive(): void {
    obs.call("multistream.status").then((r) => (live = r.outputs)).catch(() => {});
  }
  function loadStats(): void {
    obs.call("stats.get").then((s) => (stats = s)).catch(() => {});
  }

  $effect(() => {
    loadCanvases();
    loadBindings();
    loadProfiles();
    loadLive();
    const offMulti = obs.on("multistream.changed", (p) => (live = p.outputs));
    const offBindings = obs.on("outputBinding.changed", () => {
      loadBindings();
      loadLive();
    });
    const offCanvas = obs.on("canvas.changed", loadCanvases);
    const offProfiles = obs.on("streamProfile.changed", loadProfiles);
    return () => {
      offMulti();
      offBindings();
      offCanvas();
      offProfiles();
    };
  });

  // stats.get has no push; own a 1s poll while the page is mounted.
  $effect(() => {
    loadStats();
    const timer = setInterval(loadStats, 1000);
    return () => clearInterval(timer);
  });

  // bindingUuid -> live state / polled stats row.
  let statusByBinding = $derived.by<Map<string, MultistreamStatus>>(() => {
    const m = new Map<string, MultistreamStatus>();
    for (const o of live) {
      m.set(o.bindingUuid, o);
    }
    return m;
  });
  let statByBinding = $derived.by<Map<string, OutputStat>>(() => {
    const m = new Map<string, OutputStat>();
    if (stats) {
      for (const o of stats.outputs) {
        m.set(o.bindingUuid, o);
      }
    }
    return m;
  });

  // Every canvas that has at least one binding, in canvas.list order, with its
  // bindings attached. Canvases with no destinations are omitted (nothing to toggle).
  let groups = $derived.by<{ canvas: CanvasInfo; rows: OutputBindingInfo[] }[]>(() =>
    canvases
      .map((c) => ({ canvas: c, rows: bindings.filter((b) => b.canvasUuid === c.uuid) }))
      .filter((g) => g.rows.length > 0),
  );

  // The effective state of a destination: disabled bindings never go live.
  function rowState(b: OutputBindingInfo): MultistreamState | "disabled" {
    if (!b.enabled) {
      return "disabled";
    }
    return statusByBinding.get(b.uuid)?.state ?? "idle";
  }
  function isRunning(s: MultistreamState | "disabled"): boolean {
    return s === "live" || s === "connecting";
  }

  let anyRunning = $derived(live.some((o) => isRunning(o.state)));

  // Aggregate header trio over the live per-output stats rows.
  let liveStatRows = $derived(stats ? stats.outputs.filter((o) => o.state === "Live") : []);
  let aggBitrate = $derived((liveStatRows.reduce((s, o) => s + o.bitrateKbps, 0) / 1000).toFixed(1) + " Mb/s");
  let aggDrop = $derived(String(liveStatRows.reduce((s, o) => s + o.droppedFrames, 0)));
  let aggUptime = $derived.by<string>(() => {
    if (liveStatRows.length === 0) {
      return "—";
    }
    let max = 0;
    for (const o of liveStatRows) {
      if (o.durationMs > max) {
        max = o.durationMs;
      }
    }
    return fmtDuration(max);
  });
  function fmtDuration(ms: number): string {
    const s = Math.floor(ms / 1000);
    const pad = (n: number): string => String(n).padStart(2, "0");
    return pad(Math.floor(s / 3600)) + ":" + pad(Math.floor((s % 3600) / 60)) + ":" + pad(s % 60);
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

  function initials(label: string): string {
    const words = label.trim().split(/\s+/).filter(Boolean);
    if (words.length >= 2) {
      return (words[0][0] + words[1][0]).toUpperCase();
    }
    return label.trim().slice(0, 2).toUpperCase() || "—";
  }

  function health(congestionPct: number): number {
    return Math.round(Math.max(0, Math.min(100, 100 - congestionPct)));
  }
  function healthColor(h: number): string {
    return h >= 90 ? "var(--meter-green)" : h >= 70 ? "var(--meter-yellow)" : "var(--color-live)";
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

  // Per-canvas enabled = any of its bindings enabled. Toggling sets them all.
  function canvasEnabled(rows: OutputBindingInfo[]): boolean {
    return rows.some((b) => b.enabled);
  }
  async function toggleCanvas(rows: OutputBindingInfo[]): Promise<void> {
    const target = !canvasEnabled(rows);
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

  function startAdd(canvasUuid: string): void {
    addingFor = canvasUuid;
    addProfile = profiles[0]?.uuid ?? "";
  }
  function cancelAdd(): void {
    addingFor = null;
  }
  async function confirmAdd(canvasUuid: string): Promise<void> {
    try {
      await obs.call("outputBinding.create", {
        canvasUuid,
        ...(addProfile ? { profileUuid: addProfile } : {}),
      });
      addingFor = null;
      loadBindings();
    } catch (e) {
      error = (e as Error).message;
    }
  }

  // GO LIVE / END STREAM drives every enabled binding across all canvases.
  async function toggleAll(): Promise<void> {
    if (busy || live.length === 0) {
      return;
    }
    busy = true;
    try {
      const stopping = anyRunning;
      for (const o of live) {
        if (stopping && isRunning(o.state)) {
          await obs.call("multistream.stopOutput", { uuid: o.bindingUuid }).catch(() => {});
        } else if (!stopping) {
          await obs.call("multistream.startOutput", { uuid: o.bindingUuid }).catch(() => {});
        }
      }
    } finally {
      busy = false;
    }
  }
</script>

<div class="page">
  <header class="head">
    <div class="head-left">
      <span class="title">Destinations</span>
      <span class="sub">encode once · fan out to every platform</span>
    </div>
    <div class="head-right">
      <div class="agg">
        <span class="agg-item"><span class="agg-k">OUT</span><span class="agg-v">{aggBitrate}</span></span>
        <span class="agg-item"><span class="agg-k">DROP</span><span class="agg-v">{aggDrop}</span></span>
        <span class="agg-item"><span class="agg-k">UPTIME</span><span class="agg-v">{aggUptime}</span></span>
      </div>
      <button class="golive" class:running={anyRunning} disabled={busy || live.length === 0} onclick={() => void toggleAll()}>
        {anyRunning ? "■  END STREAM" : "●  GO LIVE"}
      </button>
    </div>
  </header>

  <div class="body">
    {#if error}<p class="err">{error}</p>{/if}

    {#if groups.length === 0}
      <div class="empty">
        <p class="empty-title">No destinations yet</p>
        <p class="empty-sub">Add a stream profile to a canvas to start streaming it. Manage profiles in Settings → Stream Profiles.</p>
      </div>
    {/if}

    {#each groups as g (g.canvas.uuid)}
      {@const cEnabled = canvasEnabled(g.rows)}
      {@const liveCount = g.rows.filter((b) => rowState(b) === "live").length}
      <section class="card" class:off={!cEnabled}>
        <div class="card-head">
          <span class="card-name">{g.canvas.name}</span>
          <span class="card-meta">
            {g.canvas.outputWidth}×{g.canvas.outputHeight} · {Math.round(g.canvas.fpsNum / g.canvas.fpsDen)} fps
          </span>
          <span class="card-spacer"></span>
          <span class="card-tag">{liveCount} / {g.rows.length} live</span>
          <label class="switch" title={cEnabled ? "Disable canvas" : "Enable canvas"}>
            <input type="checkbox" checked={cEnabled} onchange={() => void toggleCanvas(g.rows)} />
            <span class="track"><span class="thumb"></span></span>
          </label>
        </div>

        <div class="dest-grid">
          {#each g.rows as b (b.uuid)}
            {@const s = rowState(b)}
            {@const stat = statByBinding.get(b.uuid)}
            {@const isLive = s === "live"}
            {@const h = isLive && stat ? health(stat.congestionPct) : 0}
            <div class="dest-tile" class:off={!b.enabled}>
              <div class="dest-head">
                <span class="mark">{initials(b.profileLabel)}</span>
                <div class="dest-col">
                  <div class="dest-line1">
                    <span class="dest-name" class:deleted={isDangling(b.profileLabel)} class:unset={isUnset(b.profileLabel)}>
                      {rowName(b)}
                    </span>
                    <span class="dest-state" style:color={STATE_COLOR[s]} style:background={STATE_TAG_BG[s]}>
                      {titleState(s).toUpperCase()}
                    </span>
                  </div>
                  <div class="dest-sub">
                    {#if isLive && stat}
                      {(stat.bitrateKbps / 1000).toFixed(1)} Mb/s · {stat.droppedFrames} dropped
                    {:else if s === "error"}
                      {statusByBinding.get(b.uuid)?.lastError || "error"}
                    {:else if b.enabled}
                      ready to go live
                    {:else}
                      disabled
                    {/if}
                  </div>
                </div>
                <label class="switch sm" title={b.enabled ? "Disable" : "Enable"}>
                  <input type="checkbox" checked={b.enabled} onchange={(e) => void toggleRow(b, e.currentTarget.checked)} />
                  <span class="track"><span class="thumb"></span></span>
                </label>
              </div>

              <div class="health-row">
                <div class="health-track">
                  <div class="health-fill" style:width={h + "%"} style:background={isLive && stat ? healthColor(h) : "transparent"}></div>
                </div>
                <button class="trash" title="Remove destination" aria-label="Remove destination" onclick={() => void removeRow(b)}>
                  <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.7" stroke-linecap="round">
                    <path d="M4 7h16M9 7V5h6v2M6 7l1 13h10l1-13" />
                  </svg>
                </button>
              </div>
            </div>
          {/each}

          {#if addingFor === g.canvas.uuid}
            <div class="dest-tile add-form">
              <span class="add-label">New destination</span>
              <select bind:value={addProfile}>
                <option value="">No destination (placeholder)</option>
                {#each profiles as p (p.uuid)}
                  <option value={p.uuid}>{p.label || p.platform}</option>
                {/each}
              </select>
              <div class="add-actions">
                <button class="ghost" onclick={cancelAdd}>Cancel</button>
                <button class="accent" onclick={() => void confirmAdd(g.canvas.uuid)}>Add</button>
              </div>
            </div>
          {:else}
            <button class="dest-tile add-tile" onclick={() => startAdd(g.canvas.uuid)}>
              <span class="add-plus">+</span>
              <span>Add destination</span>
            </button>
          {/if}
        </div>
      </section>
    {/each}
  </div>
</div>

<style>
  .page {
    height: 100%;
    display: flex;
    flex-direction: column;
    background: var(--color-base);
    color: var(--color-text);
  }
  .head {
    flex: 0 0 auto;
    height: 58px;
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 16px;
    padding: 0 24px;
    border-bottom: var(--border-weight) solid var(--color-border);
    background: var(--color-surface);
  }
  .head-left {
    display: flex;
    align-items: baseline;
    gap: 12px;
    min-width: 0;
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
  .head-right {
    display: flex;
    align-items: center;
    gap: 18px;
    flex: 0 0 auto;
  }
  .agg {
    display: flex;
    align-items: center;
    gap: 16px;
    font-family: var(--font-mono);
  }
  .agg-item {
    display: flex;
    align-items: baseline;
    gap: 6px;
  }
  .agg-k {
    font-size: 9px;
    letter-spacing: 0.08em;
    color: var(--color-muted);
  }
  .agg-v {
    font-size: 12px;
    color: var(--color-dim);
  }
  .golive {
    display: flex;
    align-items: center;
    gap: 8px;
    height: auto;
    background: var(--color-accent);
    color: var(--color-accent-ink);
    border: 0;
    font-family: var(--font-ui);
    font-size: 12px;
    font-weight: 600;
    letter-spacing: 0.02em;
    padding: 9px 20px;
  }
  .golive.running {
    background: var(--color-live);
    color: #fff;
  }
  .golive:disabled {
    opacity: 0.5;
    cursor: default;
  }

  .body {
    flex: 1;
    min-height: 0;
    overflow: auto;
    padding: 22px 24px;
    display: flex;
    flex-direction: column;
    gap: 18px;
  }
  .err {
    margin: 0;
    color: var(--color-live);
    font-size: 12px;
  }
  .empty {
    margin-top: 40px;
    text-align: center;
  }
  .empty-title {
    margin: 0 0 6px;
    font-size: 15px;
    font-weight: 600;
    color: var(--color-text);
  }
  .empty-sub {
    margin: 0;
    font-family: var(--font-mono);
    font-size: 11px;
    color: var(--color-muted);
  }

  .card {
    border: var(--border-weight) solid var(--color-border);
    background: var(--color-surface);
  }
  .card.off {
    opacity: 0.72;
  }
  .card-head {
    display: flex;
    align-items: center;
    gap: 12px;
    padding: 14px 18px;
    border-bottom: var(--border-weight) solid var(--color-border);
  }
  .card-name {
    font-size: 14px;
    font-weight: 600;
    color: var(--color-text);
  }
  .card-meta {
    font-family: var(--font-mono);
    font-size: 10px;
    letter-spacing: 0.06em;
    color: var(--color-muted);
  }
  .card-spacer {
    flex: 1;
  }
  .card-tag {
    font-family: var(--font-mono);
    font-size: 9px;
    letter-spacing: 0.06em;
    text-transform: uppercase;
    color: var(--color-dim);
  }

  .dest-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(320px, 1fr));
    gap: 14px;
    padding: 16px 18px;
  }
  .dest-tile {
    display: flex;
    flex-direction: column;
    gap: 14px;
    padding: 16px;
    border: var(--border-weight) solid var(--color-border);
    background: var(--color-surface-2);
  }
  .dest-tile.off {
    background: var(--color-base);
  }
  .dest-head {
    display: flex;
    align-items: center;
    gap: 12px;
  }
  .mark {
    flex: 0 0 auto;
    width: 38px;
    height: 38px;
    display: flex;
    align-items: center;
    justify-content: center;
    font-family: var(--font-mono);
    font-size: 13px;
    color: var(--color-dim);
    background: var(--color-surface);
    border: var(--border-weight) solid var(--color-border);
  }
  .dest-col {
    flex: 1;
    min-width: 0;
  }
  .dest-line1 {
    display: flex;
    align-items: center;
    gap: 8px;
  }
  .dest-name {
    font-size: 14px;
    font-weight: 500;
    color: var(--color-text);
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }
  .dest-name.deleted {
    color: var(--color-live);
    font-style: italic;
  }
  .dest-name.unset {
    color: var(--color-muted);
    font-style: italic;
    font-weight: 400;
  }
  .dest-state {
    flex: 0 0 auto;
    font-family: var(--font-mono);
    font-size: 8px;
    letter-spacing: 0.06em;
    padding: 2px 6px;
  }
  .dest-sub {
    margin-top: 3px;
    font-family: var(--font-mono);
    font-size: 10px;
    color: var(--color-muted);
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }
  .health-row {
    display: flex;
    align-items: center;
    gap: 12px;
  }
  .health-track {
    flex: 1;
    height: 5px;
    background: var(--color-base);
  }
  .health-fill {
    height: 100%;
    transition: width 0.3s ease;
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
    display: inline-block;
    width: 38px;
    height: 20px;
    background: var(--color-base);
    border: var(--border-weight) solid var(--color-border);
    position: relative;
    transition: background 0.12s ease;
  }
  .switch.sm .track {
    width: 34px;
    height: 18px;
  }
  .thumb {
    position: absolute;
    top: 2px;
    left: 2px;
    width: 12px;
    height: 12px;
    background: var(--color-muted);
    transition:
      transform 0.12s ease,
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
    transform: translateX(18px);
    background: var(--color-accent);
  }
  .switch.sm input:checked + .track .thumb {
    transform: translateX(16px);
  }
  .switch input:focus-visible + .track {
    outline: 1px solid var(--color-accent);
    outline-offset: 1px;
  }

  .add-tile {
    align-items: center;
    justify-content: center;
    gap: 8px;
    flex-direction: row;
    height: auto;
    border-style: dashed;
    background: transparent;
    color: var(--color-dim);
    font-family: var(--font-ui);
    font-size: 12px;
  }
  .add-tile:hover {
    border-color: var(--color-accent);
    color: var(--color-accent);
  }
  .add-plus {
    font-size: 16px;
    line-height: 1;
  }
  .add-form {
    gap: 10px;
  }
  .add-label {
    font-family: var(--font-mono);
    font-size: 9px;
    letter-spacing: 0.1em;
    text-transform: uppercase;
    color: var(--color-muted);
  }
  .add-actions {
    display: flex;
    justify-content: flex-end;
    gap: 8px;
  }
  .add-actions .ghost {
    height: auto;
    padding: 7px 14px;
    background: none;
    font-size: 12px;
  }
  .add-actions .accent {
    height: auto;
    padding: 7px 16px;
    font-size: 12px;
  }
</style>
