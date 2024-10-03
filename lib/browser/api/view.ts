import { EventEmitter } from 'events';

const { View } = process._linkedBinding('electron_browser_view');

Object.setPrototypeOf((View as any).prototype, EventEmitter.prototype);

export default View;
