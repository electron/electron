const common = require('../../../common/api/exports/electron')
// since browser module list is also used in renderer, keep it separate.
const moduleList = require('../module-list')

// Import common modules.
common.defineProperties(exports)

for (const module of moduleList) {
  Object.defineProperty(exports, module.name, {
    enumerable: !module.private,
    get: () => require(`../${module.file}`)
  })
}
