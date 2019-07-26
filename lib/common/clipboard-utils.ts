const { nativeImage, NativeImage } = process.electronBinding('native_image')

const objectMap = function (source: Object, mapper: (value: any) => any) {
  const sourceEntries = Object.entries(source)
  const targetEntries = sourceEntries.map(([key, val]) => [key, mapper(val)])
  return Object.fromEntries(targetEntries)
}

export function serialize (value: any): any {
  if (value instanceof NativeImage) {
    return {
      buffer: value.toBitmap(),
      size: value.getSize(),
      __ELECTRON_SERIALIZED_NativeImage__: true
    }
  } else if (Array.isArray(value)) {
    return value.map(serialize)
  } else if (value instanceof Buffer) {
    return value
  } else if (value instanceof Object) {
    return objectMap(value, serialize)
  } else {
    return value
  }
}

export function deserialize (value: any): any {
  if (value && value.__ELECTRON_SERIALIZED_NativeImage__) {
    return nativeImage.createFromBitmap(value.buffer, value.size)
  } else if (Array.isArray(value)) {
    return value.map(deserialize)
  } else if (value instanceof Buffer) {
    return value
  } else if (value instanceof Object) {
    return objectMap(value, deserialize)
  } else {
    return value
  }
}
