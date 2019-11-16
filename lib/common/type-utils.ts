const { nativeImage, NativeImage } = process.electronBinding('native_image')

export function isPromise (val: any) {
  return (
    val &&
    val.then &&
    val.then instanceof Function &&
    val.constructor &&
    val.constructor.reject &&
    val.constructor.reject instanceof Function &&
    val.constructor.resolve &&
    val.constructor.resolve instanceof Function
  )
}

const serializableTypes = [
  Boolean,
  Number,
  String,
  Date,
  Error,
  RegExp,
  ArrayBuffer
]

export function isSerializableObject (value: any) {
  return value === null || ArrayBuffer.isView(value) || serializableTypes.some(type => value instanceof type)
}

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
  } else if (isSerializableObject(value)) {
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
  } else if (isSerializableObject(value)) {
    return value
  } else if (value instanceof Object) {
    return objectMap(value, deserialize)
  } else {
    return value
  }
}
