import { EventEmitter } from 'events'

class IpcMain extends EventEmitter {
  handle (method: string, fn: Function) {
    if (this.listenerCount(method) !== 0) {
      throw new Error(`Attempted to register a second handler for '${method}'`)
    }
    this.on(method, async (e, ...args) => {
      try {
        e.reply(await Promise.resolve(fn(...args)))
      } catch (err) {
        e.throw(err)
      }
    })
  }
}

const ipcMain = new IpcMain()

// Do not throw exception when channel name is "error".
ipcMain.on('error', () => {})

export default ipcMain
