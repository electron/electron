const common = require('../../../common/api/exports/electron')
const moduleList = require('../module-list')

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
    get: () => require(`../${file}`)
  })
}
