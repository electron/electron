module.exports = function atomBindingSetup (binding, processType) {
  return function atomBinding (name) {
    try {
      return binding(`atom_${processType}_${name}`)
    } catch (error) {
      if (/No such module/.test(error.message)) {
        return binding(`atom_common_${name}`)
      } else {
        throw error
      }
    }
  }
}
