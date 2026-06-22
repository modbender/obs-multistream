<script lang="ts">
  import { obs, type CanvasInfo, type MultistreamStatus } from "./lib/bridge";
  import { canvasFocus, setFocusedCanvas } from "./lib/canvasFocus.svelte";
  import TopBar from "./lib/TopBar.svelte";
  import ScenesPanel from "./lib/ScenesPanel.svelte";
  import SourcesPanel from "./lib/SourcesPanel.svelte";
  import PreviewArea from "./lib/PreviewArea.svelte";
  import CanvasPanel from "./lib/CanvasPanel.svelte";
  import MultistreamPanel from "./lib/MultistreamPanel.svelte";
  import AudioMixer from "./lib/AudioMixer.svelte";
  import SceneTransitions from "./lib/SceneTransitions.svelte";
  import Controls from "./lib/Controls.svelte";

  let version = $state<string>("…");

  // Canvas list drives the layout: the Default canvas is the central preview +
  // left Scenes dock; non-Default enabled canvases become right-column side docks.
  let canvases = $state<CanvasInfo[]>([]);
  let loaded = $state(false);
  // One representative live-status row per canvas (for the side docks' dots).
  let statusByCanvas = $state<Record<string, MultistreamStatus>>({});

  const defaultCanvas = $derived(canvases.find((c) => c.isDefault) ?? null);
  const additionals = $derived(canvases.filter((c) => !c.isDefault && c.enabled));

  $effect(() => {
    obs
      .call("getVersion")
      .then((v) => (version = v || "unknown"))
      .catch((e) => (version = "error: " + (e as Error).message));
  });

  async function load() {
    try {
      canvases = await obs.call("canvas.list");
    } catch {
      // keep prior list; status bar / docks surface failures elsewhere
    } finally {
      loaded = true;
    }
  }

  // Default the shared Sources focus to the Default canvas once known, without
  // stomping a focus the user later set by clicking a side dock.
  $effect(() => {
    const def = defaultCanvas;
    if (def && canvasFocus.uuid === null) setFocusedCanvas(def.uuid);
  });

  function rank(s: MultistreamStatus["state"]): number {
    return s === "live" ? 3 : s === "connecting" ? 2 : s === "error" ? 1 : 0;
  }

  function applyStatuses(rows: MultistreamStatus[]) {
    const next: Record<string, MultistreamStatus> = {};
    for (const r of rows) {
      const prev = next[r.canvasUuid];
      if (!prev || rank(r.state) > rank(prev.state)) next[r.canvasUuid] = r;
    }
    statusByCanvas = next;
  }

  $effect(() => {
    void load();
    // canvas.changed reshapes the list; an output enable/disable flips a canvas's
    // `enabled` gate, so re-fetch on both.
    const offCanvas = obs.on("canvas.changed", () => void load());
    const offBinding = obs.on("outputBinding.changed", () => void load());
    const offStatus = obs.on("multistream.changed", (p) => applyStatuses(p.outputs ?? []));
    obs
      .call("multistream.status")
      .then((r) => applyStatuses(r.outputs ?? []))
      .catch(() => {});
    return () => {
      offCanvas();
      offBinding();
      offStatus();
    };
  });
</script>

<div class="shell">
  <TopBar />

  <main>
    <aside class="left">
      {#if defaultCanvas}
        <ScenesPanel canvas={defaultCanvas.uuid} isDefault={true} />
      {/if}
      <SourcesPanel />
    </aside>

    <div class="center">
      {#if defaultCanvas}
        <PreviewArea canvas={defaultCanvas.uuid} label={defaultCanvas.name} />
      {:else if loaded}
        <div class="center-empty"><p class="dim">No Default canvas</p></div>
      {/if}
    </div>

    <aside class="right">
      {#each additionals as c (c.uuid)}
        <CanvasPanel canvas={c} status={statusByCanvas[c.uuid] ?? null} />
      {/each}
      <MultistreamPanel />
    </aside>

    <div class="bottom">
      <AudioMixer />
      <SceneTransitions />
      <Controls />
    </div>
  </main>

  <footer class="statusbar">
    <span>libobs {version}</span>
  </footer>
</div>

<style>
  .shell {
    display: flex;
    flex-direction: column;
    height: 100%;
  }
  main {
    flex: 1;
    display: grid;
    grid-template-columns: 280px 1fr 320px;
    grid-template-rows: 1fr auto;
    grid-template-areas:
      "left center right"
      "bottom bottom bottom";
    gap: 14px;
    padding: 14px;
    min-height: 0;
  }
  .left {
    grid-area: left;
    display: flex;
    flex-direction: column;
    gap: 14px;
    min-height: 0;
    overflow: auto;
  }
  .center {
    grid-area: center;
    display: flex;
    flex-direction: column;
    min-height: 0;
    min-width: 0;
  }
  .center-empty {
    flex: 1;
    display: flex;
    align-items: center;
    justify-content: center;
    border: 1px dashed var(--border);
    border-radius: 10px;
  }
  .right {
    grid-area: right;
    display: flex;
    flex-direction: column;
    gap: 14px;
    min-height: 0;
    overflow: auto;
  }
  .bottom {
    grid-area: bottom;
    display: grid;
    grid-template-columns: 1fr 1fr 240px;
    gap: 14px;
    min-height: 0;
  }
  .dim {
    color: var(--text-dim);
    margin: 0;
    font-size: 12px;
  }
  .statusbar {
    border-top: 1px solid var(--border);
    background: var(--bg-raised);
    padding: 6px 18px;
    font-size: 12px;
    color: var(--text-dim);
  }
</style>
