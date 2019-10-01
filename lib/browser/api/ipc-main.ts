import { IpcMainImpl } from '@electron/internal/browser/ipc-main-impl'

const ipcMain = new IpcMainImpl()

// Do not throw exception when channel name is "error".
ipcMain.on('error', () => {})

export default ipcMain
