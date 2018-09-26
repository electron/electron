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
    get: common.memoizedGetter(() => require(`@electron/internal/renderer/api/${file}`))
  })
}
