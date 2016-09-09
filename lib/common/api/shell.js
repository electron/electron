const fs = require('fs')
const path = require('path')
const shell = process.atomBinding('shell')

module.exports = shell

const squirrelUpdateExePath = path.join(path.dirname(process.execPath), '..', 'update.exe')

if (process.platform === 'win32' && fs.existsSync(squirrelUpdateExePath)) {
  const originalWriteShortcutLink = shell.writeShortcutLink
  shell.writeShortcutLink = (shortcutPath, operation, opts) => {
    const options = opts || operation || {}

    if (!options.target) {
      options.target = squirrelUpdateExePath
      const processArgs = options.args || ''
      options.args = `--processStart="${path.basename(process.execPath)}" --process-start-args=${JSON.stringify(processArgs)}`
    }
    if (opts) {
      return originalWriteShortcutLink(shortcutPath, operation, options)
    }
    return originalWriteShortcutLink(shortcutPath, options)
  };
}
