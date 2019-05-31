'use strict'

const common = require('@electron/internal/common/api/exports/electron')
const moduleList = require('@electron/internal/renderer/api/module-list')

// Import common modules.
common.defineProperties(exports)

for (const module of moduleList) {
  Object.defineProperty(exports, module.name, {
    enumerable: !module.private,
    get: common.handleESModule(module.loader)
  })
}
