import { createIndependentIpcMain } from '@electron/internal/browser/ipc-main-creator'

export const ipcMainInternal = createIndependentIpcMain() as ElectronInternal.IpcMainInternal
