<script lang="ts">
  import { onMount } from "svelte";
  import { obs } from "../lib/bridge";

  let { windowId }: { windowId: number } = $props();
  let host: HTMLDivElement;

  // Report the panel's rect (CSS px + this window's devicePixelRatio) so the native
  // overlay for (windowId, Default canvas) lands exactly over it. The host resolves
  // `window` to the originating top-level window, parenting the overlay there.
  function report() {
    if (!host) return;
    const r = host.getBoundingClientRect();
    void obs.call("preview.setRect", {
      window: windowId,
      x: r.left,
      y: r.top,
      w: r.width,
      h: r.height,
      dpr: window.devicePixelRatio || 1,
    });
  }

  onMount(() => {
    report();
    const ro = new ResizeObserver(report);
    ro.observe(host);
    window.addEventListener("resize", report);
    return () => {
      ro.disconnect();
      window.removeEventListener("resize", report);
      void obs.call("preview.hide", { window: windowId });
    };
  });
</script>

<div class="overlay-region" bind:this={host}></div>

<style>
  .overlay-region {
    height: 100%;
    width: 100%;
    background: #000;
  }
</style>
