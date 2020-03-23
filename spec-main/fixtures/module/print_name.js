exports.print = function (obj) {
  return obj.constructor.name;
};

exports.echo = function (obj) {
  return obj;
};

const typedArrays = {
  Int8Array,
  Uint8Array,
  Uint8ClampedArray,
  Int16Array,
  Uint16Array,
  Int32Array,
  Uint32Array,
  Float32Array,
  Float64Array
};

exports.typedArray = function (type, values) {
  const constructor = typedArrays[type];
  const array = new constructor(values.length);
  for (let i = 0; i < values.length; ++i) {
    array[i] = values[i];
  }
  return array;
};

exports.getNaN = function () {
  return NaN;
};

exports.getInfinity = function () {
  return Infinity;
};
