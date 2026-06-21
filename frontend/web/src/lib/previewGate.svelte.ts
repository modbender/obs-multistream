// Ref-counted suspension of the native preview overlay.
//
// The obs preview is a native child HWND z-ordered ABOVE the CEF browser, so any
// DOM surface that overlaps the preview region (modals, dialogs) would be hidden
// behind it -- DOM can't paint over a native window. Components that render over
// the preview suspend it while open; PreviewArea hides the overlay whenever the
// count is non-zero and re-asserts its rect when it returns to zero.

let count = $state(0);

/** Suspend the preview while a modal/overlay is open. Returns a release fn. */
export function suspendPreview(): () => void {
	count++;
	let released = false;
	return () => {
		if (!released) {
			released = true;
			count = Math.max(0, count - 1);
		}
	};
}

/** Reactive: true while any caller holds a suspension. */
export function previewSuspended(): boolean {
	return count > 0;
}
