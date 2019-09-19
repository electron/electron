import { EventEmitter } from 'events'

const { NativeTheme, nativeTheme } = process.electronBinding('native_theme')

Object.setPrototypeOf(NativeTheme.prototype, EventEmitter.prototype)
EventEmitter.call(nativeTheme as any)

module.exports = nativeTheme
