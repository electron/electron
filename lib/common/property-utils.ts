export function defineProperty(target: Object, propertyName: string, getter: () => any, setter?: (value: any) => void) {
  Object.defineProperty(target, propertyName, {
    get: () => getter.call(target),
    set: setter ? (value) => setter.call(target, value) : undefined
  });
}
