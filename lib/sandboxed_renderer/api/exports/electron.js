const moduleList = require('../module-list')

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
    get: load
  })
}
