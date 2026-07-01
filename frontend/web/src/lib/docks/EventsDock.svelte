<script lang="ts">
  import { obs, type NormalizedEvent, type ChatPlatform, type EventType } from "../bridge";

  // Host supplies tab chrome + strips __* keys; this body declares no props.
  let {}: Record<string, unknown> = $props();

  // Platform dot/tag color + actor-name fallback (matches the Multichat dock).
  const PLATFORM_COLOR: Record<ChatPlatform, string> = {
    twitch: "#a970ff",
    youtube: "#ff4e45",
    kick: "#53fc18",
  };
  const PLATFORM_LABEL: Record<ChatPlatform, string> = {
    twitch: "Twitch",
    youtube: "YouTube",
    kick: "Kick",
  };
  // Human labels per event type (skeleton copy; richer cards come in a later phase).
  const TYPE_LABEL: Record<EventType, string> = {
    follow: "Follow",
    sub: "Sub",
    resub: "Resub",
    subgift: "Gift Sub",
    cheer: "Cheer",
    raid: "Raid",
    superchat: "Super Chat",
    supersticker: "Super Sticker",
    member: "Member",
  };

  // --- feed (ring-capped + virtualized) -------------------------------------
  // Hard cap on retained events so a sustained feed can't grow the array or the
  // measured-height map without bound; trimming prunes both in lockstep.
  const MAX = 500;
  const ESTIMATE = 34; // px height estimate for an unmeasured row
  const OVERSCAN = 6; // rows rendered beyond the viewport on each side
  const STICK_PX = 24; // within this of the bottom counts as "stuck to latest"

  // Each row carries a client-assigned monotonic key so the render key never
  // depends on the host-supplied ev.id (which could arrive empty or duplicated and
  // would throw each_key_duplicate). ev.id is retained for any host-side identity
  // needs; the keyed list + heights map use clientKey only.
  let events = $state<{ clientKey: number; e: NormalizedEvent }[]>([]);
  let seq = 0;
  // clientKey -> measured row height. Plain map (not deep-reactive); a height change
  // bumps `measureVersion` to recompute the layout. Pruned alongside the ring trim.
  const heights = new Map<number, number>();
  let measureVersion = $state(0);

  let scrollEl: HTMLDivElement | undefined;
  let viewTop = $state(0);
  let viewH = $state(0);
  let autoStick = $state(true);

  // Batch incoming events onto a single rAF flush so a burst re-renders once
  // (not once per event) and the array is rebuilt at most once per frame.
  let pending: NormalizedEvent[] = [];
  let rafId = 0;
  function enqueue(e: NormalizedEvent): void {
    pending.push(e);
    if (!rafId) rafId = requestAnimationFrame(flush);
  }
  function flush(): void {
    rafId = 0;
    if (pending.length === 0) return;
    let next = events.concat(pending.map((e) => ({ clientKey: ++seq, e })));
    pending = [];
    if (next.length > MAX) {
      // clientKeys are unique+monotonic, so trimmed rows never alias a kept one --
      // prune their heights directly, in lockstep with the ring trim.
      for (const d of next.slice(0, next.length - MAX)) heights.delete(d.clientKey);
      next = next.slice(next.length - MAX);
    }
    events = next;
  }

  // Replace the whole feed. events.list / events.backfill arrive newest-first, so
  // reverse into oldest->newest (top->bottom) to match the enqueue-at-bottom order.
  // Drops any pending appends and clears the measured heights (fresh clientKeys).
  function setFeed(list: NormalizedEvent[]): void {
    pending = [];
    if (rafId) {
      cancelAnimationFrame(rafId);
      rafId = 0;
    }
    heights.clear();
    const rows: { clientKey: number; e: NormalizedEvent }[] = new Array(list.length);
    for (let i = 0; i < list.length; i++) {
      rows[i] = { clientKey: ++seq, e: list[list.length - 1 - i] };
    }
    events = rows;
    measureVersion++;
    autoStick = true;
  }

  // Cumulative row offsets + total height; recomputed when the event set or any
  // measured height changes. O(n) over the <=MAX ring -- cheap.
  let layout = $derived.by<{ tops: number[]; total: number }>(() => {
    void measureVersion;
    const tops = new Array<number>(events.length);
    let acc = 0;
    for (let i = 0; i < events.length; i++) {
      tops[i] = acc;
      acc += heights.get(events[i].clientKey) ?? ESTIMATE;
    }
    return { tops, total: acc };
  });

  // Visible window [start, end) including overscan, from the scroll offset.
  let range = $derived.by<{ start: number; end: number }>(() => {
    const { tops } = layout;
    const n = events.length;
    if (n === 0) return { start: 0, end: 0 };
    const top = viewTop;
    const bottom = viewTop + viewH;
    let start = n - 1;
    for (let i = 0; i < n; i++) {
      const h = heights.get(events[i].clientKey) ?? ESTIMATE;
      if (tops[i] + h > top) {
        start = i;
        break;
      }
    }
    let end = n;
    for (let i = start; i < n; i++) {
      if (tops[i] > bottom) {
        end = i;
        break;
      }
    }
    return { start: Math.max(0, start - OVERSCAN), end: Math.min(n, end + OVERSCAN) };
  });

  let visible = $derived(
    events.slice(range.start, range.end).map((row, i) => ({ ...row, top: layout.tops[range.start + i] })),
  );

  // Measure each rendered row; a height delta bumps measureVersion so the layout
  // (and thus the bottom anchor) reflects real wrapped heights.
  function measureRow(node: HTMLElement, key: number): { destroy(): void } {
    const apply = (): void => {
      const h = node.offsetHeight;
      if (h > 0 && heights.get(key) !== h) {
        heights.set(key, h);
        measureVersion++;
      }
    };
    const ro = new ResizeObserver(apply);
    ro.observe(node);
    apply();
    return {
      destroy() {
        ro.disconnect();
      },
    };
  }

  function onScroll(): void {
    if (!scrollEl) return;
    viewTop = scrollEl.scrollTop;
    viewH = scrollEl.clientHeight;
    autoStick = scrollEl.scrollHeight - scrollEl.scrollTop - scrollEl.clientHeight <= STICK_PX;
  }

  function jumpToLatest(): void {
    autoStick = true;
    if (scrollEl) scrollEl.scrollTop = scrollEl.scrollHeight;
  }

  // Keep the view pinned to the newest event while stuck to the bottom. Depends on
  // layout.total so it re-pins after a freshly measured row grows the sizer.
  $effect(() => {
    void layout.total;
    if (autoStick && scrollEl) {
      scrollEl.scrollTop = scrollEl.scrollHeight;
      // Keep the window state coherent without waiting on the async scroll event.
      viewTop = scrollEl.scrollTop;
    }
  });

  // Track the viewport height (jump-to-latest visibility + range both need it).
  $effect(() => {
    if (!scrollEl) return;
    viewH = scrollEl.clientHeight;
    const ro = new ResizeObserver(() => {
      if (scrollEl) viewH = scrollEl.clientHeight;
    });
    ro.observe(scrollEl);
    return () => ro.disconnect();
  });

  // Build the one-line summary (excluding message, which is bound separately so it
  // renders as escaped text). Optional fields are appended only when present.
  function summary(e: NormalizedEvent): string {
    const parts: string[] = [];
    if (e.tier) parts.push("Tier " + e.tier);
    if (e.months) parts.push(e.months + " mo");
    if (e.count) parts.push("x" + e.count);
    if (e.amount != null) parts.push(e.currency ? e.amount + " " + e.currency : String(e.amount));
    return parts.join(" · ");
  }

  function clear(): void {
    // The host clears its store then emits an empty events.backfill; setFeed([])
    // below is a local echo so the feed empties immediately even if the push lags.
    obs.call("events.clear").catch(() => {});
    setFeed([]);
  }

  $effect(() => {
    obs
      .call("events.list")
      .then((list) => setFeed(list))
      .catch(() => {});
    const offNew = obs.on("events.new", (e) => enqueue(e));
    const offBackfill = obs.on("events.backfill", (batch) => setFeed(batch));
    return () => {
      offNew();
      offBackfill();
      if (rafId) cancelAnimationFrame(rafId);
      rafId = 0;
      pending = [];
    };
  });
