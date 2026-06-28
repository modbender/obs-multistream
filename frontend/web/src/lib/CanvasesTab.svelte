<script lang="ts">
  import { obs, type CanvasInfo, type EncoderType } from "./bridge";
  import PropertyForm from "./properties/PropertyForm.svelte";
  import CanvasEditor from "./CanvasEditor.svelte";

  interface Props {
    /** A canvas uuid to open for editing once the list loads (deep-link from a
     * canvas panel's settings button). */
    editCanvas?: string | null;
  }
  let { editCanvas = null }: Props = $props();

  let canvases = $state<CanvasInfo[]>([]);
  let videoEncoders = $state<EncoderType[]>([]);
  let audioEncoders = $state<EncoderType[]>([]);
  let loaded = $state(false);
  let error = $state<string | null>(null);

  // Inline add/edit editor. `editorCanvas` null => the add form; a canvas => editing.
  let editorOpen = $state(false);
  let editorCanvas = $state<CanvasInfo | null>(null);

  // Which canvas's encoder-settings expander is open (uuid), and which kind.
  let expandedUuid = $state<string | null>(null);
  let expandedKind = $state<"video" | "audio">("video");

  async function loadCanvases() {
    try {
      canvases = await obs.call("canvas.list");
    } catch (e) {
      error = (e as Error).message;
    }
  }

  async function loadAll() {
    error = null;
    try {
      const [list, venc, aenc] = await Promise.all([
        obs.call("canvas.list"),
        obs.call("encoderTypes.list", { kind: "video" }),
        obs.call("encoderTypes.list", { kind: "audio" }),
      ]);
      canvases = list;
      videoEncoders = venc;
      audioEncoders = aenc;
    } catch (e) {
      error = (e as Error).message;
    } finally {
      loaded = true;
    }
  }

  $effect(() => {
    void loadAll();
    // Live-refresh the list when any canvas mutates (this tab or elsewhere).
    const off = obs.on("canvas.changed", () => void loadCanvases());
    return off;
  });

  // Deep-link: when opened with an editCanvas uuid, open that canvas's edit form
  // once the list is loaded (runs once per uuid).
  let deepLinked = $state<string | null>(null);
  $effect(() => {
    if (!editCanvas || !loaded || deepLinked === editCanvas) return;
    const c = canvases.find((x) => x.uuid === editCanvas);
    if (c) {
      deepLinked = editCanvas;
      openEdit(c);
    }
  });

  function fpsText(c: CanvasInfo): string {
    if (!(c.fpsDen > 0)) return String(c.fpsNum);
    return c.fpsDen > 1 ? (c.fpsNum / c.fpsDen).toFixed(2) : String(c.fpsNum);
  }

  function encName(list: EncoderType[], id: string): string {
    return list.find((e) => e.id === id)?.name ?? id ?? "—";
  }

  function gcd(a: number, b: number): number {
    return b === 0 ? a : gcd(b, a % b);
  }
  function ratioLabel(w: number, h: number): string {
    if (!(w > 0) || !(h > 0)) {
      return "";
    }
    const g = gcd(w, h) || 1;
    return `${w / g}:${h / g}`;
  }

  function openAdd() {
    editorCanvas = null;
    editorOpen = true;
  }

  function openEdit(c: CanvasInfo) {
    editorCanvas = c;
    editorOpen = true;
  }

  async function remove(c: CanvasInfo) {
    if (c.isDefault) return;
    try {
      await obs.call("canvas.remove", { uuid: c.uuid });
      if (expandedUuid === c.uuid) expandedUuid = null;
      await loadCanvases();
    } catch (e) {
      error = (e as Error).message;
    }
  }

  function toggleExpand(uuid: string, kind: "video" | "audio") {
    if (expandedUuid === uuid && expandedKind === kind) {
      expandedUuid = null;
    } else {
      expandedUuid = uuid;
      expandedKind = kind;
    }
  }

  // The expander renders once below the grid (not as a grid cell), so resolve the
  // open canvas from its uuid. Clears itself if that canvas is removed.
  const expandedCanvas = $derived(expandedUuid ? (canvases.find((c) => c.uuid === expandedUuid) ?? null) : null);
</script>

