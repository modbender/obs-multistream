<script lang="ts">
  import { onMount } from "svelte";
  import { obs } from "./bridge";
  import { previewSuspended } from "./previewGate.svelte";

  interface Props {
    /** The Default canvas uuid. The central preview reports/hides BY this uuid so
     * it is addressable distinctly from the additional-canvas side docks. */
    canvas: string;
    /** Display label for the small corner caption (canvas name). */
    label?: string;
  }
  let { canvas, label = "Default" }: Props = $props();

  // The native preview overlay (a sibling HWND above CEF) covers this exact
  // element; it stays transparent so the overlay paints through. Logic mirrors
  // CanvasPanel's per-canvas preview region, scoped to the Default uuid.
  let previewEl = $state<HTMLElement | undefined>();

  function reportRect() {
    if (!previewEl) return;
    const r = previewEl.getBoundingClientRect();
    obs
      .call("preview.setRect", {
        canvas,
        x: r.left,
        y: r.top,
        w: r.width,
        h: r.height,
        dpr: window.devicePixelRatio || 1,
      })
      .catch((e) => console.log("preview.setRect failed: " + (e as Error).message));
  }

  function hidePreview() {
    obs.call("preview.hide", { canvas }).catch(() => {});
  }

  // The central Default preview is always mounted; on unmount (app teardown) tear
  // the surface down rather than leave a stale native display lingering.
  function destroyPreview() {
    obs.call("preview.destroy", { canvas }).catch(() => {});
  }

  onMount(() => {
    reportRect();
    const ro = new ResizeObserver(reportRect);
    if (previewEl) ro.observe(previewEl);
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
  // hide our own surface and re-assert its rect when cleared.
  $effect(() => {
    if (previewSuspended()) {
      hidePreview();
    } else {
      reportRect();
    }
  });
</script>

<section class="preview-area" bind:this={previewEl}>
  <span class="label">{label}</span>
</section>

<style>
  .preview-area {
    flex: 1;
    min-height: 0;
    min-width: 0;
    position: relative;
    border: 1px solid var(--border);
    border-radius: 10px;
    /* Transparent: the native overlay HWND paints this region. */
    background: transparent;
    overflow: hidden;
  }
  .label {
    position: absolute;
    top: 8px;
    left: 10px;
    font-size: 10px;
    letter-spacing: 0.04em;
    text-transform: uppercase;
    color: var(--text-dim);
    pointer-events: none;
  }
</style>
