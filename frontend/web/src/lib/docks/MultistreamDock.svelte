<script lang="ts">
  import { obs, type MultistreamStatus, type MultistreamState } from "../bridge";
  import { openSettings } from "../settingsOpener.svelte";

  // Host supplies tab chrome + strips __* keys; this body declares no props.
  let {}: Record<string, unknown> = $props();

  // State -> dot color, mapped to theme tokens (re-skins with the active preset).
  const STATE_COLOR: Record<MultistreamState, string> = {
    idle: "var(--color-muted)",
    connecting: "var(--meter-yellow)",
    live: "var(--meter-green)",
    error: "var(--color-live)",
  };

  let outputs = $state<MultistreamStatus[]>([]);
  let loaded = $state(false);
  let error = $state<string | null>(null);

  async function load() {
    try {
      const res = await obs.call("multistream.status");
      outputs = res.outputs;
      error = null;
    } catch (e) {
      error = (e as Error).message;
    } finally {
      loaded = true;
    }
  }

  $effect(() => {
    void load();
    // Authoritative status comes from the engine push; the enabled-binding set
    // changes the row list, so re-fetch on outputBinding.changed.
    const offChanged = obs.on("multistream.changed", (p) => {
      outputs = p.outputs;
    });
    const offBindings = obs.on("outputBinding.changed", () => void load());
    return () => {
      offChanged();
      offBindings();
    };
  });

  function isRunning(state: MultistreamState): boolean {
    return state === "connecting" || state === "live";
  }

  async function toggle(o: MultistreamStatus) {
    error = null;
    try {
      if (isRunning(o.state)) {
        await obs.call("multistream.stopOutput", { uuid: o.bindingUuid });
      } else {
        await obs.call("multistream.startOutput", { uuid: o.bindingUuid });
      }
      // The authoritative row update arrives via multistream.changed.
    } catch (e) {
      error = (e as Error).message;
    }
  }
</script>

<div class="dock">
  <div class="toolbar">
    <button class="add" title="Add output (Settings)" onclick={() => openSettings("outputs")}>＋</button>
  </div>

  {#if error}
    <p class="msg err">{error}</p>
  {/if}

  {#if !loaded}
    <p class="msg">Loading…</p>
  {:else if outputs.length === 0}
    <p class="msg">No enabled outputs — add one in Settings → Outputs.</p>
  {:else}
    <ul class="list">
      {#each outputs as o (o.bindingUuid)}
        <li class="row">
          <span class="dot" style:background={STATE_COLOR[o.state]} title={o.state}></span>
          <div class="info">
            <div class="line1">
              <span class="name">{o.profileLabel}</span>
              <span class="arrow">→</span>
              <span class="canvas">{o.canvasName}</span>
            </div>
            {#if o.state === "error" && o.lastError}
              <div class="lasterr">{o.lastError}</div>
            {/if}
          </div>
          <button class="mini" class:on={isRunning(o.state)} onclick={() => void toggle(o)}>
            {isRunning(o.state) ? "STOP" : "START"}
          </button>
        </li>
      {/each}
    </ul>
  {/if}
</div>

<style>
  .dock {
    height: 100%;
    display: flex;
    flex-direction: column;
    background: var(--color-surface);
    font-family: var(--font-ui);
    overflow: auto;
  }
  .toolbar {
    display: flex;
    justify-content: flex-end;
    padding: 4px 6px;
    border-bottom: var(--border-weight) solid var(--color-border);
  }
  .add {
    height: auto;
    padding: 2px 8px;
    font-size: 13px;
    line-height: 1;
    background: transparent;
    border: var(--border-weight) solid var(--color-border);
    color: var(--color-muted);
  }
  .add:hover {
    color: var(--color-accent);
    border-color: var(--color-accent);
  }
  .list {
    list-style: none;
    margin: 0;
    padding: 6px;
    display: flex;
    flex-direction: column;
    gap: 4px;
  }
  .row {
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 7px 9px;
    border: var(--border-weight) solid var(--color-border);
    background: var(--color-base);
  }
  .dot {
    width: 9px;
    height: 9px;
    flex-shrink: 0;
  }
  .info {
    min-width: 0;
    flex: 1;
  }
  .line1 {
    display: flex;
    align-items: center;
    gap: 6px;
    font-size: 11px;
    min-width: 0;
    letter-spacing: var(--letter-spacing);
    text-transform: var(--label-case);
  }
  .name {
    color: var(--color-text);
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }
  .arrow {
    color: var(--color-muted);
    flex-shrink: 0;
  }
  .canvas {
    color: var(--color-muted);
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }
  .lasterr {
    margin-top: 2px;
    font-size: 10px;
    color: var(--color-live);
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }
  .mini {
    flex-shrink: 0;
    height: auto;
    padding: 4px 10px;
    font-size: 10px;
    letter-spacing: var(--letter-spacing);
    text-transform: var(--label-case);
    background: transparent;
    border: var(--border-weight) solid var(--color-border);
    color: var(--color-text);
  }
  .mini:hover {
    border-color: var(--color-accent);
    color: var(--color-accent);
  }
  .mini.on {
    background: var(--color-live);
    border-color: var(--color-live);
    color: #fff;
  }
  .msg {
    margin: 0;
    padding: 10px 9px;
    font-size: 11px;
    color: var(--color-muted);
    letter-spacing: var(--letter-spacing);
    text-transform: var(--label-case);
  }
  .msg.err {
    color: var(--color-live);
    text-transform: none;
  }
</style>
