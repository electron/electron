const common = require('../../../common/api/exports/electron')
const moduleList = require('../module-list')

// Import common modules.
common.defineProperties(exports)

for (const module of moduleList) {
  Object.defineProperty(exports, module.name, {
    enumerable: !module.private,
    get: () => require(`../${module.file}`)
  })
}
