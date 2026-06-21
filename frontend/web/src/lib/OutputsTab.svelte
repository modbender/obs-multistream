<script lang="ts">
  import {
    obs,
    type OutputBindingInfo,
    type StreamProfileInfo,
    type CanvasInfo,
    type OutputBindingCreateParams,
  } from "./bridge";

  let bindings = $state<OutputBindingInfo[]>([]);
  let profiles = $state<StreamProfileInfo[]>([]);
  let canvases = $state<CanvasInfo[]>([]);
  let loaded = $state(false);
  let error = $state<string | null>(null);

  // Inline add form: pick a profile + a canvas, then create.
  let formOpen = $state(false);
  let fProfile = $state(""); // "" => unset (a canvas with no destination yet)
  let fCanvas = $state("");
  let saving = $state(false);
  let formError = $state<string | null>(null);

  async function loadBindings() {
    bindings = await obs.call("outputBinding.list");
  }

  async function loadAll() {
    error = null;
    try {
      const [b, p, c] = await Promise.all([
        obs.call("outputBinding.list"),
        obs.call("streamProfile.list"),
        obs.call("canvas.list"),
      ]);
      bindings = b;
      profiles = p;
      canvases = c;
    } catch (e) {
      error = (e as Error).message;
    } finally {
      loaded = true;
    }
  }

  async function loadLists() {
    // Profile/canvas labels feed the joins + the add dropdowns, so refresh them
    // when either underlying list mutates.
    try {
      const [p, c] = await Promise.all([obs.call("streamProfile.list"), obs.call("canvas.list")]);
      profiles = p;
      canvases = c;
      // Names may have changed; re-pull bindings so the joined labels follow.
      await loadBindings();
    } catch (e) {
      error = (e as Error).message;
    }
  }

  $effect(() => {
    void loadAll();
    const offBindings = obs.on("outputBinding.changed", () => void loadBindings());
    const offProfiles = obs.on("streamProfile.changed", () => void loadLists());
    const offCanvases = obs.on("canvas.changed", () => void loadLists());
    return () => {
      offBindings();
      offProfiles();
      offCanvases();
    };
  });

  function openAdd() {
    fProfile = "";
    fCanvas = canvases[0]?.uuid ?? "";
    formError = null;
    formOpen = true;
  }

  function closeForm() {
    formOpen = false;
  }

  const formValid = $derived(fCanvas.length > 0);

  async function create() {
    if (!formValid || saving) return;
    saving = true;
    formError = null;
    try {
      const params: OutputBindingCreateParams = { canvasUuid: fCanvas };
      if (fProfile) params.profileUuid = fProfile;
      await obs.call("outputBinding.create", params);
      await loadBindings();
      formOpen = false;
    } catch (e) {
      formError = (e as Error).message;
    } finally {
      saving = false;
    }
  }

  async function toggle(b: OutputBindingInfo, enabled: boolean) {
    try {
      await obs.call("outputBinding.setEnabled", { uuid: b.uuid, enabled });
      await loadBindings();
    } catch (e) {
      error = (e as Error).message;
    }
  }

  async function remove(b: OutputBindingInfo) {
    try {
      await obs.call("outputBinding.remove", { uuid: b.uuid });
      await loadBindings();
    } catch (e) {
      error = (e as Error).message;
    }
  }

  function isDangling(label: string): boolean {
    return label === "(deleted)";
  }
  function isUnset(label: string): boolean {
    return label === "(unset)";
  }
</script>

