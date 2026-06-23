<script lang="ts">
  import { onMount } from "svelte";
  import { obs } from "../bridge";
  import { previewSuspended } from "../previewGate.svelte";

  let {}: Record<string, unknown> = $props();

  // The native overlay (a sibling HWND above CEF) covers this exact element; it
  // stays transparent so the overlay paints through. Global channel-0 path: the
  // Default surface is addressed by OMITTING the canvas param (P3 side docks pass
  // their own uuid, keeping this surface distinct).
  let previewEl = $state<HTMLElement | undefined>();

  function reportRect() {
    if (!previewEl) {
      return;
    }
    const r = previewEl.getBoundingClientRect();
    obs
      .call("preview.setRect", {
        x: r.left,
        y: r.top,
        w: r.width,
        h: r.height,
        dpr: window.devicePixelRatio || 1,
      })
      .catch((e) => console.log("preview.setRect failed: " + (e as Error).message));
  }

  function hidePreview() {
    obs.call("preview.hide", {}).catch(() => {});
  }

  function destroyPreview() {
    obs.call("preview.destroy", {}).catch(() => {});
  }

  onMount(() => {
    reportRect();
    const ro = new ResizeObserver(reportRect);
    if (previewEl) {
      ro.observe(previewEl);
    }
    window.addEventListener("resize", reportRect);
    window.addEventListener("scroll", reportRect, true);
    return () => {
      ro.disconnect();
      window.removeEventListener("resize", reportRect);
      window.removeEventListener("scroll", reportRect, true);
      destroyPreview();
    };
  });

  // The global previewGate suspends every native overlay while a modal is open;
  // hide our surface and re-assert its rect when cleared.
  $effect(() => {
    if (previewSuspended()) {
      hidePreview();
    } else {
      reportRect();
    }
  });
</script>

<section class="preview" bind:this={previewEl}>
  <span class="label">Default</span>
</section>

<style>
  .preview {
    height: 100%;
    width: 100%;
    position: relative;
    /* Transparent: the native overlay HWND paints this region. */
    background: transparent;
    overflow: hidden;
  }
  .label {
    position: absolute;
    top: 8px;
    left: 10px;
    font-family: var(--font-ui);
    font-size: 9px;
    letter-spacing: var(--letter-spacing);
    text-transform: var(--label-case);
    color: var(--color-muted);
    pointer-events: none;
  }
</style>
