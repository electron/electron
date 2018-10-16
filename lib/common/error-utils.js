'use strict'

const constructors = new Map([
  [Error.name, Error],
  [EvalError.name, EvalError],
  [RangeError.name, RangeError],
  [ReferenceError.name, ReferenceError],
  [SyntaxError.name, SyntaxError],
  [TypeError.name, TypeError],
  [URIError.name, URIError]
])

exports.deserialize = function (error) {
  if (error && error.__ELECTRON_SERIALIZED_ERROR__ && constructors.has(error.name)) {
    const constructor = constructors.get(error.name)
    const deserializedError = new constructor(error.message)
    deserializedError.stack = error.stack
    deserializedError.from = error.from
    deserializedError.cause = exports.deserialize(error.cause)
    return deserializedError
  }
  return error
}

exports.serialize = function (error) {
  if (error instanceof Error) {
    // Errors get lost, because: JSON.stringify(new Error('Message')) === {}
    // Take the serializable properties and construct a generic object
    return {
      message: error.message,
      stack: error.stack,
      name: error.name,
      from: process.type,
      cause: exports.serialize(error.cause),
      __ELECTRON_SERIALIZED_ERROR__: true
    }
  }
  return error
}
