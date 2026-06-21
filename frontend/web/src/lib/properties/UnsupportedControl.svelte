<script lang="ts">
  import type { ControlProps } from "./controls";
  let { prop }: ControlProps = $props();
  // font / editable_list / frame_rate: serialized best-effort by the backend but
  // not yet editable here. Show a clear placeholder rather than a broken control.
</script>

<div class="unsupported" title={prop.long_description ?? ""}>
  <span class="badge">{prop.type}</span>
  <span class="msg">Editing not yet supported (TODO)</span>
</div>

<style>
  .unsupported {
    display: flex;
    align-items: center;
    gap: 8px;
    font-size: 12px;
    color: var(--text-dim);
  }
  .badge {
    font-family: var(--mono, ui-monospace, monospace);
    background: var(--bg-sunken);
    border: 1px solid var(--border);
    border-radius: 4px;
    padding: 1px 6px;
  }
</style>
