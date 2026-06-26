<script lang="ts">
  import CanvasesTab from "../CanvasesTab.svelte";
  import StreamsTab from "../StreamsTab.svelte";
  import OutputsTab from "../OutputsTab.svelte";
  import HotkeysTab from "../HotkeysTab.svelte";
  import AudioTab from "../AudioTab.svelte";
  import AppearanceTab from "../AppearanceTab.svelte";
  import { settingsNav, setSettingsTab, type SettingsTab } from "../settingsOpener.svelte";

  // Full Settings page: header + a 196px left sub-nav + a content pane that renders
  // the selected tab's component. AI/MCP is no longer here (it's its own nav-rail
  // page). The page model is live-apply — every tab persists through its own bridge
  // calls, so there's no OK/Apply/Cancel boundary (the modal's transactional revert
  // is intentionally dropped).
  const tabs: { id: SettingsTab; label: string }[] = [
    { id: "canvases", label: "Canvases" },
    { id: "streams", label: "Stream Profiles" },
    { id: "outputs", label: "Outputs" },
    { id: "audio", label: "Audio" },
    { id: "hotkeys", label: "Hotkeys" },
    { id: "appearance", label: "Appearance" },
  ];
</script>

<div class="page">
  <header class="head">
    <span class="title">Settings</span>
  </header>

  <div class="body">
    <nav class="subnav" aria-label="Settings categories">
      {#each tabs as t (t.id)}
        <button
          class="nav-item"
          class:active={settingsNav.tab === t.id}
          aria-current={settingsNav.tab === t.id}
          onclick={() => setSettingsTab(t.id)}>{t.label}</button
        >
      {/each}
    </nav>

    <div class="pane">
      {#if settingsNav.tab === "canvases"}
        <CanvasesTab editCanvas={settingsNav.editCanvas} />
      {:else if settingsNav.tab === "streams"}
        <StreamsTab />
      {:else if settingsNav.tab === "outputs"}
        <OutputsTab />
      {:else if settingsNav.tab === "audio"}
        <AudioTab />
      {:else if settingsNav.tab === "hotkeys"}
        <HotkeysTab />
      {:else if settingsNav.tab === "appearance"}
        <AppearanceTab />
      {/if}
    </div>
  </div>
</div>

<style>
  .page {
    height: 100%;
    display: flex;
    flex-direction: column;
    min-height: 0;
    background: var(--color-base);
    color: var(--color-text);
  }
  .head {
    flex: 0 0 auto;
    height: 58px;
    display: flex;
    align-items: center;
    gap: 12px;
    padding: 0 24px;
    border-bottom: var(--border-weight) solid var(--color-border);
    background: var(--color-surface);
  }
  .title {
    font-family: var(--font-ui);
    font-size: 16px;
    font-weight: 600;
    letter-spacing: -0.01em;
  }
  .body {
    flex: 1;
    min-height: 0;
    display: flex;
  }
  .subnav {
    flex: 0 0 196px;
    border-right: var(--border-weight) solid var(--color-border);
    background: var(--color-surface);
    padding: 10px 0;
    overflow-y: auto;
  }
  .nav-item {
    display: block;
    width: 100%;
    text-align: left;
    padding: 9px 18px;
    font-family: var(--font-ui);
    font-size: 12px;
    font-weight: 400;
    background: transparent;
    border: 0;
    border-left: 2.5px solid transparent;
    color: var(--color-dim);
    cursor: pointer;
  }
  .nav-item:hover {
    color: var(--color-text);
  }
  .nav-item.active {
    font-weight: 600;
    background: color-mix(in srgb, var(--color-accent) 10%, transparent);
    border-left-color: var(--color-accent);
    color: var(--color-accent);
  }
  .pane {
    flex: 1;
    min-width: 0;
    overflow: auto;
    padding: 24px 28px;
  }
</style>
