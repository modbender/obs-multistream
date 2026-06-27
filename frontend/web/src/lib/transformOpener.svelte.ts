// Shared opener for the numeric Edit Transform dialog. Mirrors filterDialogOpener:
// App owns the single TransformDialog mount gated on `.target`; any component (a
// source context menu, the Edit menu) calls openTransform(target, label) to request it.

import { type TransformTarget } from "./bridge";
import { suspendPreview } from "./previewGate.svelte";

interface TransformOpenerState {
  target: TransformTarget | null;
  /** Display label for the dialog header (usually the source name). */
  label: string;
}

export const transformOpener = $state<TransformOpenerState>({
  target: null,
  label: "",
});

// Hold the preview suspension for the dialog's lifetime; acquiring it here (not in
// the modal's mount effect) keeps the gate ref-count from transiently hitting zero
// during a context-menu -> modal handoff, which would let the native overlay
// re-raise above the modal.
let release: (() => void) | null = null;

/** Open the Edit Transform dialog for one scene item. */
export function openTransform(target: TransformTarget, label: string): void {
  transformOpener.target = target;
  transformOpener.label = label;
  release ??= suspendPreview();
}

export function closeTransform(): void {
  transformOpener.target = null;
  transformOpener.label = "";
  release?.();
  release = null;
}
