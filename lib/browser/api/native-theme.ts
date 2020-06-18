import { EventEmitter } from 'events';

const { NativeTheme, nativeTheme } = process.electronBinding('native_theme', 'common');

Object.setPrototypeOf(NativeTheme.prototype, EventEmitter.prototype);
EventEmitter.call(nativeTheme as any);

module.exports = nativeTheme;
