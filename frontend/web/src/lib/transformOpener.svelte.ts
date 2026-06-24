// Shared opener for the numeric Edit Transform dialog. Mirrors filterDialogOpener:
// App owns the single TransformDialog mount gated on `.target`; any component (a
// source context menu, the Edit menu) calls openTransform(target, label) to request it.

import { obs, type TransformTarget } from "./bridge";

interface TransformOpenerState {
  target: TransformTarget | null;
  /** Display label for the dialog header (usually the source name). */
  label: string;
}

export const transformOpener = $state<TransformOpenerState>({
  target: null,
  label: "",
});

/** Open the Edit Transform dialog for one scene item. */
export function openTransform(target: TransformTarget, label: string): void {
  transformOpener.target = target;
  transformOpener.label = label;
}

export function closeTransform(): void {
  transformOpener.target = null;
  transformOpener.label = "";
}

// --- default-canvas selection tracking (for the MenuBar Edit > Transform item) ---
//
// There is no shared selection store, so we keep a tiny module rune fed by the
// global selection push. Only the Default surface (canvas == null) drives it; a
// non-null canvas belongs to a per-canvas dock and is ignored here. The id clears
// when a mutation deselects (sceneItem.selected with id == null) so the menu item
// disables again when nothing is selected.

interface DefaultSelectionState {
  scene: string | null;
  id: number | null;
}

export const defaultSelection = $state<DefaultSelectionState>({ scene: null, id: null });

let started = false;

/** Idempotently subscribe to the global selection push. Call once from the shell. */
export function startDefaultSelectionTracking(): void {
  if (started) {
    return;
  }
  started = true;
  obs.on("sceneItem.selected", (p) => {
    if (p.canvas != null) {
      return;
    }
    defaultSelection.scene = p.scene;
    defaultSelection.id = p.id;
  });
}
