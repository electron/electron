exports.print = function (obj) {
  return obj.constructor.name
}

exports.echo = function (obj) {
  return obj
}

exports.typedArray = function (name) {
  const int16 = new Int16Array(name.length)
  for (let i = 0; i < name.length; ++i) {
    int16[i] = name[i]
  }
  return int16
}
