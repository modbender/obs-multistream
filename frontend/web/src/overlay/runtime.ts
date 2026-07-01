// The clean native widget API injected into every served overlay document. Runs
// inside an OBS Browser Source (its own CEF process), NOT the app. Reads the
// host-injected window.__OVERLAY__ = {id, token, port, fields} and streams
// NormalizedEvents over SSE. Compiled to dist/overlay/runtime.js by bun build.

interface OverlayBootstrap {
  id: string;
  token: string;
  port: number;
  fields: Record<string, unknown>;
}

// Mirrors bridge.ts NormalizedEvent (kept local so the runtime bundles standalone).
interface NormalizedEvent {
  id: string;
  platform: "twitch" | "youtube" | "kick";
  type: string;
  ts: number;
  actorName: string;
  actorColor?: string;
  amount?: number;
  currency?: string;
  tier?: string;
  months?: number;
  count?: number;
  message?: string;
}

type LoadCtx = { fields: Record<string, unknown> };
type LoadHandler = (ctx: LoadCtx) => void;
type EventHandler = (e: NormalizedEvent) => void;

const boot: OverlayBootstrap = (window as unknown as { __OVERLAY__: OverlayBootstrap }).__OVERLAY__ ?? {
  id: "",
  token: "",
  port: 43000,
  fields: {},
};

const loadHandlers: LoadHandler[] = [];
const eventHandlers: EventHandler[] = [];

const OBSOverlay = {
  fields: boot.fields,
  onLoad(fn: LoadHandler) {
    loadHandlers.push(fn);
  },
  onEvent(fn: EventHandler) {
    eventHandlers.push(fn);
  },
  playSound(url: string, volume = 1) {
    if (!url) return;
    const a = new Audio(url);
    a.volume = Math.max(0, Math.min(1, volume));
    void a.play().catch(() => {});
  },
};

(window as unknown as { OBSOverlay: typeof OBSOverlay }).OBSOverlay = OBSOverlay;

function fireLoad() {
  const ctx: LoadCtx = { fields: boot.fields };
  for (const fn of loadHandlers) {
    try {
      fn(ctx);
    } catch (e) {
      console.log("OBSOverlay onLoad threw: " + (e as Error).message);
    }
  }
  window.dispatchEvent(new CustomEvent("obs:load", { detail: ctx }));
}

function fireEvent(e: NormalizedEvent) {
  for (const fn of eventHandlers) {
    try {
      fn(e);
    } catch (err) {
      console.log("OBSOverlay onEvent threw: " + (err as Error).message);
    }
  }
  window.dispatchEvent(new CustomEvent("obs:event", { detail: e }));
}

// EventSource auto-reconnects on drop; the host keepalive keeps it warm.
const src = new EventSource("/w/" + boot.id + "/events?t=" + boot.token);
src.onmessage = (msg) => {
  try {
    fireEvent(JSON.parse(msg.data) as NormalizedEvent);
  } catch {
    /* ignore a malformed frame */
  }
};

// Fire load once the DOM + handlers are ready. Handlers registered synchronously in
// the user JS run before this microtask, so a raf defer is enough.
if (document.readyState === "complete" || document.readyState === "interactive") {
  requestAnimationFrame(fireLoad);
} else {
  window.addEventListener("DOMContentLoaded", () => requestAnimationFrame(fireLoad));
}

export {}; // isolatedModules module marker; --format iife strips this from the built output
