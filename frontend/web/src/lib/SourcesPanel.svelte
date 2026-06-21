<script lang="ts">
  import { obs, type SceneItem, type ReorderDirection } from "./bridge";
  import { sceneState } from "./scenes.svelte";
  import PropertyForm from "./properties/PropertyForm.svelte";

  let items = $state<SceneItem[]>([]);
  let loaded = $state(false);
  let error = $state<string | null>(null);
  let selectedItemId = $state<number | null>(null);
  // The source whose properties dialog is open (its source name), or null.
  let propsForSource = $state<string | null>(null);

  function openProperties(item: SceneItem) {
    if (item.source) propsForSource = item.source;
  }

  async function load() {
    if (!sceneState.current) {
      items = [];
      loaded = true;
      return;
    }
    error = null;
    try {
      items = await obs.call("sceneItems.list", { scene: sceneState.current });
      // Drop a stale selection if the item is gone.
      if (selectedItemId !== null && !items.some((i) => i.id === selectedItemId)) {
        selectedItemId = null;
      }
    } catch (e) {
      error = (e as Error).message;
    } finally {
      loaded = true;
    }
  }

  // Reload whenever the current scene changes.
  $effect(() => {
    // Touch the dependency so the effect re-runs on scene switch.
    void sceneState.current;
    void load();
  });

  // Refresh on scene-item mutations that target the scene we're showing.
  $effect(() => {
    const off = obs.on("sceneItems.changed", (p) => {
      if (!p.scene || p.scene === sceneState.current) {
        void load();
      }
    });
    return off;
  });

  function reportError(e: unknown) {
    error = (e as Error).message;
  }

  async function toggleVisible(item: SceneItem) {
    try {
      await obs.call("sceneItems.setVisible", {
        scene: sceneState.current,
        id: item.id,
        visible: !item.visible,
      });
    } catch (e) {
      reportError(e);
    }
  }

  async function toggleLocked(item: SceneItem) {
    try {
      await obs.call("sceneItems.setLocked", {
        scene: sceneState.current,
        id: item.id,
        locked: !item.locked,
      });
    } catch (e) {
      reportError(e);
    }
  }

  async function reorder(item: SceneItem, direction: ReorderDirection) {
    try {
      await obs.call("sceneItems.reorder", { scene: sceneState.current, id: item.id, direction });
    } catch (e) {
      reportError(e);
    }
  }

  async function remove(item: SceneItem) {
    try {
      await obs.call("sceneItems.remove", { scene: sceneState.current, id: item.id });
    } catch (e) {
      reportError(e);
    }
  }
</script>

<section class="panel">
  <header><h2>Sources</h2></header>

  {#if error}
    <p class="error">{error}</p>
  {:else if !sceneState.current}
    <p class="dim">No scene selected</p>
  {:else if !loaded}
    <p class="dim">Loading…</p>
  {:else if items.length === 0}
    <p class="dim">No sources</p>
  {:else}
    <ul>
      {#each items as item, idx (item.id)}
        <li class:selected={item.id === selectedItemId} class:hidden-src={!item.visible}>
          <button
            class="icon"
            title={item.visible ? "Hide" : "Show"}
            onclick={() => void toggleVisible(item)}>{item.visible ? "👁" : "🚫"}</button
          >
          <button
            class="icon"
            title={item.locked ? "Unlock" : "Lock"}
            onclick={() => void toggleLocked(item)}>{item.locked ? "🔒" : "🔓"}</button
          >
          <button
            class="name"
            onclick={() => (selectedItemId = item.id)}
            ondblclick={() => openProperties(item)}
          >
            {item.source ?? "(unnamed)"}
          </button>
          <span class="row-actions">
            <button class="icon" title="Properties" onclick={() => openProperties(item)}>⚙</button>
            <button
              class="icon"
              title="Move up"
              disabled={idx === 0}
              onclick={() => void reorder(item, "up")}>▲</button
            >
            <button
              class="icon"
              title="Move down"
              disabled={idx === items.length - 1}
              onclick={() => void reorder(item, "down")}>▼</button
            >
            <button class="icon" title="Remove" onclick={() => void remove(item)}>🗑</button>
          </span>
        </li>
      {/each}
    </ul>
  {/if}
</section>

{#if propsForSource}
  <div
    class="modal-backdrop"
    role="presentation"
    onclick={(e) => {
      if (e.target === e.currentTarget) propsForSource = null;
    }}
  >
    <div class="modal" role="dialog" aria-modal="true" aria-label="Source properties">
      <header class="modal-head">
        <h3>Properties — {propsForSource}</h3>
        <button class="icon close" title="Close" onclick={() => (propsForSource = null)}>✕</button>
      </header>
      <div class="modal-body">
        <PropertyForm kind="source" ref={propsForSource} />
      </div>
    </div>
  </div>
{/if}

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
    display: flex;
    align-items: center;
    gap: 4px;
    padding: 4px 6px;
    border-radius: 6px;
  }
  li.selected {
    background: color-mix(in srgb, var(--accent) 22%, transparent);
  }
  li.hidden-src .name {
    color: var(--text-dim);
    text-decoration: line-through;
  }
  .name {
    flex: 1;
    text-align: left;
    background: none;
    border: none;
    font: inherit;
    cursor: pointer;
    color: var(--text-soft);
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
    padding: 3px 4px;
  }
  li.selected .name {
    color: var(--text);
  }
  .row-actions {
    display: none;
    gap: 2px;
  }
  li:hover .row-actions {
    display: inline-flex;
  }
  .icon {
    background: none;
    border: none;
    color: var(--text-dim);
    cursor: pointer;
    padding: 2px 4px;
    border-radius: 4px;
    font-size: 12px;
    line-height: 1;
  }
  .icon:hover:not(:disabled) {
    color: var(--text);
    background: var(--bg-sunken);
  }
  .icon:disabled {
    opacity: 0.3;
    cursor: default;
  }
  .dim {
    color: var(--text-dim);
    margin: 0;
  }
  .error {
    color: var(--off);
    margin: 0;
    font-size: 12px;
  }
  .modal-backdrop {
    position: fixed;
    inset: 0;
    background: rgba(0, 0, 0, 0.55);
    display: flex;
    align-items: center;
    justify-content: center;
    z-index: 100;
    padding: 24px;
  }
  .modal {
    background: var(--bg-raised);
    border: 1px solid var(--border);
    border-radius: 12px;
    width: min(560px, 100%);
    max-height: 80vh;
    display: flex;
    flex-direction: column;
    box-shadow: 0 18px 48px rgba(0, 0, 0, 0.5);
  }
  .modal-head {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 14px 18px;
    border-bottom: 1px solid var(--border);
  }
  .modal-head h3 {
    margin: 0;
    font-size: 14px;
    font-weight: 600;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }
  .modal-body {
    padding: 18px;
    overflow: auto;
  }
  .close {
    font-size: 14px;
  }
</style>
