<script lang="ts">
  import { obs } from "./bridge";
  import { openSettings } from "./settingsOpener.svelte";

  // Streaming toggle mirrors TopBar's logic: seed from getStreamingState, follow
  // streaming.changed, and guard re-entrancy with `busy` while a call is in flight.
  let live = $state(false);
  let busy = $state(false);

  async function refresh() {
    try {
      const st = await obs.call("getStreamingState");
      live = !!st?.active;
    } catch {
      // leave previous state; status surfaces hard failures elsewhere
    }
  }

  async function toggle() {
    busy = true;
    try {
      const r = await obs.call(live ? "streaming.stop" : "streaming.start");
      live = !!r?.active;
    } catch (e) {
      console.log("OBSLOG: streaming toggle ERR: " + (e as Error).message);
    } finally {
      busy = false;
    }
  }

  $effect(() => {
    refresh();
    return obs.on("streaming.changed", (p) => (live = !!p?.active));
  });
</script>

<section class="panel">
  <header>
    <h2>Controls</h2>
  </header>
  <div class="buttons">
    <button class="stream" class:live onclick={toggle} disabled={busy}>
      {live ? "Stop Streaming" : "Start Streaming"}
    </button>
    <button class="ghost" disabled title="Coming soon">Studio Mode</button>
    <button class="ghost" onclick={() => openSettings("video")}>Settings</button>
  </div>
</section>

<style>
  .panel {
    border: 1px solid var(--border);
    border-radius: 10px;
    background: var(--bg-raised);
    padding: 14px 16px;
    display: flex;
    flex-direction: column;
    gap: 10px;
    min-height: 0;
    overflow: auto;
  }
  header {
    display: flex;
    align-items: center;
    justify-content: space-between;
  }
  h2 {
    margin: 0;
    font-size: 13px;
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 0.05em;
    color: var(--text-dim);
  }
  .buttons {
    display: flex;
    flex-direction: column;
    gap: 8px;
  }
  .buttons button {
    width: 100%;
    text-align: center;
  }
  .stream.live {
    background: var(--off);
  }
  .stream.live:hover {
    background: #ef4444;
  }
</style>
