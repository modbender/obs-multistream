<script lang="ts">
  import type { ControlProps } from "./controls";
  import type { PathProperty } from "../bridge";
  let { prop, value, onChange }: ControlProps = $props();

  const p = $derived(prop as PathProperty);
  const str = $derived(value == null ? "" : String(value));
  // OS file dialog is a later bridge method (4.3.x). For now this is an editable
  // text input; the Browse affordance is a no-op placeholder until then.
</script>

<div class="path" title={prop.long_description ?? ""}>
  <input
    type="text"
    value={str}
    disabled={!prop.enabled}
    placeholder={p.path_type === "directory" ? "directory path" : "file path"}
    oninput={(e) => onChange(prop.name, (e.currentTarget as HTMLInputElement).value)}
  />
  <button type="button" class="browse" disabled title="OS file dialog: TODO (later bridge method)">
    Browse…
  </button>
</div>

<style>
  .path {
    display: flex;
    gap: 6px;
  }
  input {
    flex: 1;
    min-width: 0;
  }
  .browse {
    white-space: nowrap;
  }
</style>
