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

<div class="dock-body">
  <div class="dock-toolbar">
    <button class="dock-add" title="Add scene" onclick={beginAdd}>＋</button>
  </div>

  {#if defaultCanvas.error}
    <p class="dock-msg err">{defaultCanvas.error}</p>
  {:else if !defaultCanvas.loaded}
    <p class="dock-msg">Loading…</p>
  {:else}
    <ul class="dock-list">
      {#each defaultCanvas.scenes as scene (scene.name)}
        <li class="dock-row" class:sel={scene.current}>
          {#if renamingFrom === scene.name}
            <input
              class="inline"
              bind:value={renameTo}
              onkeydown={onRenameKey}
              onblur={commitRename}
              use:focusOnMount
            />
          {:else}
            <button class="dock-label" ondblclick={() => beginRename(scene.name)} onclick={() => setCurrent(scene.name)}>
              {scene.name}
            </button>
            <span class="dock-actions">
              <button class="dock-icon" title="Rename" onclick={() => beginRename(scene.name)}>✎</button>
              <button
                class="dock-icon"
                title="Remove"
                disabled={defaultCanvas.scenes.length <= 1}
                onclick={() => void remove(scene.name)}>🗑</button
              >
            </span>
          {/if}
        </li>
      {/each}

      {#if adding}
        <li class="dock-row">
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
      <p class="dock-msg">No scenes</p>
    {/if}
  {/if}

  {#if actionError}
    <p class="dock-msg err">{actionError}</p>
  {/if}
</div>

<style>
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
</style>
