export function electronBindingSetup (binding: typeof process['_linkedBinding']): typeof process['electronBinding'] {
  return function electronBinding (name: string, processType: 'renderer' | 'browser' | 'common') {
    return binding(`electron_${processType}_${name}`);
  };
}
