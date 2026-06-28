// Shared navigation state for the Settings page (7.3). The Settings UI is a full
// nav-rail page, not a modal: openSettings(tab) navigates the page router to it
// with a tab selected. SettingsPage reads settingsNav; the sub-nav buttons call
// setSettingsTab. (Canvases moved out to its own nav-rail page.)

import { setPage } from "./pageStore.svelte";

export type SettingsTab = "general" | "audio" | "hotkeys" | "browserDocks" | "appearance" | "advanced";

export const settingsNav = $state<{ tab: SettingsTab }>({
  tab: "general",
});

/** Switch sub-nav tab from within the Settings page. */
export function setSettingsTab(tab: SettingsTab): void {
  settingsNav.tab = tab;
}

/** Navigate to the Settings page on a tab. */
export function openSettings(tab: SettingsTab = "general"): void {
  settingsNav.tab = tab;
  setPage("settings");
}
