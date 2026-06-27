// Shared opener for the Log Viewer modal. Mirrors missingFilesOpener:
// App owns the single LogViewerDialog mount gated on `.open`; the Settings page
// (a diagnostics tool) calls openLogViewer() to request it.

import { suspendPreview } from "./previewGate.svelte";

export const logViewerOpen = $state<{ open: boolean }>({ open: false });

// Hold the preview suspension across the dialog lifetime so the native overlay
// never raises above the modal.
let release: (() => void) | null = null;

/** Open the Log Viewer modal. */
export function openLogViewer(): void {
  logViewerOpen.open = true;
  release ??= suspendPreview();
}

export function closeLogViewer(): void {
  logViewerOpen.open = false;
  release?.();
  release = null;
}
