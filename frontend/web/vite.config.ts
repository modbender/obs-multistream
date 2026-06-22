import { defineConfig } from "vite";
import { svelte } from "@sveltejs/vite-plugin-svelte";
import { resolve } from "node:path";

// base './' makes the built index.html reference assets relatively (./assets/*),
// which the C++ app:// scheme handler resolves as app://app/assets/* off the
// bundle root. The bundle must be fully offline: no CDN, no dev server.
export default defineConfig({
  base: "./",
  plugins: [svelte()],
  build: {
    outDir: "dist",
    emptyOutDir: true,
    // Inline nothing as data: URLs forced; keep assets as files the scheme
    // handler serves. Default Rollup chunking is fine under app://.
    target: "es2022",
    rollupOptions: {
      // Multi-page: the production app loads index.html; the throwaway P0
      // windowing spike loads spike.html (gated by FE_SPIKE_WINDOWING host-side).
      input: {
        index: resolve(__dirname, "index.html"),
        spike: resolve(__dirname, "spike.html"),
      },
    },
  },
});
