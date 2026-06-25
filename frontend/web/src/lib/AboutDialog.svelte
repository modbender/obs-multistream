<script lang="ts">
  import { obs } from "./bridge";
  import { suspendPreview } from "./previewGate.svelte";

  interface Props {
    onClose: () => void;
  }
  let { onClose }: Props = $props();

  // Overlaps the native preview overlay (a sibling HWND above CEF); suspend it
  // while open so the overlay doesn't occlude the modal (same as TransformDialog).
  $effect(() => suspendPreview());

  // libobs version, loaded on open; "…" until resolved.
  let version = $state("…");

  $effect(() => {
    obs
      .call("getVersion")
      .then((v) => (version = v || "unknown"))
      .catch((e) => (version = "error: " + (e as Error).message));
  });

  function onKeydown(e: KeyboardEvent) {
    if (e.key === "Escape") {
      onClose();
    }
  }
</script>

<svelte:window onkeydown={onKeydown} />

<div
  class="modal-backdrop"
  role="presentation"
  onclick={(e) => {
    if (e.target === e.currentTarget) onClose();
  }}
>
  <div class="modal" role="dialog" aria-modal="true" aria-label="About OBS MultiStream">
    <header class="modal-head">
      <h3>About</h3>
      <button class="icon close" title="Close" onclick={onClose}>✕</button>
    </header>

    <div class="modal-body">
      <div class="wordmark">OBS MultiStream</div>
      <p class="tagline">Native multi-destination streaming, built on OBS Studio.</p>

      <dl class="facts">
        <div class="fact">
          <dt>Engine</dt>
          <dd>libobs {version}</dd>
        </div>
        <div class="fact">
          <dt>License</dt>
          <dd>Licensed under GPLv2+.</dd>
        </div>
        <div class="fact">
          <dt>Upstream</dt>
          <dd>A fork of OBS Studio (obsproject.com).</dd>
        </div>
      </dl>
    </div>

    <footer class="modal-foot">
      <button class="btn ghost" onclick={onClose}>Close</button>
    </footer>
  </div>
</div>

<style>
  .modal-backdrop {
    position: fixed;
    inset: 0;
    background: rgba(0, 0, 0, 0.55);
    display: flex;
    align-items: center;
    justify-content: center;
    z-index: 100;
    padding: 24px;
  }
  .modal {
    background: var(--color-surface);
    border: var(--border-weight) solid var(--color-border);
    width: min(380px, 100%);
    max-height: 86vh;
    display: flex;
    flex-direction: column;
    box-shadow: 0 18px 48px rgba(0, 0, 0, 0.5);
    font-family: var(--font-ui);
  }
  .modal-head {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 8px 11px;
    background: var(--color-surface);
    border-bottom: var(--border-weight) solid var(--color-border);
  }
  .modal-head h3 {
    margin: 0;
    font-size: 11px;
    letter-spacing: var(--letter-spacing);
    text-transform: var(--label-case);
    color: var(--color-text);
    font-weight: 600;
  }
  .modal-body {
    padding: 16px 14px;
    overflow: auto;
  }
  .modal-foot {
    display: flex;
    justify-content: flex-end;
    padding: 8px 11px;
    border-top: var(--border-weight) solid var(--color-border);
  }

  .wordmark {
    font-size: 18px;
    font-weight: 700;
    letter-spacing: var(--letter-spacing);
    color: var(--color-text);
  }
  .tagline {
    margin: 4px 0 16px;
    font-size: 11px;
    color: var(--color-muted);
  }

  .facts {
    margin: 0;
    display: flex;
    flex-direction: column;
    gap: 8px;
  }
  .fact {
    display: flex;
    flex-direction: column;
    gap: 2px;
    border: var(--border-weight) solid var(--color-border);
    background: var(--color-base);
    padding: 7px 8px;
  }
  .fact dt {
    font-size: 9px;
    letter-spacing: var(--letter-spacing);
    text-transform: uppercase;
    color: var(--color-accent);
  }
  .fact dd {
    margin: 0;
    font-size: 11px;
    color: var(--color-text);
  }

  .btn {
    height: auto;
    padding: 5px 10px;
    font-family: var(--font-ui);
    font-size: 11px;
    border: var(--border-weight) solid var(--color-border);
    background: transparent;
    color: var(--color-text);
    letter-spacing: var(--letter-spacing);
    text-transform: var(--label-case);
  }
  .btn:hover:not(:disabled) {
    border-color: var(--color-accent);
    color: var(--color-accent);
  }
  .btn.ghost {
    background: none;
  }

  .icon {
    background: none;
    border: none;
    color: var(--color-muted);
    cursor: pointer;
    padding: 2px 4px;
    font-size: 13px;
    line-height: 1;
    height: auto;
  }
  .icon:hover {
    color: var(--color-text);
  }
</style>
