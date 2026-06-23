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
  },
});
