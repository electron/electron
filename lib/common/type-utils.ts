const { nativeImage, NativeImage } = process.electronBinding('native_image');

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
  );
}

const serializableTypes = [
  Boolean,
  Number,
  String,
  Date,
  Error,
  RegExp,
  ArrayBuffer
];

// https://developer.mozilla.org/en-US/docs/Web/API/Web_Workers_API/Structured_clone_algorithm#Supported_types
export function isSerializableObject (value: any) {
  return value === null || ArrayBuffer.isView(value) || serializableTypes.some(type => value instanceof type);
}

function objectMap (source: Object, mapper: (value: any) => any) {
  const sourceEntries = Object.entries(source);
  const targetEntries = sourceEntries.map(([key, val]) => [key, mapper(val)]);
  return Object.fromEntries(targetEntries);
}

export function serializeNativeImage (image: Electron.NativeImage) {
  const representations = [];
  for (const scaleFactor of image.getScaleFactors()) {
    const size = image.getSize(scaleFactor);
    const dataURL = image.toDataURL({ scaleFactor });
    representations.push({ scaleFactor, size, dataURL });
  }
  return representations;
}

export function deserializeNativeImage (data: any) {
  const image = nativeImage.createEmpty();
  for (const rep of data) {
    const { size, scaleFactor, dataURL } = rep;
    const { width, height } = size;
    image.addRepresentation({ dataURL, scaleFactor, width, height });
  }
  return image;
}

export function serialize (value: any): any {
  if (value instanceof NativeImage) {
    return { __ELECTRON_SERIALIZED_NativeImage__: true, data: serializeNativeImage(value) };
  } else if (value instanceof Buffer) {
    return { __ELECTRON_SERIALIZED_Buffer__: true, data: value };
  } else if (Array.isArray(value)) {
    return value.map(serialize);
  } else if (isSerializableObject(value)) {
    return value;
  } else if (value instanceof Object) {
    return objectMap(value, serialize);
  } else {
    return value;
  }
}

export function deserialize (value: any): any {
  if (value && value.__ELECTRON_SERIALIZED_NativeImage__) {
    return deserializeNativeImage(value.data);
  } else if (value && value.__ELECTRON_SERIALIZED_Buffer__) {
    const { buffer, byteOffset, byteLength } = value.data;
    return Buffer.from(buffer, byteOffset, byteLength);
  } else if (Array.isArray(value)) {
    return value.map(deserialize);
  } else if (isSerializableObject(value)) {
    return value;
  } else if (value instanceof Object) {
    return objectMap(value, deserialize);
  } else {
    return value;
  }
}
