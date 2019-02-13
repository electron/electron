import { remote, Remote } from 'electron'

export function getRemote (name: keyof Remote) {
  if (!remote) {
    throw new Error(`${name} requires remote, which is not enabled`)
  }
  return remote[name]
}
