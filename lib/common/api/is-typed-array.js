'use strict'

module.exports = function isTypedArray(val) {
  return (
    val &&
    (val instanceof Int8Array
    || val instanceof Int16Array
    || val instanceof Int32Array
    || val instanceof Uint8Array
    || val instanceof Uint8ClampedArray
    || val instanceof Uint16Array
    || val instanceof Uint32Array
    || val instanceof Float32Array
    || val instanceof Float64Array)
  )
}