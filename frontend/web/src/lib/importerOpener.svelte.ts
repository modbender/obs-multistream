// Shared opener for the OBS Studio importer wizard. Mirrors logViewerOpener:
// App owns the single ImporterDialog mount gated on `.open`; the Settings page
// (an import tool) calls openImporter() to request it.

import { suspendPreview } from "./previewGate.svelte";

export const importerOpen = $state<{ open: boolean }>({ open: false });

// Hold the preview suspension across the dialog lifetime so the native overlay
// never raises above the modal.
let release: (() => void) | null = null;

/** Open the OBS Studio importer wizard. */
export function openImporter(): void {
  importerOpen.open = true;
  release ??= suspendPreview();
}

export function closeImporter(): void {
  importerOpen.open = false;
  release?.();
  release = null;
}
