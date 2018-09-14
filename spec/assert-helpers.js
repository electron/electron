function resolveGetters (...args) {
  return args.map(o => JSON.parse(JSON.stringify(o)))
}

module.exports = {
  resolveGetters
}
