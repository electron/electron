function resolveSingleObjectGetters (object) {
  if (object && typeof object === 'object') {
    const newObject = {}
    for (const key in object) {
      newObject[key] = resolveGetters(object[key])[0]
    }
    return newObject
  }
  return object
}

function resolveGetters (...args) {
  return args.map(resolveSingleObjectGetters)
}

module.exports = {
  resolveGetters
}
