<script lang="ts">
  import type { ControlProps } from "./controls";
  import type { ListProperty } from "../bridge";
  let { prop, value, onChange }: ControlProps = $props();

  const p = $derived(prop as ListProperty);

  // <select> values are strings, but item values may be int/float/bool. Index by
  // position and report the item's real typed value back. Match the current
  // value to an item to seed the selection.
  const selectedIdx = $derived(p.items.findIndex((it) => it.value === value));

  function pickByIndex(idxStr: string) {
    const idx = parseInt(idxStr, 10);
    const item = p.items[idx];
    if (item) onChange(prop.name, item.value);
  }
</script>

{#if p.combo_type === "radio"}
  <div class="radio-group" title={prop.long_description ?? ""}>
    {#each p.items as item, idx (idx)}
      <label class="radio">
        <input
          type="radio"
          name={prop.name}
          checked={item.value === value}
          disabled={!prop.enabled || item.disabled}
          onchange={() => onChange(prop.name, item.value)}
        />
        <span>{item.name ?? String(item.value)}</span>
      </label>
    {/each}
  </div>
{:else if p.combo_type === "editable"}
  <!-- Editable combo: free text + datalist of suggestions (string format). -->
  <input
    type="text"
    list={`dl-${prop.name}`}
    value={value == null ? "" : String(value)}
    disabled={!prop.enabled}
    title={prop.long_description ?? ""}
    oninput={(e) => onChange(prop.name, (e.currentTarget as HTMLInputElement).value)}
  />
  <datalist id={`dl-${prop.name}`}>
    {#each p.items as item, idx (idx)}
      <option value={String(item.value)}>{item.name ?? ""}</option>
    {/each}
  </datalist>
{:else}
  <select
    disabled={!prop.enabled}
    title={prop.long_description ?? ""}
    value={String(selectedIdx)}
    onchange={(e) => pickByIndex((e.currentTarget as HTMLSelectElement).value)}
  >
    {#each p.items as item, idx (idx)}
      <option value={String(idx)} disabled={item.disabled}>{item.name ?? String(item.value)}</option>
    {/each}
  </select>
{/if}

<style>
  select,
  input {
    width: 100%;
  }
  .radio-group {
    display: flex;
    flex-direction: column;
    gap: 4px;
  }
  .radio {
    display: flex;
    align-items: center;
    gap: 8px;
    cursor: pointer;
  }
</style>
