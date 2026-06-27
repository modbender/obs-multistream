<script lang="ts">
  // A Dockview panel hosting an arbitrary web URL in a full-size sandboxed iframe
  // (Task 12). The reconciler supplies {url,title} from the store by dock id, so a
  // restored layout re-resolves its url even though the saved layout carries no
  // params. The mount adapter strips internal __* keys before they reach here.
  //
  // NOTE: some sites block framing via X-Frame-Options / CSP frame-ancestors (full
  // platform dashboards); chat/widget popout URLs embed fine. That caveat is
  // surfaced in the Browser Docks settings manager, not per-panel.
  interface Props {
    url: string;
    title: string;
  }
  let { url, title }: Props = $props();
</script>

{#if url}
  <iframe
    class="frame"
    src={url}
    {title}
    sandbox="allow-scripts allow-same-origin allow-forms allow-popups"
  ></iframe>
{:else}
  <p class="empty">No URL set for this browser dock.</p>
{/if}

<style>
  .frame {
    width: 100%;
    height: 100%;
    border: 0;
    border-radius: 0;
    display: block;
    background: var(--color-base);
  }
  .empty {
    margin: 0;
    padding: 16px;
    font-family: var(--font-ui);
    font-size: 12px;
    color: var(--color-muted);
  }
</style>
