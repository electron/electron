export function electronBindingSetup (binding: typeof process['_linkedBinding'], processType: typeof process['type']): typeof process['electronBinding'] {
  return function electronBinding (name: string) {
    try {
      return binding(`electron_${processType}_${name}`);
    } catch (error) {
      if (/No such module/.test(error.message)) {
        return binding(`electron_common_${name}`);
      } else {
        throw error;
      }
    }
  };
}
