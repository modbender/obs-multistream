<script lang="ts">
  import { obs } from "./bridge";

  interface Entry {
    ts: string;
    text: string;
  }

  let open = $state(true);
  let entries = $state<Entry[]>([]);

  function push(text: string) {
    const ts = new Date().toLocaleTimeString();
    entries = [...entries, { ts, text }].slice(-200);
    console.log("OBSLOG: " + text);
  }

  $effect(() => {
    const offStreaming = obs.on("streaming.changed", (p) =>
      push("streaming.changed -> " + JSON.stringify(p)),
    );
    const offObs = obs.on("obs.event", (p) => push("obs.event -> " + JSON.stringify(p)));
    return () => {
      offStreaming();
      offObs();
    };
  });
</script>

<section class="panel">
  <header>
    <button class="ghost toggle" onclick={() => (open = !open)}>
      {open ? "▾" : "▸"} Event Log
      <span class="count">{entries.length}</span>
    </button>
    {#if open}
      <button class="ghost" onclick={() => (entries = [])}>Clear</button>
    {/if}
  </header>

  {#if open}
    <div class="log">
      {#if entries.length === 0}
        <span class="dim">No events yet</span>
      {:else}
        {#each entries as e (e.ts + e.text)}
          <div class="line"><span class="ts">[{e.ts}]</span> {e.text}</div>
        {/each}
      {/if}
    </div>
  {/if}
</section>

<style>
  .panel {
    border: 1px solid var(--border);
    border-radius: 10px;
    background: var(--bg-raised);
    padding: 10px 12px;
    display: flex;
    flex-direction: column;
    gap: 8px;
  }
  header {
    display: flex;
    align-items: center;
    justify-content: space-between;
  }
  .toggle {
    display: inline-flex;
    align-items: center;
    gap: 8px;
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 0.05em;
    font-size: 12px;
  }
  .count {
    background: var(--bg-sunken);
    border-radius: 999px;
    padding: 1px 8px;
    font-size: 11px;
    color: var(--text-dim);
  }
  .log {
    font-family: "Cascadia Code", Consolas, monospace;
    font-size: 12px;
    background: var(--bg-sunken);
    border: 1px solid var(--border);
    border-radius: 8px;
    padding: 10px;
    max-height: 160px;
    overflow: auto;
    color: #b6bcc6;
  }
  .line {
    white-space: pre-wrap;
    word-break: break-word;
  }
  .ts {
    color: var(--text-dim);
  }
  .dim {
    color: var(--text-dim);
  }
</style>
