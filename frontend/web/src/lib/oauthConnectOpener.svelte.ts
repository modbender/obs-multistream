// Shared opener for the OAuth device-code connect modal. Mirrors missingFilesOpener:
// App owns the single OAuthConnectDialog mount gated on `.open`; the Streams tab
// requests it with the profile + provider to connect via openOAuthConnect().

import { suspendPreview } from "./previewGate.svelte";

/** The profile being connected + which provider drives the device-code flow. */
export interface OAuthConnectRequest {
  profileUuid: string;
  providerId: string;
  platformName: string;
}

export const oauthConnect = $state<{ open: boolean; req: OAuthConnectRequest | null }>({
  open: false,
  req: null,
});

// Hold the preview suspension across the dialog lifetime so the native overlay
// never raises above the modal.
let release: (() => void) | null = null;

/** Open the OAuth connect modal for one profile/provider. */
export function openOAuthConnect(req: OAuthConnectRequest): void {
  oauthConnect.req = req;
  oauthConnect.open = true;
  release ??= suspendPreview();
}

export function closeOAuthConnect(): void {
  oauthConnect.open = false;
  oauthConnect.req = null;
  release?.();
  release = null;
}
