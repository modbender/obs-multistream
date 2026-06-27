// Shared opener for the source Filters dialog. Mirrors settingsOpener/themeEditorOpener:
// App owns the single FilterDialog mount gated on `.open`; any component (a source
// context menu, an audio-mixer row) calls openFilters(source, kind) to request it.

import { suspendPreview } from "./previewGate.svelte";

export type FilterKind = "audio" | "video" | "all";

export const filterDialogOpener = $state<{ open: boolean; source: string | null; kind: FilterKind }>({
  open: false,
  source: null,
  kind: "all",
});

// Hold the preview suspension across the whole dialog lifetime. Acquiring it here
// (synchronously, at open) rather than in the modal's mount effect keeps the gate
// ref-count from transiently hitting zero during a context-menu -> modal handoff,
// which would otherwise let the native overlay re-raise above the modal.
let release: (() => void) | null = null;

/** Open the Filters dialog for one source (addressed by name). */
export function openFilters(source: string, kind: FilterKind = "all"): void {
  filterDialogOpener.source = source;
  filterDialogOpener.kind = kind;
  filterDialogOpener.open = true;
  release ??= suspendPreview();
}

export function closeFilters(): void {
  filterDialogOpener.open = false;
  filterDialogOpener.source = null;
  filterDialogOpener.kind = "all";
  release?.();
  release = null;
}
