import { remote } from 'electron'

export function getRemote (name: keyof Electron.Remote) {
  if (!remote) {
    throw new Error(`${name} requires remote, which is not enabled`)
  }
  return remote[name]
}
