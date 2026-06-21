<script lang="ts">
  import type { ControlProps } from "./controls";
  let { prop, value, onChange }: ControlProps = $props();
  const checked = $derived(Boolean(value));
</script>

<label class="bool" title={prop.long_description ?? ""}>
  <input
    type="checkbox"
    {checked}
    disabled={!prop.enabled}
    onchange={(e) => onChange(prop.name, (e.currentTarget as HTMLInputElement).checked)}
  />
  <span>{prop.label ?? prop.name}</span>
</label>

<style>
  .bool {
    display: flex;
    align-items: center;
    gap: 8px;
    cursor: pointer;
    grid-column: 1 / -1;
  }
  input:disabled + span {
    opacity: 0.5;
  }
</style>
