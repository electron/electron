'use strict'

const moduleList = require('@electron/internal/sandboxed_renderer/api/module-list')

const handleESModule = (m) => {
  // Handle Typescript modules with an "export default X" statement
  if (m.__esModule) return m.default
  return m
}

for (const {
  name,
  load,
  enabled = true,
  private: isPrivate = false
} of moduleList) {
  if (!enabled) {
    continue
  }

  Object.defineProperty(exports, name, {
    enumerable: !isPrivate,
    get: () => handleESModule(load())
  })
}
