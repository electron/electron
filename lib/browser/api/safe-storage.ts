import { EventEmitter } from 'events';
const { safeStorage } = process._linkedBinding('electron_browser_safe_storage');

Object.setPrototypeOf(safeStorage, EventEmitter.prototype);

export default safeStorage;
