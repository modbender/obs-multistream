// Shared opener for the Theme Editor modal. Mirrors settingsOpener: App owns the
// single ThemeEditor mount gated on `.open`; any component (the Docks menu) calls
// openThemeEditor() to request it.

import { suspendPreview } from "./previewGate.svelte";

export const themeEditorOpener = $state<{ open: boolean }>({ open: false });

// Hold the preview suspension for the editor's lifetime; acquiring it here rather
// than in the modal's mount effect keeps the gate ref-count one-directional and
// consistent with the other openers (no transient zero on open).
let release: (() => void) | null = null;

export function openThemeEditor(): void {
  themeEditorOpener.open = true;
  release ??= suspendPreview();
}

export function closeThemeEditor(): void {
  themeEditorOpener.open = false;
  release?.();
  release = null;
}
