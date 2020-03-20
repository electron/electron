import { app, session } from 'electron';

// Global protocol APIs.
const protocol = process.electronBinding('protocol');

// Fallback protocol APIs of default session.
Object.setPrototypeOf(protocol, new Proxy({}, {
  get (_target, property) {
    if (!app.isReady()) return;

    const protocol = session.defaultSession!.protocol;
    if (!Object.prototype.hasOwnProperty.call(Object.getPrototypeOf(protocol), property)) return;

    // Returning a native function directly would throw error.
    return (...args: any[]) => (protocol[property as keyof Electron.Protocol] as Function)(...args);
  },

  ownKeys () {
    if (!app.isReady()) return [];

    return Object.getOwnPropertyNames(Object.getPrototypeOf(session.defaultSession!.protocol));
  },

  getOwnPropertyDescriptor () {
    return { configurable: true, enumerable: true };
  }
}));

export default protocol;
