<script lang="ts">
  import { obs } from "./bridge";

  let scenes = $state<string[]>([]);
  let current = $state<string | null>(null);
  let error = $state<string | null>(null);
  let loaded = $state(false);

  async function load() {
    error = null;
    try {
      [scenes, current] = await Promise.all([
        obs.call("listScenes"),
        obs.call("getCurrentScene"),
      ]);
    } catch (e) {
      error = (e as Error).message;
    } finally {
      loaded = true;
    }
  }

  load();
</script>

<section class="panel">
  <header>
    <h2>Scenes</h2>
    <button class="ghost" onclick={load}>Refresh</button>
  </header>

  {#if error}
    <p class="error">{error}</p>
  {:else if !loaded}
    <p class="dim">Loading…</p>
  {:else if scenes.length === 0}
    <p class="dim">No scenes</p>
  {:else}
    <ul>
      {#each scenes as scene (scene)}
        <li class:active={scene === current}>{scene}</li>
      {/each}
    </ul>
  {/if}
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
  ul {
    list-style: none;
    margin: 0;
    padding: 0;
    display: flex;
    flex-direction: column;
    gap: 2px;
  }
  li {
    padding: 7px 10px;
    border-radius: 6px;
    color: var(--text-soft);
  }
  li.active {
    background: color-mix(in srgb, var(--accent) 22%, transparent);
    color: var(--text);
  }
  .dim {
    color: var(--text-dim);
    margin: 0;
  }
  .error {
    color: var(--off);
    margin: 0;
  }
</style>
