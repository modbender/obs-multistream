<script lang="ts">
  import { themeStore } from "./theme/themeStore.svelte";
  import { ACCENT_VALUES } from "./theme/presets";
  import type { AccentName, ThemeMode, ThemeTokens } from "./theme/tokens";
  import Segmented, { type SegmentedOption } from "./Segmented.svelte";

  // Decision C/D: the full per-token theme editor lives inline here. Quick axes
  // (accent / mode / density) sit at the top; the complete palette, typography,
  // element styles, preset save/delete, and a live preview follow below. Every
  // change is a live tweak on the global theme store (applies + persists at once).

  // --- quick axes ------------------------------------------------------------

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

  // --- data-driven axis tables (DRY) -----------------------------------------

  // One entry per color token; the short label sits beneath the swatch (mock .sw span).
  const PALETTE: { key: keyof ThemeTokens; label: string }[] = [
    { key: "colorBase", label: "base" },
    { key: "colorRail", label: "rail" },
    { key: "colorSurface", label: "surface" },
    { key: "colorSurface2", label: "surf-2" },
    { key: "colorBorder", label: "border" },
    { key: "colorBorder2", label: "bord-2" },
    { key: "colorText", label: "text" },
    { key: "colorDim", label: "dim" },
    { key: "colorMuted", label: "muted" },
    { key: "colorAccent", label: "accent" },
    { key: "colorAccentContrast", label: "accent-c" },
    { key: "colorLive", label: "live" },
    { key: "meterGreen", label: "mtr-grn" },
    { key: "meterYellow", label: "mtr-yel" },
    { key: "meterRed", label: "mtr-red" },
  ];

  // Font stacks the presets use plus a few common extras. If the live value isn't
  // listed (e.g. an imported custom), it's prepended as a synthetic option so the
  // <select> still reflects it.
  const FONT_OPTIONS: { value: string; label: string }[] = [
    { value: "'Geist', 'Segoe UI', system-ui, sans-serif", label: "Geist" },
    { value: "'Geist Mono', ui-monospace, monospace", label: "Geist Mono" },
    { value: "'Consolas', 'SF Mono', ui-monospace, monospace", label: "Consolas / Mono" },
    { value: "'JetBrains Mono', ui-monospace, monospace", label: "JetBrains Mono" },
    { value: "'Segoe UI', system-ui, sans-serif", label: "Segoe UI" },
    { value: "'Inter', system-ui, sans-serif", label: "Inter" },
    { value: "system-ui, sans-serif", label: "System sans" },
    { value: "ui-monospace, monospace", label: "Monospace" },
  ];

  function fontOptionsFor(current: string): { value: string; label: string }[] {
    if (FONT_OPTIONS.some((o) => o.value === current)) {
      return FONT_OPTIONS;
    }
    return [{ value: current, label: "Custom (" + current.split(",")[0].replace(/['"]/g, "") + ")" }, ...FONT_OPTIONS];
  }

  const uiFontOptions = $derived(fontOptionsFor(themeStore.tokens.fontUi));
  const monoFontOptions = $derived(fontOptionsFor(themeStore.tokens.fontMono));

  const LABEL_CASE_OPTS: SegmentedOption[] = [
    { label: "Aa", value: "none" },
    { label: "AA", value: "uppercase" },
  ];
  const LETTER_SPACING_OPTS: SegmentedOption[] = [
    { label: "0", value: "0" },
    { label: ".04", value: "0.04em" },
    { label: ".08", value: "0.08em" },
    { label: ".1", value: "0.1em" },
  ];
  const METER_OPTS: SegmentedOption[] = [
    { label: "Gradient", value: "gradient" },
    { label: "Segmented", value: "segmented" },
  ];
  const SELECTION_OPTS: SegmentedOption[] = [
    { label: "Left bar", value: "left-bar" },
    { label: "Fill", value: "fill" },
  ];
  const BORDER_OPTS: SegmentedOption[] = [
    { label: "1px", value: "1px" },
    { label: "2px", value: "2px" },
  ];

  // --- preset save (inline-rename pattern, mirrors ScenesDock) ----------------

  let saving = $state(false);
  let saveName = $state("");

  function focusOnMount(node: HTMLInputElement) {
    node.focus();
    node.select();
  }
  function beginSave() {
    saving = true;
    saveName = "";
  }
  function commitSave() {
    const name = saveName.trim();
    saving = false;
    if (name) {
      themeStore.saveCustom(name);
    }
  }
  function onSaveKey(e: KeyboardEvent) {
    if (e.key === "Enter") {
      commitSave();
    } else if (e.key === "Escape") {
      saveName = "";
      saving = false;
    }
  }

  function onColorInput(key: keyof ThemeTokens, e: Event) {
    themeStore.setToken(key, (e.currentTarget as HTMLInputElement).value);
  }
</script>

<div class="appearance">
  <h2 class="title">Appearance</h2>

  <section class="block">
    <div class="block-label">Accent</div>
    <div class="accent-swatches">
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

  <div class="block-label">Theme editor</div>
  <p class="note editor-note">
    Full control over individual colors, fonts, and element styles. Edits apply live across the whole interface; save a
    set of tokens as a named preset to reuse it.
  </p>

  <!-- inline per-token editor (was the Theme Editor modal) -->
  <div class="wrap">
    <!-- left: editor groups -->
    <div class="editor">
      <section class="grp">
        <div class="glbl">Palette</div>
        <div class="palette-row">
          {#each PALETTE as p (p.key)}
            <label class="sw" title={p.label}>
              <input
                type="color"
                value={themeStore.tokens[p.key]}
                oninput={(e) => onColorInput(p.key, e)}
                aria-label={p.label}
              />
              <span>{p.label}</span>
            </label>
          {/each}
        </div>
      </section>

      <section class="grp">
        <div class="glbl">Typography</div>
        <div class="ctl">
          <span class="ctl-lbl">UI font</span>
          <select
            class="pick"
            value={themeStore.tokens.fontUi}
            onchange={(e) => themeStore.setToken("fontUi", (e.currentTarget as HTMLSelectElement).value)}
          >
            {#each uiFontOptions as o (o.value)}
              <option value={o.value}>{o.label}</option>
            {/each}
          </select>
        </div>
        <div class="ctl">
          <span class="ctl-lbl">Mono font</span>
          <select
            class="pick"
            value={themeStore.tokens.fontMono}
            onchange={(e) => themeStore.setToken("fontMono", (e.currentTarget as HTMLSelectElement).value)}
          >
            {#each monoFontOptions as o (o.value)}
              <option value={o.value}>{o.label}</option>
            {/each}
          </select>
        </div>
        <div class="ctl">
          <span class="ctl-lbl">Label case</span>
          <Segmented
            options={LABEL_CASE_OPTS}
            value={themeStore.tokens.labelCase}
            onChange={(v) => themeStore.setToken("labelCase", v as ThemeTokens["labelCase"])}
          />
        </div>
        <div class="ctl">
          <span class="ctl-lbl">Spacing</span>
          <Segmented
            options={LETTER_SPACING_OPTS}
            value={themeStore.tokens.letterSpacing}
            onChange={(v) => themeStore.setToken("letterSpacing", v)}
          />
        </div>
      </section>

      <section class="grp">
        <div class="glbl">Element styles</div>
        <div class="ctl">
          <span class="ctl-lbl">Meter</span>
          <Segmented
            options={METER_OPTS}
            value={themeStore.tokens.meterStyle}
            onChange={(v) => themeStore.setToken("meterStyle", v as ThemeTokens["meterStyle"])}
          />
        </div>
        <div class="ctl">
          <span class="ctl-lbl">Selection</span>
          <Segmented
            options={SELECTION_OPTS}
            value={themeStore.tokens.selectionStyle}
            onChange={(v) => themeStore.setToken("selectionStyle", v as ThemeTokens["selectionStyle"])}
          />
        </div>
        <div class="ctl">
          <span class="ctl-lbl">Border</span>
          <Segmented
            options={BORDER_OPTS}
            value={themeStore.tokens.borderWeight}
            onChange={(v) => themeStore.setToken("borderWeight", v)}
          />
        </div>
      </section>

      <div class="presets">
        {#each themeStore.allThemes as t (t.id)}
          <button class="preset" class:on={themeStore.activeId === t.id} onclick={() => themeStore.selectPreset(t.id)}>
            <span class="preset-name">{t.name}</span>
            {#if t.id.startsWith("custom-")}
              <span
                class="preset-del"
                role="button"
                tabindex="0"
                title="Delete theme"
                onclick={(e) => {
                  e.stopPropagation();
                  themeStore.deleteCustom(t.id);
                }}
                onkeydown={(e) => {
                  if (e.key === "Enter" || e.key === " ") {
                    e.stopPropagation();
                    themeStore.deleteCustom(t.id);
                  }
                }}>✕</span
              >
            {/if}
          </button>
        {/each}
        {#if saving}
          <input
            class="save-input"
            placeholder="Theme name"
            bind:value={saveName}
            onkeydown={onSaveKey}
            onblur={commitSave}
            use:focusOnMount
          />
        {:else}
          <button class="preset save" onclick={beginSave}>+ Save…</button>
        {/if}
      </div>
    </div>

    <!-- right: live preview (built entirely from token vars; repaints live) -->
    <div class="prev-col">
      <p class="plbl">Live preview</p>
      <div class="prev">
        <div class="prev-hd"><span>Scenes</span><span>⧉ ✕</span></div>
        <div class="prev-bd">
          <div class="prev-it">Intro</div>
          <div class="prev-it sel">Live · Main</div>
          <div class="prev-it">BRB</div>
          <div class="prev-btn">● Go Live</div>
          <div class="prev-mt"><div class="prev-mt-unlit"></div></div>
        </div>
      </div>
      <p class="cap">Editing tokens on the left repaints this instantly.</p>
    </div>
  </div>
</div>

<style>
  .appearance {
    max-width: 960px;
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
  .accent-swatches {
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
  .editor-note {
    margin: 0 0 12px;
  }

  /* --- inline editor --------------------------------------------------------- */
  .wrap {
    display: flex;
    gap: 16px;
    align-items: flex-start;
  }
  .editor {
    flex: 1;
    min-width: 0;
    background: var(--color-base);
    border: var(--border-weight) solid var(--color-border);
  }

  .grp {
    padding: 10px 11px;
    border-bottom: var(--border-weight) solid var(--color-border);
  }
  .glbl {
    font-size: 9px;
    color: var(--color-muted);
    letter-spacing: 0.08em;
    text-transform: uppercase;
    margin-bottom: 9px;
  }

  .palette-row {
    display: flex;
    gap: 6px;
    flex-wrap: wrap;
    row-gap: 20px;
  }
  .sw {
    position: relative;
    width: 30px;
    height: 24px;
    border: var(--border-weight) solid var(--color-border);
    cursor: pointer;
    display: block;
  }
  .sw input[type="color"] {
    width: 100%;
    height: 100%;
    padding: 0;
    margin: 0;
    border: none;
    background: none;
    cursor: pointer;
  }
  /* Make the native color well fill the swatch with no chrome. */
  .sw input[type="color"]::-webkit-color-swatch-wrapper {
    padding: 0;
  }
  .sw input[type="color"]::-webkit-color-swatch {
    border: none;
  }
  .sw span {
    position: absolute;
    bottom: -14px;
    left: 50%;
    transform: translateX(-50%);
    font-size: 7px;
    color: var(--color-muted);
    white-space: nowrap;
  }

  .ctl {
    display: flex;
    gap: 8px;
    align-items: center;
    margin-bottom: 7px;
  }
  .ctl:last-child {
    margin-bottom: 0;
  }
  .ctl-lbl {
    font-size: 9px;
    color: var(--color-muted);
    letter-spacing: 0.06em;
    text-transform: uppercase;
    width: 64px;
    flex: 0 0 auto;
  }
  .pick {
    flex: 1;
    min-width: 0;
    height: auto;
    background: var(--color-base);
    border: var(--border-weight) solid var(--color-border);
    color: var(--color-text);
    font-family: var(--font-ui);
    font-size: 10px;
    padding: 4px 8px;
  }
  .pick:focus {
    outline: none;
    border-color: var(--color-accent);
  }

  .presets {
    display: flex;
    gap: 6px;
    flex-wrap: wrap;
    align-items: center;
    padding: 10px 11px;
  }
  .preset {
    display: inline-flex;
    align-items: center;
    gap: 6px;
    height: auto;
    padding: 5px 10px;
    font-family: var(--font-ui);
    font-size: 10px;
    background: transparent;
    border: var(--border-weight) solid var(--color-border);
    color: var(--color-text);
  }
  .preset:hover {
    border-color: var(--color-accent);
  }
  .preset.on {
    border-color: var(--color-accent);
    color: var(--color-accent);
    background: color-mix(in srgb, var(--color-accent) 14%, transparent);
  }
  .preset-name {
    letter-spacing: var(--letter-spacing);
    text-transform: var(--label-case);
  }
  .preset-del {
    font-size: 9px;
    line-height: 1;
    color: var(--color-muted);
  }
  .preset-del:hover {
    color: var(--color-live);
  }
  .preset.save {
    color: var(--color-muted);
  }
  .save-input {
    height: auto;
    padding: 5px 8px;
    background: var(--color-base);
    border: var(--border-weight) solid var(--color-accent);
    color: var(--color-text);
    font-family: var(--font-ui);
    font-size: 10px;
    width: 130px;
  }
  .save-input:focus {
    outline: none;
  }

  /* --- live preview: pure token-var DOM, repaints on every setToken --------- */
  .prev-col {
    flex: 0 0 auto;
    width: 210px;
  }
  .plbl {
    font-size: 9px;
    color: var(--color-muted);
    text-transform: uppercase;
    letter-spacing: 0.06em;
    margin: 0 0 6px;
  }
  .prev {
    background: var(--color-base);
    border: var(--border-weight) solid var(--color-border);
    font-family: var(--font-mono);
  }
  .prev-hd {
    display: flex;
    justify-content: space-between;
    background: var(--color-surface);
    border-bottom: var(--border-weight) solid var(--color-border);
    padding: 6px 9px;
    font-size: 9px;
    letter-spacing: var(--letter-spacing);
    text-transform: var(--label-case);
    color: var(--color-accent);
  }
  .prev-bd {
    padding: 9px;
  }
  .prev-it {
    font-size: 11px;
    color: var(--color-text);
    padding: 5px 7px;
    border-bottom: var(--border-weight) solid var(--color-border);
    border-left: 3px solid transparent;
    letter-spacing: var(--letter-spacing);
    text-transform: var(--label-case);
  }
  :root[data-selection-style="left-bar"] .prev-it.sel {
    border-left-color: var(--color-accent);
    background: color-mix(in srgb, var(--color-accent) 14%, transparent);
    color: var(--color-accent);
  }
  :root[data-selection-style="fill"] .prev-it.sel {
    background: color-mix(in srgb, var(--color-accent) 24%, transparent);
    color: var(--color-accent);
  }
  .prev-btn {
    background: var(--color-accent);
    color: var(--color-accent-contrast);
    font-size: 11px;
    padding: 7px;
    text-align: center;
    margin-top: 8px;
    font-weight: 700;
    letter-spacing: var(--letter-spacing);
    text-transform: var(--label-case);
  }
  /* Meter idiom from AudioMixerDock: zone gradient track + segmented mask. */
  .prev-mt {
    position: relative;
    height: 8px;
    margin-top: 8px;
    overflow: hidden;
    background-color: var(--color-base);
    background-image: linear-gradient(
      90deg,
      var(--meter-green) 0%,
      var(--meter-green) 58%,
      var(--meter-yellow) 78%,
      var(--meter-red) 100%
    );
  }
  :root[data-meter-style="segmented"] .prev-mt {
    -webkit-mask-image: repeating-linear-gradient(90deg, #000 0 4px, transparent 4px 5px);
    mask-image: repeating-linear-gradient(90deg, #000 0 4px, transparent 4px 5px);
  }
  .prev-mt-unlit {
    position: absolute;
    top: 0;
    right: 0;
    bottom: 0;
    width: 30%;
    background: var(--color-base);
  }
  .cap {
    font-size: 9px;
    color: var(--color-muted);
    margin: 10px 0 0;
    line-height: 1.6;
  }
</style>
