<script lang="ts">
  // Placeholder shell — audio sources/levels not yet wired. Positioned in the
  // bottom dock row so it can be filled in later without a layout change.
</script>

<section class="panel">
  <header>
    <h2>Audio Mixer</h2>
  </header>
  <p class="dim">Not yet wired</p>
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
  .dim {
    color: var(--text-dim);
    margin: 0;
    font-size: 12px;
  }
</style>
