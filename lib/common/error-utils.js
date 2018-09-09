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
  if (error.__ELECTRON_SERIALIZED_ERROR__ && constructors.has(error.name)) {
    const constructor = constructors.get(error.name)
    const rehydratedError = new constructor(error.message)
    rehydratedError.stack = error.stack
    return rehydratedError
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
      __ELECTRON_SERIALIZED_ERROR__: true
    }
  }
  return error
}
