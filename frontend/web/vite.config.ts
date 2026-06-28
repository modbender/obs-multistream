import { defineConfig } from "vite";
import { svelte } from "@sveltejs/vite-plugin-svelte";

// base './' makes the built index.html reference assets relatively (./assets/*),
// which the C++ app:// scheme handler resolves as app://app/assets/* off the
// bundle root. The bundle is fully offline: no CDN, no dev server.
export default defineConfig({
  base: "./",
  plugins: [svelte()],
  build: {
    outDir: "dist",
    emptyOutDir: true,
    // Keep assets as files the scheme handler serves (no forced data: URLs).
    target: "es2022",
    // The bundle loads from local disk via the app:// scheme, so chunk size has no
    // network cost and code-splitting buys nothing -- the shell needs it all at
    // startup. Raise the warning above today's ~540 kB so it still flags a real
    // regression (e.g. an accidental heavy dep) instead of crying wolf every build.
    chunkSizeWarningLimit: 900,
  },
});
