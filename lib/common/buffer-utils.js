// Note: Don't use destructuring assignment for `Buffer`, or we'll hit a
// browserify bug that makes the statement invalid, throwing an error in
// sandboxed renderer.
const Buffer = require('buffer').Buffer

const typedArrays = {
  Buffer,
  ArrayBuffer,
  Int8Array,
  Uint8Array,
  Uint8ClampedArray,
  Int16Array,
  Uint16Array,
  Int32Array,
  Uint32Array,
  Float32Array,
  Float64Array
}

function getType (value) {
  for (const type of Object.keys(typedArrays)) {
    if (value instanceof typedArrays[type]) {
      return type
    }
  }
  return null
}

function getBuffer (value) {
  if (value instanceof Buffer) {
    return value
  } else if (value instanceof ArrayBuffer) {
    return Buffer.from(value)
  } else {
    return Buffer.from(value.buffer, value.byteOffset, value.byteLength)
  }
}

exports.isBuffer = function (value) {
  return ArrayBuffer.isView(value) || value instanceof ArrayBuffer
}

exports.bufferToMeta = function (value) {
  return {
    type: getType(value),
    data: getBuffer(value),
    length: value.length
  }
}

exports.metaToBuffer = function (value) {
  const constructor = typedArrays[value.type]
  const data = getBuffer(value.data)

  if (constructor === Buffer) {
    return data
  } else if (constructor === ArrayBuffer) {
    return data.buffer
  } else if (constructor) {
    return new constructor(data.buffer, data.byteOffset, value.length)
  } else {
    return data
  }
}
