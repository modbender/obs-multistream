// Registry mapping a property descriptor `type` -> the Svelte control component
// that renders it. Keeps PropertyForm data-driven: a new property type is one
// import + one map entry, not another if/else branch.
import type { Component } from "svelte";
import type { PropertyDescriptor } from "../bridge";

import BoolControl from "./BoolControl.svelte";
import NumberControl from "./NumberControl.svelte";
import TextControl from "./TextControl.svelte";
import PathControl from "./PathControl.svelte";
import ListControl from "./ListControl.svelte";
import ColorControl from "./ColorControl.svelte";
import ButtonControl from "./ButtonControl.svelte";
import GroupControl from "./GroupControl.svelte";
import UnsupportedControl from "./UnsupportedControl.svelte";

// Props every control receives. `value` is the current value; `onChange` reports
// a new value for `prop.name` (the form debounces + pushes to properties.set).
// `onButton` invokes properties.button for a named button prop.
export interface ControlProps {
  prop: PropertyDescriptor;
  value: unknown;
  onChange: (name: string, value: unknown) => void;
  onButton: (name: string) => void;
  // Resolve any property's current value from the flat settings map. Groups need
  // it to render their nested children (obs_data settings are flat, not nested).
  lookup: (name: string) => unknown;
}

// eslint-disable-next-line @typescript-eslint/no-explicit-any
type AnyControl = Component<ControlProps, any, any>;

export const controlRegistry: Record<string, AnyControl> = {
  bool: BoolControl as AnyControl,
  int: NumberControl as AnyControl,
  float: NumberControl as AnyControl,
  text: TextControl as AnyControl,
  path: PathControl as AnyControl,
  list: ListControl as AnyControl,
  color: ColorControl as AnyControl,
  color_alpha: ColorControl as AnyControl,
  button: ButtonControl as AnyControl,
  group: GroupControl as AnyControl,
};

export function controlFor(type: string): AnyControl {
  return controlRegistry[type] ?? (UnsupportedControl as AnyControl);
}