<div class="outputs">
  {#if error}<p class="error">{error}</p>{/if}

  {#if !loaded}
    <p class="dim">Loading output bindings…</p>
  {:else}
    {#if bindings.length === 0}
      <p class="dim">No output bindings yet. Bind a stream profile to a canvas to start streaming it.</p>
    {:else}
      <ul class="list">
        {#each bindings as b (b.uuid)}
          <li class="row">
            <div class="info">
              <span class="pair">
                <span
                  class="name"
                  class:deleted={isDangling(b.profileLabel)}
                  class:unset={isUnset(b.profileLabel)}>{b.profileLabel}</span
                >
                <span class="arrow">→</span>
                <span
                  class="name"
                  class:deleted={isDangling(b.canvasName)}
                  class:unset={isUnset(b.canvasName)}>{b.canvasName}</span
                >
              </span>
            </div>
            <div class="rowactions">
              <label class="toggle" title={b.enabled ? "Enabled" : "Disabled"}>
                <input
                  type="checkbox"
                  checked={b.enabled}
                  onchange={(e) => void toggle(b, (e.currentTarget as HTMLInputElement).checked)}
                />
                <span class="track"><span class="thumb"></span></span>
              </label>
              <button class="mini danger" title="Remove" onclick={() => void remove(b)}>🗑</button>
            </div>
          </li>
        {/each}
      </ul>
    {/if}

    <div class="addbar">
      <button class="btn" onclick={openAdd}>+ Add Output Binding</button>
    </div>
  {/if}

  {#if formOpen}
    <div class="form">
      <h4>New Output Binding</h4>

      <div class="field">
        <span class="flabel">Stream Profile</span>
        <select bind:value={fProfile}>
          <option value="">(unset — no destination)</option>
          {#each profiles as p (p.uuid)}
            <option value={p.uuid}>{p.label || p.platform}</option>
          {/each}
        </select>
      </div>

      <div class="field">
        <span class="flabel">Canvas</span>
        <select bind:value={fCanvas}>
          {#each canvases as c (c.uuid)}
            <option value={c.uuid}>{c.name}</option>
          {/each}
        </select>
      </div>

      {#if formError}<p class="error">{formError}</p>{/if}

      <div class="actions">
        <button class="btn ghost" onclick={closeForm}>Close</button>
        <button class="btn primary" disabled={!formValid || saving} onclick={() => void create()}>
          {saving ? "Creating…" : "Create"}
        </button>
      </div>
    </div>
  {/if}
</div>

<style>
  .outputs {
    padding: 8px 0 4px;
  }
  .list {
    list-style: none;
    margin: 0;
    padding: 0;
    display: flex;
    flex-direction: column;
    gap: 4px;
  }
  .row {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 10px;
    padding: 8px 10px;
    border: 1px solid var(--border);
    border-radius: 8px;
    background: var(--bg-sunken);
  }
  .info {
    min-width: 0;
  }
  .pair {
    display: flex;
    align-items: center;
    gap: 8px;
    font-size: 13px;
  }
  .name {
    color: var(--text);
    font-weight: 500;
  }
  .name.deleted {
    color: var(--off, #d65a5a);
    font-style: italic;
  }
  .name.unset {
    color: var(--text-dim);
    font-style: italic;
    font-weight: 400;
  }
  .arrow {
    color: var(--text-dim);
  }
  .rowactions {
    display: flex;
    align-items: center;
    gap: 8px;
    flex-shrink: 0;
  }
  .toggle {
    display: inline-flex;
    align-items: center;
    cursor: pointer;
  }
  .toggle input {
    position: absolute;
    opacity: 0;
    width: 0;
    height: 0;
  }
  .track {
    display: inline-block;
    width: 34px;
    height: 18px;
    border-radius: 999px;
    background: var(--bg-raised);
    border: 1px solid var(--border);
    position: relative;
    transition: background 0.12s ease;
  }
  .thumb {
    position: absolute;
    top: 1px;
    left: 1px;
    width: 14px;
    height: 14px;
    border-radius: 50%;
    background: var(--text-dim);
    transition:
      transform 0.12s ease,
      background 0.12s ease;
  }
  .toggle input:checked + .track {
    background: var(--accent);
    border-color: var(--accent);
  }
  .toggle input:checked + .track .thumb {
    transform: translateX(16px);
    background: #fff;
  }
  .mini {
    background: none;
    border: 1px solid var(--border);
    border-radius: 6px;
    color: var(--text-soft);
    cursor: pointer;
    font: inherit;
    font-size: 12px;
    padding: 4px 7px;
    line-height: 1;
  }
  .mini:hover:not(:disabled) {
    color: var(--text);
    background: var(--bg-raised);
  }
  .mini.danger:hover:not(:disabled) {
    color: var(--off, #d65a5a);
    border-color: var(--off, #d65a5a);
  }
  .addbar {
    margin-top: 12px;
  }
  .form {
    margin-top: 14px;
    padding: 14px;
    border: 1px solid var(--border);
    border-radius: 10px;
    background: var(--bg-raised);
  }
  .form h4 {
    margin: 0 0 12px;
    font-size: 12px;
    text-transform: uppercase;
    letter-spacing: 0.06em;
    color: var(--text-dim);
  }
  .field {
    margin-bottom: 12px;
  }
  .flabel {
    display: block;
    font-size: 12px;
    color: var(--text-soft);
    margin-bottom: 6px;
  }
  select {
    background: var(--bg-sunken);
    border: 1px solid var(--border);
    border-radius: 6px;
    padding: 7px 10px;
    color: var(--text);
    font: inherit;
    width: 100%;
  }
  select:focus {
    outline: none;
    border-color: var(--accent);
  }
  .actions {
    display: flex;
    justify-content: flex-end;
    gap: 8px;
    margin-top: 4px;
  }
  .btn {
    border-radius: 6px;
    padding: 7px 14px;
    font: inherit;
    cursor: pointer;
    border: 1px solid var(--border);
    background: var(--bg-sunken);
    color: var(--text-soft);
  }
  .btn:hover:not(:disabled) {
    color: var(--text);
  }
  .btn.primary {
    background: var(--accent);
    border-color: var(--accent);
    color: #fff;
  }
  .btn.primary:disabled {
    opacity: 0.45;
    cursor: default;
  }
  .btn.ghost {
    background: none;
  }
  .dim {
    color: var(--text-dim);
    margin: 0;
  }
  .error {
    color: var(--off, #d65a5a);
    margin: 0 0 8px;
    font-size: 12px;
  }
</style>
