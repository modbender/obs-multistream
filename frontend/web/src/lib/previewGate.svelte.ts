// Ref-counted suspension of the native preview overlay.
//
// The obs preview is a native child HWND z-ordered ABOVE the CEF browser, so any
// DOM surface that overlaps the preview region (modals, dialogs) would be hidden
// behind it -- DOM can't paint over a native window. Components that render over
// the preview suspend it while open; PreviewArea hides the overlay whenever the
// count is non-zero and re-asserts its rect when it returns to zero.

import { untrack } from "svelte";

let count = $state(0);

/** Suspend the preview while a modal/overlay is open. Returns a release fn. */
export function suspendPreview(): () => void {
	// Mutate inside untrack: callers invoke this from an $effect, and a plain
	// `count++` both reads and writes `count`, which would subscribe the calling
	// effect to its own write -> effect_update_depth_exceeded. Untracking the
	// read keeps the gate one-directional: writers bump the count without
	// subscribing; only previewSuspended() readers below take the dependency.
	untrack(() => count++);
	let released = false;
	return () => {
		if (!released) {
			released = true;
			untrack(() => (count = Math.max(0, count - 1)));
		}
	};
}

/** Reactive: true while any caller holds a suspension. */
export function previewSuspended(): boolean {
	return count > 0;
}
