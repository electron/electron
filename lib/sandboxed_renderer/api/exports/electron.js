'use strict'

const moduleList = require('@electron/internal/sandboxed_renderer/api/module-list')

for (const {
  name,
  load,
  enabled = true,
  configurable = false,
  private: isPrivate = false
} of moduleList) {
  if (!enabled) {
    continue
  }

  Object.defineProperty(exports, name, {
    configurable,
    enumerable: !isPrivate,
    get: load
  })
}
