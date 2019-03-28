import * as path from 'path'

export const isParentDir = function (parent: string, dir: string) {
  const relative = path.relative(parent, dir)
  return !!relative && !relative.startsWith('..') && !path.isAbsolute(relative)
}
