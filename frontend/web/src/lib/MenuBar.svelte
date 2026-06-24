<script lang="ts">
  import Menu, { type MenuItem } from "./Menu.svelte";
  import { DOCKS } from "./dock/dockRegistry";
  import { openSettings } from "./settingsOpener.svelte";
  import { openThemeEditor } from "./themeEditorOpener.svelte";
  import { defaultSelection, openTransform } from "./transformOpener.svelte";

  // App passes the dock-visibility map + the toggle / reset / lock actions so the
  // Docks menu drives the live layout. visibleDocks[id] === false => hidden.
  let {
    visibleDocks,
    toggleDock,
    resetLayout,
    locked,
    toggleLock,
  }: {
    visibleDocks: Record<string, boolean>;
    toggleDock: (id: string) => void;
    resetLayout: () => void;
    locked: boolean;
    toggleLock: () => void;
  } = $props();

  // §3.5 LOCKED contents. Actions not in P1 scope are present but disabled so the
  // menu shape matches the spec exactly; later phases enable them.
  const fileItems: (MenuItem | null)[] = $derived([
    { label: "Settings", action: () => openSettings("video") },
    { label: "Recordings", disabled: true },
    null,
    { label: "Exit", action: () => window.close() },
  ]);
  // Transform enables for the current default-canvas (channel-0) selection, tracked
  // by transformOpener via the global sceneItem.selected push.
  const editItems: (MenuItem | null)[] = $derived([
    { label: "Undo", disabled: true },
    { label: "Redo", disabled: true },
    null,
    {
      label: "Transform",
      disabled: defaultSelection.id == null,
      action: () =>
        defaultSelection.id != null &&
        openTransform({ scene: defaultSelection.scene ?? undefined, id: defaultSelection.id }, "Selected Source"),
    },
  ]);
  const viewItems: (MenuItem | null)[] = [
    { label: "Studio Mode", disabled: true },
    { label: "Stats", disabled: true },
    { label: "Fullscreen Preview", disabled: true },
  ];
  // Docks menu: one toggle per dock (checked = visible) + Reset + Lock + the theme
  // editor (preset switching + full token editing live in the editor now).
  const dockItems: (MenuItem | null)[] = $derived([
    ...DOCKS.map((d) => ({
      label: d.title,
      checked: visibleDocks[d.id] !== false,
      action: () => toggleDock(d.id),
    })),
    null,
    { label: "Reset Layout", action: resetLayout },
    { label: "Lock Docks", checked: locked, action: toggleLock },
    null,
    { label: "Theme Editor…", action: () => openThemeEditor() },
  ]);
  const helpItems: (MenuItem | null)[] = [{ label: "About OBS MultiStreamer", disabled: true }];
</script>

<div class="menubar">
  <Menu label="File" items={fileItems} />
  <Menu label="Edit" items={editItems} />
  <Menu label="View" items={viewItems} />
  <Menu label="Docks ▾" items={dockItems} />
  <Menu label="Help" items={helpItems} />
</div>

<style>
  .menubar {
    display: flex;
    gap: 4px;
    align-items: center;
    background: var(--color-surface);
    border-bottom: var(--border-weight) solid var(--color-border);
    padding: 2px 6px;
    flex: 0 0 auto;
  }
</style>
