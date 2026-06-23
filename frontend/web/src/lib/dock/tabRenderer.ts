import type { ITabRenderer, TabPartInitParameters } from "dockview-core";

// Custom dock-header chrome per dock-model.html: a title plus the `⧉` tear-out and
// `✕` close affordances. Implemented as a vanilla dockview ITabRenderer (the tab IS
// the dock header), so it stays framework-agnostic and is driven entirely off the
// panel's init params + its DockviewPanelApi.
//
// - `✕` calls api.close() -> the panel is removed; the Docks menu reopens it.
// - `⧉` calls the detachDock seam threaded through params.__detach (P1: logs only;
//   real floating-window detach lands next phase per the P1 floating-deferral).
// - `__accent` tints the title to match the accent docks in the mock.
//
// Header chrome (uppercase title, 0-radius buttons) is styled in app.css against
// these class names so it re-skins live with the active theme.
export class DockTab implements ITabRenderer {
  private readonly _element: HTMLDivElement;
  private readonly _title: HTMLSpanElement;
  private readonly _detachBtn: HTMLButtonElement;
  private readonly _closeBtn: HTMLButtonElement;
  private _onDetach: (() => void) | undefined;
  private _onClose: (() => void) | undefined;

  constructor() {
    this._element = document.createElement("div");
    this._element.className = "obs-dock-tab";

    this._title = document.createElement("span");
    this._title.className = "obs-dock-title";

    this._detachBtn = document.createElement("button");
    this._detachBtn.className = "obs-dock-btn";
    this._detachBtn.title = "Tear out to its own window";
    this._detachBtn.textContent = "⧉"; // ⧉
    this._detachBtn.addEventListener("click", this.handleDetach);

    this._closeBtn = document.createElement("button");
    this._closeBtn.className = "obs-dock-btn";
    this._closeBtn.title = "Close (reopen from Docks menu)";
    this._closeBtn.textContent = "✕"; // ✕
    this._closeBtn.addEventListener("click", this.handleClose);

    const btns = document.createElement("span");
    btns.className = "obs-dock-btns";
    btns.appendChild(this._detachBtn);
    btns.appendChild(this._closeBtn);

    this._element.appendChild(this._title);
    this._element.appendChild(btns);
  }

  get element(): HTMLElement {
    return this._element;
  }

  init(params: TabPartInitParameters): void {
    this._title.textContent = params.title;

    const accent = params.params?.__accent === true;
    this._element.classList.toggle("accent", accent);

    const panelId = params.api.id;
    const detach = params.params?.__detach as ((id: string) => void) | undefined;
    this._onDetach = detach ? () => detach(panelId) : undefined;
    this._onClose = () => params.api.close();
  }

  private readonly handleDetach = (e: MouseEvent): void => {
    e.stopPropagation();
    this._onDetach?.();
  };

  private readonly handleClose = (e: MouseEvent): void => {
    e.stopPropagation();
    this._onClose?.();
  };

  dispose(): void {
    this._detachBtn.removeEventListener("click", this.handleDetach);
    this._closeBtn.removeEventListener("click", this.handleClose);
  }
}
