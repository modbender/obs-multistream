<script lang="ts">
  import { onMount } from "svelte";
  import { defaultCanvas } from "./defaultCanvasStore.svelte";

  // The mount adapter strips internal __* keys; this dock declares no props.
  let {}: Record<string, unknown> = $props();

  onMount(() => defaultCanvas.start());

  let adding = $state(false);
  let newName = $state("");
  let renamingFrom = $state<string | null>(null);
  let renameTo = $state("");
  let actionError = $state<string | null>(null);

  function focusOnMount(node: HTMLInputElement) {
    node.focus();
    node.select();
  }

  function report(e: unknown) {
    actionError = (e as Error).message;
  }

  function setCurrent(name: string) {
    actionError = null;
    defaultCanvas.setCurrent(name).catch(report);
  }

  function beginAdd() {
    actionError = null;
    adding = true;
    newName = "";
  }

  async function commitAdd() {
    const name = newName.trim();
    adding = false;
    if (!name) {
      return;
    }
    actionError = null;
    try {
      await defaultCanvas.create(name);
    } catch (e) {
      report(e);
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
    if (!from || !to || to === from) {
      return;
    }
    actionError = null;
    try {
      await defaultCanvas.rename(from, to);
    } catch (e) {
      report(e);
    }
  }

  async function remove(name: string) {
    actionError = null;
    try {
      await defaultCanvas.remove(name);
    } catch (e) {
      report(e);
    }
  }

  function onAddKey(e: KeyboardEvent) {
    if (e.key === "Enter") {
      void commitAdd();
    } else if (e.key === "Escape") {
      adding = false;
    }
  }

  function onRenameKey(e: KeyboardEvent) {
    if (e.key === "Enter") {
      void commitRename();
    } else if (e.key === "Escape") {
      renamingFrom = null;
    }
  }
</script>

<div class="dock">
  <div class="toolbar">
    <button class="add" title="Add scene" onclick={beginAdd}>＋</button>
  </div>

  {#if defaultCanvas.error}
    <p class="msg err">{defaultCanvas.error}</p>
  {:else if !defaultCanvas.loaded}
    <p class="msg">Loading…</p>
  {:else}
    <ul class="list">
      {#each defaultCanvas.scenes as scene (scene.name)}
        <li class="row" class:sel={scene.current}>
          {#if renamingFrom === scene.name}
            <input
              class="inline"
              bind:value={renameTo}
              onkeydown={onRenameKey}
              onblur={commitRename}
              use:focusOnMount
            />
          {:else}
            <button class="label" ondblclick={() => beginRename(scene.name)} onclick={() => setCurrent(scene.name)}>
              {scene.name}
            </button>
            <span class="actions">
              <button class="icon" title="Rename" onclick={() => beginRename(scene.name)}>✎</button>
              <button
                class="icon"
                title="Remove"
                disabled={defaultCanvas.scenes.length <= 1}
                onclick={() => void remove(scene.name)}>🗑</button
              >
            </span>
          {/if}
        </li>
      {/each}

      {#if adding}
        <li class="row">
          <input
            class="inline"
            placeholder="Scene name"
            bind:value={newName}
            onkeydown={onAddKey}
            onblur={commitAdd}
            use:focusOnMount
          />
        </li>
      {/if}
    </ul>

    {#if defaultCanvas.scenes.length === 0 && !adding}
      <p class="msg">No scenes</p>
    {/if}
  {/if}

  {#if actionError}
    <p class="msg err">{actionError}</p>
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
    padding: 0;
    flex: 1;
    min-height: 0;
  }
  .row {
    display: flex;
    align-items: center;
    gap: 4px;
    padding: 5px 7px;
    border-bottom: var(--border-weight) solid var(--color-border);
    border-left: 3px solid transparent;
  }
  /* Selection style is token-driven (visual-style.html): Industrial = left bar,
     Slate = fill. Reflected via :root[data-selection-style] from applyTheme. */
  :global(:root[data-selection-style="left-bar"]) .row.sel {
    border-left-color: var(--color-accent);
    background: color-mix(in srgb, var(--color-accent) 12%, transparent);
  }
  :global(:root[data-selection-style="fill"]) .row.sel {
    background: color-mix(in srgb, var(--color-accent) 22%, transparent);
  }
  .row.sel .label {
    color: var(--color-accent);
  }
  .label {
    flex: 1;
    text-align: left;
    background: none;
    border: none;
    height: auto;
    padding: 0;
    color: var(--color-text);
    font-family: var(--font-ui);
    font-size: 11px;
    letter-spacing: var(--letter-spacing);
    text-transform: var(--label-case);
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
    cursor: pointer;
  }
  .actions {
    display: none;
    gap: 2px;
  }
  .row:hover .actions {
    display: inline-flex;
  }
  .icon {
    background: none;
    border: none;
    height: auto;
    padding: 2px 4px;
    font-size: 11px;
    line-height: 1;
    color: var(--color-muted);
  }
  .icon:hover:not(:disabled) {
    color: var(--color-accent);
  }
  .icon:disabled {
    opacity: 0.3;
    cursor: default;
  }
  .inline {
    flex: 1;
    background: var(--color-base);
    border: var(--border-weight) solid var(--color-accent);
    color: var(--color-text);
    font-family: var(--font-ui);
    font-size: 11px;
    padding: 3px 5px;
  }
  .inline:focus {
    outline: none;
  }
  .msg {
    margin: 0;
    padding: 8px 7px;
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
