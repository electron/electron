'use strict'

const moduleList = require('@electron/internal/common/api/module-list')

exports.memoizedGetter = (getter) => {
  /*
   * It's ok to leak this value as it would be leaked by the global
   * node module cache anyway at `Module._cache`.  This memoization
   * is dramatically faster than relying on nodes module cache however
   */
  let memoizedValue = null

  return () => {
    if (memoizedValue === null) {
      memoizedValue = getter()
    }
    return memoizedValue
  }
}

// Attaches properties to |targetExports|.
exports.defineProperties = function (targetExports) {
  const descriptors = {}
  for (const module of moduleList) {
    descriptors[module.name] = {
      enumerable: !module.private,
      get: exports.memoizedGetter(() => require(`@electron/internal/common/api/${module.file}`))
    }
  }
  return Object.defineProperties(targetExports, descriptors)
}
