import { app, session } from 'electron/main';

// Global protocol APIs.
const { registerSchemesAsPrivileged, getStandardSchemes, Protocol } = process._linkedBinding('electron_browser_protocol');

const protocol = {
  registerSchemesAsPrivileged,
  getStandardSchemes
};

Protocol.prototype.handle = (scheme: string, handler: (req: any) => Promise<any>) => {
  this.registerProtocol(scheme, async (req, cb) => {
    const res = await handler(req);
    if (typeof res === 'string') { return cb('string', res); }
    if (ArrayBuffer.isView(res)) { return cb('buffer', res); }
    // TODO: stream
  });
};

// The 'protocol' binding object has a few methods that are global
// (registerSchemesAsPrivileged, getStandardSchemes). The remaining methods are
// proxied to |session.defaultSession.protocol.*|.
Object.setPrototypeOf(protocol, new Proxy({}, {
  get (_target, property) {
    if (!app.isReady()) return;

    const protocol = session.defaultSession!.protocol;
    if (!Object.prototype.hasOwnProperty.call(protocol, property)) return;

    // Returning a native function directly would throw error.
    return (...args: any[]) => (protocol[property as keyof Electron.Protocol] as Function)(...args);
  },

  ownKeys () {
    if (!app.isReady()) return [];
    return Reflect.ownKeys(session.defaultSession!.protocol);
  },

  has: (target, property: string) => {
    if (!app.isReady()) return false;
    return Reflect.has(session.defaultSession!.protocol, property);
  },

  getOwnPropertyDescriptor () {
    return { configurable: true, enumerable: true };
  }
}));

export default protocol;
