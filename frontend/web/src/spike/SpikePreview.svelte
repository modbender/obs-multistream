<script lang="ts">
  let { windowId }: { windowId: number } = $props();
</script>

<div class="ph">Preview (window {windowId}) — overlay wired in Task 5</div>

<style>
  .ph {
    height: 100%;
    display: grid;
    place-items: center;
    color: #666;
    font: 12px monospace;
  }
</style>
