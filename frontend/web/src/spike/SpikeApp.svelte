<script lang="ts">
  import { onMount, mount, unmount } from "svelte";
  import { DockviewComponent, type DockviewApi } from "dockview-core";
  import { obs } from "../lib/bridge";
  import SpikePreview from "./SpikePreview.svelte";
  import SpikeScenes from "./SpikeScenes.svelte";

  let { windowId, dockId }: { windowId: number; dockId: string | null } = $props();

  let host: HTMLDivElement;
  let api: DockviewApi | undefined;

  // Dockview hands us a DOM element per panel; we mount a Svelte component into it
  // and unmount on dispose. The panel's `params` carry the dock id + windowId.
  const components = {
    preview: (_options: unknown) => {
      const el = document.createElement("div");
      el.style.height = "100%";
      const cmp = mount(SpikePreview, { target: el, props: { windowId } });
      return {
        element: el,
        init() {},
        dispose() {
          void unmount(cmp);
        },
      };
    },
    scenes: (_options: unknown) => {
      const el = document.createElement("div");
      el.style.height = "100%";
      const cmp = mount(SpikeScenes, { target: el, props: {} });
      return {
        element: el,
        init() {},
        dispose() {
          void unmount(cmp);
        },
      };
    },
  };

  onMount(() => {
    const dv = new DockviewComponent(host, {
      createComponent: (o) => components[o.name as "preview" | "scenes"](o),
    });
    api = dv.api;

    if (dockId === null) {
      // Main window: show both docks with tear-out headers.
      api.addPanel({ id: "preview", component: "preview", title: "Preview · Default" });
      api.addPanel({ id: "scenes", component: "scenes", title: "Scenes", position: { direction: "below" } });
    } else {
      // Detached window: show only the dock it owns.
      const comp = dockId.startsWith("preview") ? "preview" : "scenes";
      api.addPanel({ id: dockId, component: comp, title: dockId });
    }

    // Two-way sync proof: log every scenes.changed in THIS window.
    const off = obs.on("scenes.changed", () => {
      console.log("OBSSPIKE: scenes.changed observed in window=" + windowId);
    });
    const offOpened = obs.on("window.opened", (p) => console.log("OBSSPIKE: window.opened " + JSON.stringify(p)));
    const offClosed = obs.on("window.closed", (p) => console.log("OBSSPIKE: window.closed " + JSON.stringify(p)));

    return () => {
      off();
      offOpened();
      offClosed();
      dv.dispose();
    };
  });

  async function detach(dock: string) {
    const r = await obs.call("window.detach", { dock });
    console.log("OBSSPIKE: detach -> " + JSON.stringify(r));
    // Remove the panel from this (main) window once the host window owns it.
    api?.getPanel(dock)?.api.close();
  }
</script>

<div class="spike-root">
  {#if dockId === null}
    <div class="toolbar">
      <button onclick={() => detach("preview")}>⧉ Detach Preview</button>
      <button onclick={() => detach("scenes")}>⧉ Detach Scenes</button>
    </div>
  {/if}
  <div class="dock-host" bind:this={host}></div>
</div>

<style>
  .spike-root {
    display: flex;
    flex-direction: column;
    height: 100%;
  }
  .toolbar {
    display: flex;
    gap: 8px;
    padding: 6px;
    background: #141414;
  }
  button {
    background: #e0a23c;
    color: #0a0a0a;
    border: 0;
    padding: 4px 10px;
    cursor: pointer;
  }
  .dock-host {
    flex: 1;
    min-height: 0;
  }
  :global(.dv-react-part),
  :global(.dv-tab) {
    border-radius: 0 !important;
  }
</style>
