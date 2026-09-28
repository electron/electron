// Registers the <webview> custom element in a renderer that has `webviewTag`
// enabled. This runs in Electron's context (the isolated world when context
// isolation is on); the main-world half is lib/isolated_renderer. Everything
// else a sandboxed renderer needs is set up natively by
// ElectronSandboxedRendererClient.
import { webViewInit } from '@electron/internal/renderer/web-view/web-view-init';

const { mainFrame } = process._linkedBinding('electron_renderer_web_frame');

webViewInit(mainFrame.getWebPreference('webviewTag'), mainFrame.getWebPreference('isWebView'));
