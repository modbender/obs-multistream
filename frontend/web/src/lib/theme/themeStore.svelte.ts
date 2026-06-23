import { obs } from "../bridge";
import { applyTheme } from "./tokens";
import { presetById, DEFAULT_PRESET_ID } from "./presets";

// Runes store for the active theme preset. setPreset() repaints live (rewrites
// CSS vars) and persists the choice via the bridge. hydrate() loads the saved
// preset on boot. Persistence failures are non-fatal (UI still themes).
class ThemeStore {
  activeId = $state(DEFAULT_PRESET_ID);

  apply(id: string): void {
    this.activeId = id;
    applyTheme(presetById(id).tokens);
  }

  async setPreset(id: string): Promise<void> {
    this.apply(id);
    try {
      await obs.call("theme.save", { activePreset: id });
    } catch {
      // non-fatal: theming already applied in-memory
    }
  }

  async hydrate(): Promise<void> {
    let id = DEFAULT_PRESET_ID;
    try {
      const saved = await obs.call("theme.load");
      if (saved && typeof saved.activePreset === "string") {
        id = saved.activePreset;
      }
    } catch {
      // no saved theme yet -> default
    }
    this.apply(id);
  }
}

export const themeStore = new ThemeStore();
