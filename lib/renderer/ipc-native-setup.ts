import { ipcRendererInternal } from '@electron/internal/renderer/ipc-renderer-internal';

import { ipcRenderer } from 'electron/renderer';

const v8Util = process._linkedBinding('electron_common_v8_util');

// ipc_native::EmitIPCEvent() looks up the "ipcNative" hidden object and emits
// incoming messages on these directly.
v8Util.setHiddenValue(globalThis, 'ipcNative', { ipcRenderer, ipcRendererInternal });
