// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

const binding = process._linkedBinding('electron_browser_api_bridge');

const apiBridgeMain: Electron.ApiBridgeMain = {
  event: () => binding.createEvent(),
  store: (initialValue?: any) => binding.createStore(initialValue),
  sync: (fn) => binding.markSync(fn),
  withCaller: (fn) => binding.markWithCaller(fn)
};

export default apiBridgeMain;
