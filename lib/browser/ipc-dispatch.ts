import { ipcMainInternal } from '@electron/internal/browser/ipc-main-internal';
import { MessagePortMain } from '@electron/internal/browser/message-port-main';

import { ipcMain } from 'electron/main';

// IPC from renderers and service workers is delivered to its listeners by
// api::ipc_dispatch; it only needs these JS-side objects.
process._linkedBinding('electron_browser_ipc_dispatch').setup({ ipcMain, ipcMainInternal, MessagePortMain });
