import { EventEmitter } from 'events';

const { NativeTheme, nativeTheme } = process._linkedBinding('electron_common_native_theme');

Object.setPrototypeOf(NativeTheme.prototype, EventEmitter.prototype);
EventEmitter.call(nativeTheme as any);

module.exports = nativeTheme;