</script>

<div class="events">
  <div class="scroll" bind:this={scrollEl} onscroll={onScroll}>
    {#if events.length === 0}
      <p class="empty">Follows, subs, gifts and cheers from your connected accounts appear here.</p>
    {:else}
      <div class="sizer" style:height={layout.total + "px"}>
        {#each visible as row (row.clientKey)}
          {@const e = row.e}
          {@const platformColor = PLATFORM_COLOR[e.platform] ?? "var(--color-muted)"}
          {@const actorColor = e.actorColor || platformColor}
          {@const sum = summary(e)}
          <div class="row" style:top={row.top + "px"} use:measureRow={row.clientKey}>
            <span class="pdot" style:background={platformColor} title={PLATFORM_LABEL[e.platform] ?? e.platform}></span>
            <span class="type">{TYPE_LABEL[e.type] ?? e.type}</span>
            <span class="actor" style:color={actorColor}>{e.actorName}</span>
            {#if sum}<span class="meta">{sum}</span>{/if}
            {#if e.message}<span class="msg">{e.message}</span>{/if}
          </div>
        {/each}
      </div>
    {/if}
  </div>

  {#if !autoStick && events.length > 0}
    <button class="jump" onclick={jumpToLatest}>↓ Jump to latest</button>
  {/if}

  <div class="footer">
    <button class="clearbtn" disabled={events.length === 0} onclick={clear}>Clear</button>
  </div>
</div>

<style>
  .events {
    height: 100%;
    display: flex;
    flex-direction: column;
    background: var(--color-surface);
    font-family: var(--font-ui);
    min-height: 0;
    position: relative;
  }
  .scroll {
    flex: 1;
    min-height: 0;
    overflow-y: auto;
    overflow-x: hidden;
  }
  .empty {
    margin: 0;
    padding: 14px 10px;
    font-size: 11px;
    color: var(--color-muted);
    letter-spacing: var(--letter-spacing);
    text-transform: var(--label-case);
  }
  /* Absolute-positioned rows over a sized spacer = virtualized list (only the
     visible window is in the DOM; the sizer reserves the full scroll height). */
  .sizer {
    position: relative;
    width: 100%;
  }
  .row {
    position: absolute;
    left: 0;
    right: 0;
    display: flex;
    flex-wrap: wrap;
    align-items: baseline;
    gap: 0 6px;
    padding: 4px 10px;
    font-size: 12px;
    line-height: 1.5;
    color: var(--color-text);
    word-break: break-word;
  }
  .pdot {
    align-self: center;
    width: 7px;
    height: 7px;
    flex: 0 0 auto;
  }
  .type {
    flex: 0 0 auto;
    font-family: var(--font-mono);
    font-size: 9px;
    text-transform: uppercase;
    letter-spacing: 0.04em;
    color: var(--color-base);
    background: var(--color-muted);
    padding: 0 4px;
    align-self: center;
  }
  .actor {
    font-weight: 600;
    overflow-wrap: anywhere;
  }
  .meta {
    color: var(--color-dim);
  }
  .msg {
    color: var(--color-text);
    overflow-wrap: anywhere;
  }

  .jump {
    position: absolute;
    left: 50%;
    transform: translateX(-50%);
    bottom: 46px;
    z-index: 2;
    padding: 4px 12px;
    font-size: 10px;
    font-family: var(--font-ui);
    color: var(--color-accent-ink);
    background: var(--color-accent);
    border: 0;
    cursor: pointer;
  }

  .footer {
    flex: 0 0 auto;
    display: flex;
    justify-content: flex-end;
    padding: 6px 8px;
    border-top: var(--border-weight) solid var(--color-border);
    background: var(--color-surface-2);
  }
  .clearbtn {
    padding: 3px 12px;
    font-size: 10px;
    font-family: var(--font-ui);
    color: var(--color-dim);
    background: transparent;
    border: var(--border-weight) solid var(--color-border);
    cursor: pointer;
  }
  .clearbtn:disabled {
    opacity: 0.5;
    cursor: default;
  }
</style>
