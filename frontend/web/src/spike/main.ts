import { mount } from "svelte";
import SpikeApp from "./SpikeApp.svelte";
import "dockview-core/dist/styles/dockview.css";

const params = new URLSearchParams(location.search);
const windowId = Number(params.get("window") ?? "0");
const dockId = params.get("dock"); // null in the main window; a dock id in a detached window

mount(SpikeApp, {
  target: document.getElementById("app")!,
  props: { windowId, dockId },
});
