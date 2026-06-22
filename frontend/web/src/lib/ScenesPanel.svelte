<script lang="ts">
  import { onDestroy } from "svelte";
  import { obs } from "./bridge";
  import { createCanvasSceneStore } from "./canvasScenes.svelte";

  interface Props {
    /** The canvas whose scenes this dock lists/mutates (the Default uuid). */
    canvas: string;
    /** True for the Default canvas, so its store also reacts to global-path events. */
    isDefault: boolean;
  }
  let { canvas, isDefault }: Props = $props();

  // canvas/isDefault are stable for this instance: App keys ScenesPanel by the
  // Default uuid, so a canvas swap remounts the panel rather than mutating these.
  // svelte-ignore state_referenced_locally
  const uuid = canvas;
  // svelte-ignore state_referenced_locally
  const store = createCanvasSceneStore(uuid, isDefault);
  const sceneState = store.state;

  onDestroy(() => store.dispose());

  let adding = $state(false);
  let newName = $state("");
  let renamingFrom = $state<string | null>(null);
  let renameTo = $state("");
  let actionError = $state<string | null>(null);

  function focusOnMount(node: HTMLInputElement) {
    node.focus();
    node.select();
  }

  function reportError(e: unknown) {
    actionError = (e as Error).message;
  }

  async function setCurrent(name: string) {
    if (name === sceneState.current) return;
    actionError = null;
    try {
      await obs.call("scenes.setCurrent", { canvas: uuid, name });
    } catch (e) {
      reportError(e);
    }
  }

  function beginAdd() {
    actionError = null;
    adding = true;
    newName = "";
  }

  async function commitAdd() {
    const name = newName.trim();
    if (!name) {
      adding = false;
      return;
    }
    actionError = null;
    try {
      await obs.call("scenes.create", { canvas: uuid, name });
      await obs.call("scenes.setCurrent", { canvas: uuid, name });
      adding = false;
    } catch (e) {
      reportError(e);
    }
  }

  function beginRename(name: string) {
    actionError = null;
    renamingFrom = name;
    renameTo = name;
  }

  async function commitRename() {
    const from = renamingFrom;
    const to = renameTo.trim();
    renamingFrom = null;
    if (!from || !to || to === from) return;
    actionError = null;
    try {
      await obs.call("scenes.rename", { canvas: uuid, from, to });
    } catch (e) {
      reportError(e);
    }
  }

  async function remove(name: string) {
    actionError = null;
    try {
      await obs.call("scenes.remove", { canvas: uuid, name });
    } catch (e) {
      reportError(e);
    }
  }

  function onAddKey(e: KeyboardEvent) {
    if (e.key === "Enter") void commitAdd();
    else if (e.key === "Escape") adding = false;
  }

  function onRenameKey(e: KeyboardEvent) {
    if (e.key === "Enter") void commitRename();
    else if (e.key === "Escape") renamingFrom = null;
  }
</script>

<section class="panel">
  <header>
    <h2>Scenes</h2>
    <button class="icon add" title="Add scene" onclick={beginAdd}>＋</button>
  </header>

  {#if sceneState.error}
    <p class="error">{sceneState.error}</p>
  {:else if !sceneState.loaded}
    <p class="dim">Loading…</p>
  {:else}
    <ul>
      {#each sceneState.scenes as scene (scene.name)}
        <li class:active={scene.name === sceneState.current}>
          {#if renamingFrom === scene.name}
            <input
              class="inline-input"
              bind:value={renameTo}
              onkeydown={onRenameKey}
              onblur={commitRename}
              use:focusOnMount
            />
          {:else}
            <button
              class="scene-name"
              ondblclick={() => beginRename(scene.name)}
              onclick={() => setCurrent(scene.name)}
            >
              {scene.name}
            </button>
            <span class="row-actions">
              <button class="icon" title="Rename" onclick={() => beginRename(scene.name)}>✎</button>
              <button
                class="icon"
                title="Remove"
                disabled={sceneState.scenes.length <= 1}
                onclick={() => remove(scene.name)}>🗑</button
              >
            </span>
          {/if}
        </li>
      {/each}

      {#if adding}
        <li>
          <input
            class="inline-input"
            placeholder="Scene name"
            bind:value={newName}
            onkeydown={onAddKey}
            onblur={commitAdd}
            use:focusOnMount
          />
        </li>
      {/if}
    </ul>

    {#if sceneState.scenes.length === 0 && !adding}
      <p class="dim">No scenes</p>
    {/if}
  {/if}

  {#if actionError}
    <p class="error">{actionError}</p>
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
  .add {
    font-size: 16px;
    line-height: 1;
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
  li.active {
    background: color-mix(in srgb, var(--accent) 22%, transparent);
  }
  .scene-name {
    flex: 1;
    text-align: left;
    background: none;
    border: none;
    color: var(--text-soft);
    cursor: pointer;
    padding: 3px 4px;
    font: inherit;
    font-size: 12px;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }
  li.active .scene-name {
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
    opacity: 0.35;
    cursor: default;
  }
  .inline-input {
    flex: 1;
    background: var(--bg-sunken);
    border: 1px solid var(--accent);
    border-radius: 4px;
    color: var(--text);
    padding: 3px 5px;
    font: inherit;
    font-size: 12px;
  }
  .dim {
    color: var(--text-dim);
    margin: 0;
    font-size: 12px;
  }
  .error {
    color: var(--off, #d65a5a);
    margin: 0;
    font-size: 12px;
  }
</style>
