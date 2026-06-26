<script lang="ts">
  import { themeStore } from "./theme/themeStore.svelte";
  import { ACCENT_VALUES } from "./theme/presets";
  import type { AccentName, ThemeMode, ThemeTokens } from "./theme/tokens";
  import Segmented, { type SegmentedOption } from "./Segmented.svelte";
  import { openThemeEditor } from "./themeEditorOpener.svelte";

  // Decision C: the mock's accent swatches + a live preview, PLUS the existing axes
  // (mode/density) and presets, PLUS a reachable full per-token Theme Editor. Every
  // change is a live tweak on the global theme store (applies + persists immediately).

  // Accent swatches in the mock's order; the color is the source-of-truth ACCENT_VALUES.
  const ACCENTS: AccentName[] = ["amber", "blue", "violet", "emerald", "rose"];

  const MODE_OPTS: SegmentedOption[] = [
    { label: "Dark", value: "dark" },
    { label: "Light", value: "light" },
  ];
  const DENSITY_OPTS: SegmentedOption[] = [
    { label: "Comfortable", value: "comfortable" },
    { label: "Compact", value: "compact" },
  ];
</script>

<div class="appearance">
  <h2 class="title">Appearance</h2>

  <section class="block">
    <div class="block-label">Accent</div>
    <div class="swatches">
      {#each ACCENTS as a (a)}
        <button
          class="swatch"
          class:on={themeStore.tokens.accent === a}
          title={a}
          aria-label={a}
          aria-pressed={themeStore.tokens.accent === a}
          style:background={ACCENT_VALUES[a].accent}
          onclick={() => themeStore.setToken("accent", a)}
        ></button>
      {/each}
    </div>
    <p class="note">
      Accent, mode, and density are live tweaks applied to the whole interface. Light mode is supported for venues that
      need it.
    </p>
  </section>

  <section class="block row2">
    <div class="axis">
      <div class="block-label">Mode</div>
      <Segmented
        options={MODE_OPTS}
        value={themeStore.tokens.mode}
        onChange={(v) => themeStore.setToken("mode", v as ThemeMode)}
      />
    </div>
    <div class="axis">
      <div class="block-label">Density</div>
      <Segmented
        options={DENSITY_OPTS}
        value={themeStore.tokens.density}
        onChange={(v) => themeStore.setToken("density", v as ThemeTokens["density"])}
      />
    </div>
  </section>

  <section class="block">
    <div class="block-label">Preset</div>
    <div class="presets">
      {#each themeStore.allThemes as t (t.id)}
        <button class="chip" class:on={themeStore.activeId === t.id} onclick={() => themeStore.selectPreset(t.id)}
          >{t.name}</button
        >
      {/each}
    </div>
    <p class="note">
      Presets set every token at once. For full control over individual colors, fonts, and element styles, open the Theme
      Editor.
    </p>
    <button class="editor-btn" onclick={() => openThemeEditor()}>Open Theme Editor…</button>
  </section>

  <div class="preview">
    <div class="preview-label">PREVIEW</div>
    <div class="preview-row">
      <button class="go-live">● GO LIVE</button>
      <span class="status ok">● LIVE 01:24:07</span>
      <span class="status warn">● CONNECTING</span>
      <span class="status bad">● ERROR</span>
    </div>
  </div>
</div>

<style>
  .appearance {
    max-width: 680px;
  }
  .title {
    margin: 0 0 18px;
    font-size: 15px;
    font-weight: 600;
  }
  .block {
    margin-bottom: 24px;
  }
  .row2 {
    display: flex;
    gap: 28px;
    flex-wrap: wrap;
  }
  .axis {
    min-width: 200px;
  }
  .block-label {
    font-size: 12px;
    color: var(--color-dim);
    margin-bottom: 10px;
  }
  .swatches {
    display: flex;
    gap: 10px;
  }
  .swatch {
    width: 34px;
    height: 34px;
    padding: 0;
    cursor: pointer;
    border: 0;
    box-shadow: 0 0 0 1px var(--color-border);
  }
  .swatch.on {
    box-shadow: 0 0 0 2px var(--color-text);
  }
  .note {
    margin: 12px 0 0;
    font-size: 11px;
    color: var(--color-muted);
    line-height: 1.5;
  }
  .presets {
    display: flex;
    flex-wrap: wrap;
    gap: 8px;
  }
  .chip {
    font-size: 11px;
    padding: 6px 12px;
    cursor: pointer;
    color: var(--color-dim);
    background: transparent;
    border: var(--border-weight) solid var(--color-border);
  }
  .chip:hover {
    border-color: var(--color-accent);
    color: var(--color-text);
  }
  .chip.on {
    color: var(--color-accent-contrast);
    background: var(--color-accent);
    border-color: var(--color-accent);
  }
  .editor-btn {
    margin-top: 12px;
    font-size: 12px;
    padding: 8px 14px;
    cursor: pointer;
    color: var(--color-text);
    background: transparent;
    border: var(--border-weight) solid var(--color-border);
  }
  .editor-btn:hover {
    border-color: var(--color-accent);
    color: var(--color-accent);
  }
  .preview {
    border: var(--border-weight) solid var(--color-border);
    background: var(--color-surface);
    padding: 18px;
  }
  .preview-label {
    font-family: var(--font-mono);
    font-size: 10px;
    letter-spacing: 0.1em;
    color: var(--color-muted);
    margin-bottom: 14px;
  }
  .preview-row {
    display: flex;
    gap: 12px;
    align-items: center;
    flex-wrap: wrap;
  }
  .go-live {
    padding: 9px 18px;
    font-size: 12px;
    font-weight: 600;
    border: 0;
    cursor: default;
    background: var(--color-accent);
    color: var(--color-accent-contrast);
  }
  .status {
    font-family: var(--font-mono);
    font-size: 11px;
  }
  .status.ok {
    color: var(--meter-green);
  }
  .status.warn {
    color: var(--meter-yellow);
  }
  .status.bad {
    color: var(--color-live);
  }
</style>
