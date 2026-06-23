<script lang="ts">
  // Lightweight stub for docks wired in later phases. P1 only needs each dock to
  // EXIST as a real Dockview panel with its themed header; contents are real in
  // P2-P4. `label` is the human dock name; `phase` notes when it gets wired.
  let { label = "", phase = "" }: { label?: string; phase?: string } = $props();
</script>

<div class="placeholder">
  <span class="name">{label}</span>
  {#if phase}<span class="phase">wired in {phase}</span>{/if}
</div>

<style>
  .placeholder {
    height: 100%;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    gap: 4px;
    background: var(--color-surface);
    color: var(--color-muted);
    font-family: var(--font-ui);
  }
  .name {
    font-size: 11px;
    letter-spacing: var(--letter-spacing);
    text-transform: var(--label-case);
    color: var(--color-text);
  }
  .phase {
    font-size: 9px;
    letter-spacing: 0.05em;
    text-transform: uppercase;
  }
</style>
