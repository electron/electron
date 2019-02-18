const v8Util = process.atomBinding('v8_util')

export class CallbacksRegistry {
  public nextId = 0
  public callbacks: Record<number, any> = {}

  public add (callback: (...args: Array<any>) => any) {
    // The callback is already added.
    let id: number | null = v8Util.getHiddenValue(callback, 'callbackId')
    if (id != null) return id

    id = this.nextId += 1

    // Capture the location of the function and put it in the ID string,
    // so that release errors can be tracked down easily.
    const regexp = /at (.*)/gi
    const stackString = (new Error()).stack

    let filenameAndLine
    let match

    while ((match = regexp.exec(stackString || '')) !== null) {
      const location = match[1]
      if (location.includes('(native)')) continue
      if (location.includes('(<anonymous>)')) continue
      if (location.includes('electron.asar')) continue

      const ref = /([^/^)]*)\)?$/gi.exec(location)

      if (!ref) continue
      filenameAndLine = ref[1]
      break
    }

    this.callbacks[id] = callback
    v8Util.setHiddenValue(callback, 'callbackId', id)
    v8Util.setHiddenValue(callback, 'location', filenameAndLine)

    return id
  }

  get (id: number) {
    return this.callbacks[id] || function () {}
  }

  apply (id: number, ...args: Array<any>) {
    return this.get(id).apply(global, ...args)
  }

  remove (id: number) {
    const callback = this.callbacks[id]
    if (callback) {
      v8Util.deleteHiddenValue(callback, 'callbackId')
      delete this.callbacks[id]
    }
  }
}
