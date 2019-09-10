import { Readable, PassThrough, Duplex, Transform } from 'stream'

type ReadableStreamLike = Readable | Duplex | Transform | PassThrough
type Base64String = string

export function isReadableStream (value: ReadableStreamLike): boolean {
  return value instanceof Readable && value.readable
}

export function readableStreamToMeta (value: ReadableStreamLike): Base64String {
  let buffer = Buffer.from('')
  let chunk = value.read()
  while (chunk) {
    buffer = Buffer.concat([buffer, chunk])
    chunk = value.read()
  }
  return buffer.toString('base64')
}

export function metaToReadableStream (value: Base64String): ReadableStreamLike {
  const stream = new PassThrough()
  stream.push(Buffer.from(value, 'base64'))
  stream.push(null)
  return stream
}
