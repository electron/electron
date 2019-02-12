export function atomBindingSetup (binding: typeof process['binding'], processType: typeof process['type']): typeof process['atomBinding'] {
  return function atomBinding (name: string) {
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
