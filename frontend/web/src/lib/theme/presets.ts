import type { ThemeTokens } from "./tokens";

// Density-derived spacing/height: both ship presets use "comfortable" per §5.2.
const COMFORTABLE = { spaceUnit: "8px", controlHeight: "30px" } as const;

export const INDUSTRIAL: ThemeTokens = {
  colorBase: "#0a0a0a",
  colorSurface: "#141414",
  colorBorder: "#333333",
  colorText: "#bbbbbb",
  colorMuted: "#666666",
  colorAccent: "#e0a23c",
  colorAccentContrast: "#0a0a0a",
  colorLive: "#ef4444",
  meterGreen: "#3fae4a",
  meterYellow: "#eab308",
  meterRed: "#ef4444",
  fontUi: "'Consolas', 'SF Mono', ui-monospace, monospace",
  fontMono: "'Consolas', 'SF Mono', ui-monospace, monospace",
  labelCase: "uppercase",
  letterSpacing: "0.1em",
  density: "comfortable",
  ...COMFORTABLE,
  meterStyle: "segmented",
  selectionStyle: "left-bar",
  borderWeight: "1px",
  radius: "0",
};

export const GRAPHITE: ThemeTokens = {
  colorBase: "#0d0f12",
  colorSurface: "#171a1f",
  colorBorder: "#20242b",
  colorText: "#c4cad3",
  colorMuted: "#7e8794",
  colorAccent: "#3b82f6",
  colorAccentContrast: "#ffffff",
  colorLive: "#ef4444",
  meterGreen: "#22c55e",
  meterYellow: "#eab308",
  meterRed: "#ef4444",
  fontUi: "'Segoe UI', system-ui, sans-serif",
  fontMono: "'Consolas', ui-monospace, monospace",
  labelCase: "uppercase",
  letterSpacing: "0.08em",
  density: "comfortable",
  ...COMFORTABLE,
  meterStyle: "gradient",
  selectionStyle: "left-bar",
  borderWeight: "1px",
  radius: "0",
};

export const SLATE: ThemeTokens = {
  colorBase: "#13161b",
  colorSurface: "#1d222a",
  colorBorder: "#2b313b",
  colorText: "#cdd4de",
  colorMuted: "#6b7480",
  colorAccent: "#10b981",
  colorAccentContrast: "#04140e",
  colorLive: "#ef4444",
  meterGreen: "#10b981",
  meterYellow: "#f59e0b",
  meterRed: "#ef4444",
  fontUi: "'Inter', system-ui, sans-serif",
  fontMono: "'Consolas', ui-monospace, monospace",
  labelCase: "none",
  letterSpacing: "0.04em",
  density: "comfortable",
  ...COMFORTABLE,
  meterStyle: "gradient",
  selectionStyle: "fill",
  borderWeight: "1px",
  radius: "0",
};

export interface PresetEntry {
  id: string;
  name: string;
  tokens: ThemeTokens;
}

// One entry per ship preset. Industrial is the default (first). Adding a preset
// is a single push here.
export const PRESETS: PresetEntry[] = [
  { id: "industrial", name: "Industrial", tokens: INDUSTRIAL },
  { id: "graphite", name: "Graphite", tokens: GRAPHITE },
  { id: "slate", name: "Slate", tokens: SLATE },
];

export const DEFAULT_PRESET_ID = "industrial";

export function presetById(id: string): PresetEntry {
  return PRESETS.find((p) => p.id === id) ?? PRESETS[0];
}
