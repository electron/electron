'use strict'

const common = require('@electron/internal/common/api/exports/electron')
const moduleList = require('@electron/internal/renderer/api/module-list')

// Import common modules.
common.defineProperties(exports)

for (const {
  name,
  file,
  enabled = true,
  private: isPrivate = false
} of moduleList) {
  if (!enabled) {
    continue
  }

  Object.defineProperty(exports, name, {
    enumerable: !isPrivate,
    get: common.memoizedGetter(() => {
      const value = require(`@electron/internal/renderer/api/${file}.js`)
      // Handle Typescript modules with an "export default X" statement
      if (value.__esModule) return value.default
      return value
    })
  })
}
