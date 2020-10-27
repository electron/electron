import type { Size } from 'electron/main';
import type { NativeImage } from 'electron/common';

export type ObjectMember = {
  name: string,
  value?: any,
  enumerable?: boolean,
  writable?: boolean,
  type?: 'method' | 'get'
}

export type ObjProtoDescriptor = {
  members: ObjectMember[],
  proto: ObjProtoDescriptor
} | null

export type MetaType = {
  type: 'object' | 'function',
  name: string,
  members: ObjectMember[],
  proto: ObjProtoDescriptor,
  id: number,
} | {
  type: 'value',
  value: any,
} | {
  type: 'buffer',
  value: Uint8Array,
} | {
  type: 'array',
  members: MetaType[]
} | {
  type: 'error',
  value: Error,
  members: ObjectMember[]
} | {
  type: 'exception',
  value: MetaType,
} | {
  type: 'promise',
  then: MetaType
} | {
  type: 'nativeimage'
  value: NativeImage
}

export type MetaTypeFromRenderer = {
  type: 'value',
  value: any
} | {
  type: 'remote-object',
  id: number
} | {
  type: 'array',
  value: MetaTypeFromRenderer[]
} | {
  type: 'buffer',
  value: Uint8Array
} | {
  type: 'promise',
  then: MetaTypeFromRenderer
} | {
  type: 'object',
  name: string,
  members: {
    name: string,
    value: MetaTypeFromRenderer
  }[]
} | {
  type: 'function-with-return-value',
  value: MetaTypeFromRenderer
} | {
  type: 'function',
  id: number,
  location: string,
  length: number
} | {
  type: 'nativeimage',
  value: {
    size: Size,
    buffer: Buffer,
    scaleFactor: number,
    dataURL: string
  }[]
}
