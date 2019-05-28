import { EventEmitter } from 'events'
import { IpcMainEvent } from 'electron'

class IpcMain extends EventEmitter {
  _invokeHandlers: {[channel: string]: (e: IpcMainEvent, ...args: any[]) => void};
  constructor () {
    super()
    this._invokeHandlers = {}
  }

  handle (method: string, fn: (e: IpcMainEvent, ...args: any[]) => any) {
    if (method in this._invokeHandlers) {
      throw new Error(`Attempted to register a second handler for '${method}'`)
    }
    if (typeof fn !== 'function') {
      throw new Error(`Expected handler to be a function, but found type '${typeof fn}'`)
    }
    this._invokeHandlers[method] = async (e, ...args) => {
      try {
        (e as any).reply(await Promise.resolve(fn(e, ...args)))
      } catch (err) {
        (e as any).throw(err)
      }
    }
  }

  handleOnce (method: string, fn: Function) {
    this.handle(method, (e, ...args) => {
      this.removeHandler(method)
      return fn(e, ...args)
    })
  }

  removeHandler (method: string) {
    delete this._invokeHandlers[method]
  }
}

const ipcMain = new IpcMain()

// Do not throw exception when channel name is "error".
ipcMain.on('error', () => {})

export default ipcMain
