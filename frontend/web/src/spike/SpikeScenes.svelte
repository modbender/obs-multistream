<script lang="ts">
  import { onMount } from "svelte";
  import { obs, type SceneInfo } from "../lib/bridge";
  let scenes = $state<SceneInfo[]>([]);
  async function reload() {
    scenes = await obs.call("scenes.list");
  }
  onMount(() => {
    void reload();
    const off = obs.on("scenes.changed", () => void reload());
    return off;
  });
  async function add() {
    await obs.call("scenes.create", { name: "Spike " + Date.now() });
  }
</script>

<div class="scenes">
  <button onclick={add}>+ Scene</button>
  <ul>
    {#each scenes as s (s.name)}<li class:cur={s.current}>{s.name}</li>{/each}
  </ul>
</div>

<style>
  .scenes {
    padding: 8px;
    color: #bbb;
    font: 12px monospace;
  }
  li.cur {
    color: #e0a23c;
  }
</style>
