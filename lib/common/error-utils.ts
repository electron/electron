const constructors = new Map([
  [Error.name, Error],
  [EvalError.name, EvalError],
  [RangeError.name, RangeError],
  [ReferenceError.name, ReferenceError],
  [SyntaxError.name, SyntaxError],
  [TypeError.name, TypeError],
  [URIError.name, URIError]
])

export function deserialize (error: Electron.SerializedError): Electron.ErrorWithCause {
  if (error && error.__ELECTRON_SERIALIZED_ERROR__ && constructors.has(error.name)) {
    const constructor = constructors.get(error.name)
    const deserializedError = new constructor!(error.message) as Electron.ErrorWithCause
    deserializedError.stack = error.stack
    deserializedError.from = error.from
    deserializedError.cause = exports.deserialize(error.cause)
    return deserializedError
  }
  return error
}

export function serialize (error: Electron.ErrorWithCause): Electron.SerializedError {
  if (error instanceof Error) {
    // Errors get lost, because: JSON.stringify(new Error('Message')) === {}
    // Take the serializable properties and construct a generic object
    return {
      message: error.message,
      stack: error.stack,
      name: error.name,
      from: process.type as Electron.ProcessType,
      cause: exports.serialize(error.cause),
      __ELECTRON_SERIALIZED_ERROR__: true
    }
  }
  return error
}
