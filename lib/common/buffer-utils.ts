import { Buffer } from 'buffer';

const typedArrays: Record<string, Function> = {
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
};

type BufferLike = Buffer | ArrayBuffer | ArrayBufferView

function getType (value: BufferLike) {
  for (const type of Object.keys(typedArrays)) {
    if (value instanceof typedArrays[type]) {
      return type;
    }
  }
  throw new Error('Invalid buffer');
}

function getBuffer (value: BufferLike) {
  if (value instanceof Buffer) {
    return value;
  } else if (value instanceof ArrayBuffer) {
    return Buffer.from(value);
  } else {
    return Buffer.from(value.buffer, value.byteOffset, value.byteLength);
  }
}

export function isBuffer (value: BufferLike) {
  return ArrayBuffer.isView(value) || value instanceof ArrayBuffer;
}

interface BufferMeta {
  type: keyof typeof typedArrays;
  data: Buffer;
  length: number | undefined;
}

export function bufferToMeta (value: BufferLike): BufferMeta {
  return {
    type: getType(value),
    data: getBuffer(value),
    // NB. We only use length when decoding Int8Array and friends.
    // For other buffer-like types this is expected to be undefined.
    length: (value as Buffer).length
  };
}

export function metaToBuffer (value: BufferMeta) {
  const constructor = typedArrays[value.type];
  const data = getBuffer(value.data);

  if (constructor === Buffer) {
    return data;
  } else if (constructor === ArrayBuffer) {
    return data.buffer;
  } else if (constructor) {
    return new (constructor as any)(data.buffer, data.byteOffset, value.length);
  } else {
    return data;
  }
}
