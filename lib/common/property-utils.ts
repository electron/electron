export function defineProperty<T extends object, K extends keyof T>(target: T, propertyName: K, getter: () => T[K], setter?: (value: T[K]) => void) {
  Object.defineProperty(target, propertyName, {
    get: () => getter.call(target),
    set: setter ? (value) => setter.call(target, value) : undefined
  });
}