<div class="canvases">
  {#if error}<p class="error">{error}</p>{/if}

  {#if !loaded}
    <p class="dim">Loading canvases…</p>
  {:else}
    <ul class="grid">
      {#each canvases as c (c.uuid)}
        {@const portrait = c.baseHeight > c.baseWidth}
        <li class="tile">
          <button class="preview-box" onclick={() => openEdit(c)} title="Edit “{c.name}”">
            <div class="preview-square">
              <div class="preview-frame" class:tall={portrait} style:aspect-ratio="{c.baseWidth} / {c.baseHeight}">
                <span class="res-chip">{c.baseWidth} × {c.baseHeight}</span>
              </div>
            </div>
          </button>
          <div class="tile-foot">
            <div class="info">
              <div class="line1">
                <span class="name">{c.name}</span>
                {#if c.isDefault}<span class="badge">Default</span>{/if}
                <span class="ratio">{ratioLabel(c.baseWidth, c.baseHeight)}</span>
              </div>
              <div class="line2">
                {fpsText(c)} fps · {encName(videoEncoders, c.videoEncoder)} / {encName(audioEncoders, c.audioEncoder)}
              </div>
            </div>
            <div class="rowactions">
              <button class="mini" title="Video encoder settings" onclick={() => toggleExpand(c.uuid, "video")}>V⚙</button>
              <button class="mini" title="Audio encoder settings" onclick={() => toggleExpand(c.uuid, "audio")}>A⚙</button>
              <button class="mini" title="Edit" onclick={() => openEdit(c)}>✎</button>
              <button class="mini danger" title="Remove" disabled={c.isDefault} onclick={() => void remove(c)}>🗑</button>
            </div>
          </div>
        </li>
      {/each}
    </ul>

    {#if expandedCanvas}
      <div class="expander">
        <div class="exp-head">
          {expandedCanvas.name} · {expandedKind === "video" ? "Video" : "Audio"} encoder settings
        </div>
        {#key expandedCanvas.uuid + ":" + expandedKind}
          <PropertyForm kind="encoder" ref={expandedCanvas.uuid + ":" + expandedKind} />
        {/key}
      </div>
    {/if}

    <div class="addbar">
      <button class="btn" onclick={openAdd}>+ Add Canvas</button>
    </div>
  {/if}

  {#if editorOpen}
    {#key editorCanvas?.uuid ?? "__add__"}
      <CanvasEditor
        canvas={editorCanvas}
        {videoEncoders}
        {audioEncoders}
        onClose={() => (editorOpen = false)}
        onSaved={() => {
          editorOpen = false;
          void loadCanvases();
        }}
      />
    {/key}
  {/if}
</div>

<style>
  .canvases {
    padding: 8px 0 4px;
  }
  .grid {
    list-style: none;
    margin: 0;
    padding: 0;
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(280px, 1fr));
    gap: 14px;
  }
  .tile {
    display: flex;
    flex-direction: column;
    border: var(--border-weight) solid var(--color-border);
    background: var(--color-surface);
  }
  .preview-box {
    display: flex;
    align-items: center;
    justify-content: center;
    width: 100%;
    height: 168px;
    padding: 18px;
    background: var(--color-base);
    border: 0;
    border-bottom: var(--border-weight) solid var(--color-border);
    cursor: pointer;
  }
  .preview-box:hover {
    background: color-mix(in srgb, var(--color-text) 3%, var(--color-base));
  }
  .preview-square {
    width: 132px;
    height: 132px;
    display: flex;
    align-items: center;
    justify-content: center;
  }
  .preview-frame {
    position: relative;
    width: 100%;
    height: auto;
    max-height: 100%;
    border: var(--border-weight) solid var(--color-border);
    background: var(--color-surface-2);
    box-shadow: 0 2px 10px rgba(0, 0, 0, 0.35);
  }
  .preview-frame.tall {
    width: auto;
    height: 100%;
    max-width: 100%;
  }
  .res-chip {
    position: absolute;
    top: 5px;
    left: 5px;
    font-family: var(--font-mono);
    font-size: 8px;
    letter-spacing: 0.04em;
    color: var(--color-dim);
    background: color-mix(in srgb, var(--color-base) 70%, transparent);
    padding: 1px 4px;
    white-space: nowrap;
  }
  .tile-foot {
    display: flex;
    align-items: flex-start;
    gap: 10px;
    padding: 12px 12px 10px;
  }
  .info {
    min-width: 0;
    flex: 1;
  }
  .line1 {
    display: flex;
    align-items: center;
    gap: 8px;
  }
  .name {
    font-size: 13px;
    color: var(--color-text);
    font-weight: 500;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }
  .badge {
    flex: 0 0 auto;
    font-size: 9px;
    text-transform: uppercase;
    letter-spacing: 0.05em;
    color: var(--color-accent-ink);
    background: var(--color-accent);
    padding: 1px 6px;
  }
  .ratio {
    margin-left: auto;
    flex: 0 0 auto;
    font-family: var(--font-mono);
    font-size: 10px;
    color: var(--color-muted);
  }
  .line2 {
    font-size: 11px;
    color: var(--color-muted);
    margin-top: 3px;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }
  .rowactions {
    display: flex;
    gap: 4px;
    justify-content: flex-end;
    flex: 0 0 auto;
  }
  .mini {
    background: none;
    border: 1px solid var(--border);
    border-radius: 6px;
    color: var(--text-soft);
    cursor: pointer;
    font: inherit;
    font-size: 12px;
    padding: 4px 7px;
    line-height: 1;
  }
  .mini:hover:not(:disabled) {
    color: var(--text);
    background: var(--bg-raised);
  }
  .mini.danger:hover:not(:disabled) {
    color: var(--off, #d65a5a);
    border-color: var(--off, #d65a5a);
  }
  .mini:disabled {
    opacity: 0.35;
    cursor: default;
  }
  .expander {
    margin-top: 12px;
    border: 1px solid var(--border);
    background: var(--bg-raised);
    padding: 12px 12px 14px;
  }
  .exp-head {
    font-size: 11px;
    text-transform: uppercase;
    letter-spacing: 0.06em;
    color: var(--text-dim);
    margin-bottom: 10px;
  }
  .addbar {
    margin-top: 12px;
  }
  .btn {
    border-radius: 6px;
    padding: 7px 14px;
    font: inherit;
    cursor: pointer;
    border: 1px solid var(--border);
    background: var(--bg-sunken);
    color: var(--text-soft);
  }
  .btn:hover:not(:disabled) {
    color: var(--text);
  }
  .dim {
    color: var(--text-dim);
    margin: 0;
  }
  .error {
    color: var(--off, #d65a5a);
    margin: 0 0 8px;
    font-size: 12px;
  }
</style>
