import { session } from 'electron/main';
import { Readable } from 'stream';
import { ReadableStream } from 'stream/web';

// Global protocol APIs.
const { registerSchemesAsPrivileged, getStandardSchemes, Protocol } = process._linkedBinding('electron_browser_protocol');

const ERR_UNEXPECTED = -9;

const isBuiltInScheme = (scheme: string) => scheme === 'http' || scheme === 'https';

Protocol.prototype.handle = function (this: Electron.Protocol, scheme: string, handler: (req: any) => Promise<any>) {
  const register = isBuiltInScheme(scheme) ? this.interceptProtocol : this.registerProtocol;
  const success = register.call(this, scheme, async (req: any, cb: any) => {
    const res = await handler(req);
    if (!res || typeof res !== 'object') {
      return cb({ error: ERR_UNEXPECTED });
    }
    const { error, body, headers, statusCode } = res;
    cb({
      data: body instanceof ReadableStream ? Readable.fromWeb(body) : body,
      headers: headers instanceof Headers ? Object.fromEntries(headers) : headers,
      error,
      statusCode
    });
  });
  if (!success) throw new Error(`Failed to register protocol: ${scheme}`);
};

Protocol.prototype.unhandle = function (this: Electron.Protocol, scheme: string) {
  const unregister = isBuiltInScheme(scheme) ? this.uninterceptProtocol : this.unregisterProtocol;
  if (!unregister.call(this, scheme)) { throw new Error(`Failed to unhandle protocol: ${scheme}`); }
};

const protocol = {
  registerSchemesAsPrivileged,
  getStandardSchemes,
  registerStringProtocol: (...args) => session.defaultSession.protocol.registerStringProtocol(...args),
  registerBufferProtocol: (...args) => session.defaultSession.protocol.registerBufferProtocol(...args),
  registerStreamProtocol: (...args) => session.defaultSession.protocol.registerStreamProtocol(...args),
  registerFileProtocol: (...args) => session.defaultSession.protocol.registerFileProtocol(...args),
  registerHttpProtocol: (...args) => session.defaultSession.protocol.registerHttpProtocol(...args),
  registerProtocol: (...args) => session.defaultSession.protocol.registerProtocol(...args),
  unregisterProtocol: (...args) => session.defaultSession.protocol.unregisterProtocol(...args),
  isProtocolRegistered: (...args) => session.defaultSession.protocol.isProtocolRegistered(...args),
  interceptStringProtocol: (...args) => session.defaultSession.protocol.interceptStringProtocol(...args),
  interceptBufferProtocol: (...args) => session.defaultSession.protocol.interceptBufferProtocol(...args),
  interceptStreamProtocol: (...args) => session.defaultSession.protocol.interceptStreamProtocol(...args),
  interceptFileProtocol: (...args) => session.defaultSession.protocol.interceptFileProtocol(...args),
  interceptHttpProtocol: (...args) => session.defaultSession.protocol.interceptHttpProtocol(...args),
  interceptProtocol: (...args) => session.defaultSession.protocol.interceptProtocol(...args),
  uninterceptProtocol: (...args) => session.defaultSession.protocol.uninterceptProtocol(...args),
  isProtocolIntercepted: (...args) => session.defaultSession.protocol.isProtocolIntercepted(...args),
  handle: (...args) => session.defaultSession.protocol.handle(...args),
  unhandle: (...args) => session.defaultSession.protocol.unhandle(...args)
} as typeof Electron.protocol;

export default protocol;
