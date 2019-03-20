import {
  EventEmitter
} from 'events'

enum IpcMainHeirachy {
  ROOT = 'ROOT',
  SCOPED = 'SCOPED',
}

export const createIndependentIpcMain = () => {
  const senderScoped = new Map<number, IpcMain>()

  class IpcMain extends EventEmitter implements Electron.IpcMain {
    constructor (private heirachy: IpcMainHeirachy) {
      super()
      // Do not throw exception when channel name is "error".
      this.on('error', () => {})
    }

    getScopedToSender (webContents: Electron.WebContents) {
      const senderId = webContents.id
      let scoped = senderScoped.get(senderId)
      if (!scoped) {
        // Store based on senderId here and don't persist the webContents to avoid it leaking
        scoped = new IpcMain(IpcMainHeirachy.SCOPED)
        senderScoped.set(senderId, scoped)
      }
      return scoped
    }

    emit (channel: string, event: Electron.IpcMainEvent, ...args: any[]): boolean {
      let handled = false
      // If this is the root instance of IpcMain and we have an event with a valid sender
      // we should try emit the event on the scoped IpcMain instance
      if (this.heirachy === IpcMainHeirachy.ROOT && event && event.sender && event.sender.id) {
        const scoped = senderScoped.get(event.sender.id)
        if (scoped) {
          handled = scoped.emit(channel, event, ...args) || handled
        }
      }
      handled = super.emit(channel, event, ...args) || handled
      return handled
    }
  }

  const ipcMain = new IpcMain(IpcMainHeirachy.ROOT)
  return ipcMain
}
