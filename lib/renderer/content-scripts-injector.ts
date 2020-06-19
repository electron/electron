import { webFrame } from 'electron';

import * as ipcRendererUtils from '@electron/internal/renderer/ipc-renderer-internal-utils';

const v8Util = process._linkedBinding('electron_common_v8_util');

const IsolatedWorldIDs = {
  /**
   * Start of extension isolated world IDs, as defined in
   * electron_render_frame_observer.h
   */
  ISOLATED_WORLD_EXTENSIONS: 1 << 20
};

let isolatedWorldIds = IsolatedWorldIDs.ISOLATED_WORLD_EXTENSIONS;
const extensionWorldId: {[key: string]: number | undefined} = {};

// https://cs.chromium.org/chromium/src/extensions/renderer/script_injection.cc?type=cs&sq=package:chromium&g=0&l=52
const getIsolatedWorldIdForInstance = () => {
  // TODO(samuelmaddock): allocate and cleanup IDs
  return isolatedWorldIds++;
};

const escapePattern = function (pattern: string) {
  return pattern.replace(/[\\^$+?.()|[\]{}]/g, '\\$&');
};

// Check whether pattern matches.
// https://developer.chrome.com/extensions/match_patterns
const matchesPattern = function (pattern: string) {
  if (pattern === '<all_urls>') return true;
  const regexp = new RegExp(`^${pattern.split('*').map(escapePattern).join('.*')}$`);
  const url = `${location.protocol}//${location.host}${location.pathname}`;
  return url.match(regexp);
};

// Run the code with chrome API integrated.
const runContentScript = function (this: any, extensionId: string, url: string, code: string) {
  // Assign unique world ID to each extension
  const worldId = extensionWorldId[extensionId] ||
    (extensionWorldId[extensionId] = getIsolatedWorldIdForInstance());

  // store extension ID for content script to read in isolated world
  v8Util.setHiddenValue(global, `extension-${worldId}`, extensionId);

  webFrame.setIsolatedWorldInfo(worldId, {
    name: `${extensionId} [${worldId}]`
    // TODO(samuelmaddock): read `content_security_policy` from extension manifest
    // csp: manifest.content_security_policy,
  });

  const sources = [{ code, url }];
  return webFrame.executeJavaScriptInIsolatedWorld(worldId, sources);
};

const runAllContentScript = function (scripts: Array<Electron.InjectionBase>, extensionId: string) {
  for (const { url, code } of scripts) {
    runContentScript.call(window, extensionId, url, code);
  }
};

const runStylesheet = function (this: any, url: string, code: string) {
  webFrame.insertCSS(code);
};

const runAllStylesheet = function (css: Array<Electron.InjectionBase>) {
  for (const { url, code } of css) {
    runStylesheet.call(window, url, code);
  }
};

// Run injected scripts.
// https://developer.chrome.com/extensions/content_scripts
const injectContentScript = function (extensionId: string, script: Electron.ContentScript) {
  if (!process.isMainFrame && !script.allFrames) return;
  if (!script.matches.some(matchesPattern)) return;

  if (script.js) {
    const fire = runAllContentScript.bind(window, script.js, extensionId);
    if (script.runAt === 'document_start') {
      process.once('document-start', fire);
    } else if (script.runAt === 'document_end') {
      process.once('document-end', fire);
    } else {
      document.addEventListener('DOMContentLoaded', fire);
    }
  }

  if (script.css) {
    const fire = runAllStylesheet.bind(window, script.css);
    if (script.runAt === 'document_start') {
      process.once('document-start', fire);
    } else if (script.runAt === 'document_end') {
      process.once('document-end', fire);
    } else {
      document.addEventListener('DOMContentLoaded', fire);
    }
  }
};

// Handle the request of chrome.tabs.executeJavaScript.
ipcRendererUtils.handle('CHROME_TABS_EXECUTE_SCRIPT', function (
  event: Electron.Event,
  extensionId: string,
  url: string,
  code: string
) {
  return runContentScript.call(window, extensionId, url, code);
});

module.exports = (entries: Electron.ContentScriptEntry[]) => {
  for (const entry of entries) {
    if (entry.contentScripts) {
      for (const script of entry.contentScripts) {
        injectContentScript(entry.extensionId, script);
      }
    }
  }
};
