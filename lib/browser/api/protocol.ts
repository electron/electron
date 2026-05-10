import { ProtocolRequest, session } from 'electron/main';

import { createReadStream } from 'fs';
import { STATUS_CODES } from 'http';
import { Readable } from 'stream';
import { ReadableStream } from 'stream/web';

import type { ReadableStreamDefaultController, ReadableStreamDefaultReader } from 'stream/web';

// Global protocol APIs.
const { registerSchemesAsPrivileged, getStandardSchemes, Protocol } =
  process._linkedBinding('electron_browser_protocol');

const { createURLLoader } = process._linkedBinding('electron_common_net');

const ERR_FAILED = -2;
const ERR_UNEXPECTED = -9;

const isBuiltInScheme = (scheme: string) => ['http', 'https', 'file'].includes(scheme);

function setResponseMetadata(response: Response, url: string, redirected: boolean) {
  Object.defineProperties(response, {
    url: {
      value: url,
      configurable: true
    },
    redirected: {
      value: redirected,
      configurable: true
    }
  });
}

function makeStreamFromPipe(pipe: any): ReadableStream<Uint8Array> {
  const buf = new Uint8Array(1024 * 1024 /* 1 MB */);
  return new ReadableStream({
    async pull(controller) {
      try {
        const rv = await pipe.read(buf);
        if (rv > 0) {
          controller.enqueue(buf.slice(0, rv));
        } else {
          controller.close();
        }
      } catch (e) {
        controller.error(e);
      }
    }
  });
}

function makeStreamFromFileInfo({
  filePath,
  offset = 0,
  length = -1
}: {
  filePath: string;
  offset?: number;
  length?: number;
}): ReadableStream<Uint8Array> {
  // Node's Readable.toWeb produces a WHATWG ReadableStream whose chunks are Uint8Array.
  return Readable.toWeb(
    createReadStream(filePath, {
      start: offset,
      end: length >= 0 ? offset + length : undefined
    })
  ) as ReadableStream<Uint8Array>;
}

function convertToRequestBody(uploadData: ProtocolRequest['uploadData']): RequestInit['body'] {
  if (!uploadData) return null;
  // Optimization: skip creating a stream if the request is just a single buffer.
  if (uploadData.length === 1 && (uploadData[0] as any).type === 'rawData') {
    return uploadData[0].bytes as any;
  }

  const chunks = [...uploadData] as any[]; // TODO: refine ProtocolRequest types
  // Use Node's web stream types explicitly to avoid DOM lib vs Node lib structural mismatches.
  // Generic <Uint8Array> ensures reader.read() returns value?: Uint8Array consistent with enqueue.
  let current: ReadableStreamDefaultReader<Uint8Array> | null = null;
  return new ReadableStream<Uint8Array>({
    async pull(controller) {
      if (current) {
        const { done, value } = await current.read();
        // (done => value === undefined) as per WHATWG spec
        if (done) {
          current = null;
          return this.pull!(controller);
        } else {
          controller.enqueue(value);
        }
      } else {
        if (!chunks.length) {
          return controller.close();
        }
        const chunk = chunks.shift()!;
        if (chunk.type === 'rawData') {
          controller.enqueue(chunk.bytes as Uint8Array);
        } else if (chunk.type === 'file') {
          current = makeStreamFromFileInfo(chunk).getReader();
          return this.pull!(controller);
        } else if (chunk.type === 'stream') {
          current = makeStreamFromPipe(chunk.body).getReader();
          return this.pull!(controller);
        } else if (chunk.type === 'blob') {
          // Note that even though `getBlobData()` is a `Session` API, it doesn't
          // actually use the `Session` context. Its implementation solely relies
          // on global variables which allows us to implement this feature without
          // knowledge of the `Session` associated with the current request by
          // always pulling `Blob` data out of the default `Session`.
          controller.enqueue(await session.defaultSession.getBlobData(chunk.blobUUID));
        } else {
          throw new Error(`Unknown upload data chunk type: ${chunk.type}`);
        }
      }
    }
  }) as RequestInit['body'];
}

// This isn't a true continuation, but it's a best-effort given upstream constraints.
function continueInterceptedRequest(protocol: Electron.Protocol, req: Request): Promise<Response> {
  const body = req.body;
  const loader = createURLLoader({
    bypassCustomProtocolHandlers: true,
    method: req.method,
    url: req.url,
    session: protocol.session,
    extraHeaders: Object.fromEntries(req.headers),
    referrer: req.referrer === 'about:client' ? '' : req.referrer,
    origin: req.headers.get('origin') ?? '',
    body: body
      ? (pipe) => {
          (async () => {
            const reader = (body as ReadableStream<Uint8Array>).getReader();
            try {
              while (true) {
                const { done, value } = await reader.read();
                if (done) break;
                if (value) await pipe.write(value);
              }
            } catch {
              // Closing the pipe lets the network stack surface the underlying failure.
            } finally {
              reader.releaseLock();
              pipe.done();
            }
          })();
        }
      : undefined
  });

  return new Promise<Response>((resolve, reject) => {
    let responseStarted = false;
    let settled = false;
    let streamFinished = false;
    let controller: ReadableStreamDefaultController<Uint8Array> | null = null;

    const bodyStream = new ReadableStream<Uint8Array>({
      start(streamController) {
        controller = streamController;
      },
      cancel() {
        if (!streamFinished) {
          streamFinished = true;
          loader.cancel();
        }
      }
    });

    const closeBody = () => {
      if (!streamFinished && controller) {
        streamFinished = true;
        controller.close();
      }
    };

    const errorBody = (error: Error) => {
      if (!streamFinished && controller) {
        streamFinished = true;
        controller.error(error);
      }
    };

    const resolveResponse = (response: Response) => {
      if (settled) return;
      settled = true;
      resolve(response);
    };

    const rejectResponse = (error: Error) => {
      if (settled) return;
      settled = true;
      reject(error);
    };

    loader.on('response-started', (event, finalUrl, responseHead) => {
      if (settled) return;
      responseStarted = true;

      const headers = new Headers();
      for (const [key, values] of Object.entries(responseHead.headers)) {
        for (const value of values) {
          headers.append(key, value);
        }
      }

      const nullBodyStatus = [101, 204, 205, 304];
      const body = nullBodyStatus.includes(responseHead.statusCode) || req.method === 'HEAD' ? null : bodyStream;

      const response = new Response(body as any, {
        headers,
        status: responseHead.statusCode,
        statusText: responseHead.statusMessage
      });
      setResponseMetadata(response, finalUrl, finalUrl !== req.url);

      (response as any).__original_resp = {
        _responseHead: responseHead,
        url: finalUrl
      };

      resolveResponse(response);
    });

    loader.on('redirect', (event, redirectInfo, headersObject) => {
      if (settled) return;

      const headers = new Headers();
      const headerEntries: Record<string, string[]> = {};

      for (const [key, value] of Object.entries(headersObject)) {
        const values = Array.isArray(value) ? value : [value];
        headerEntries[key] = values;
        for (const headerValue of values) {
          headers.append(key, headerValue);
        }
      }

      const response = new Response(null, {
        headers,
        status: redirectInfo.statusCode,
        statusText: STATUS_CODES[redirectInfo.statusCode] ?? ''
      });
      setResponseMetadata(response, req.url, false);

      (response as any).__original_resp = {
        _responseHead: {
          statusCode: redirectInfo.statusCode,
          statusMessage: STATUS_CODES[redirectInfo.statusCode] ?? '',
          headers: headerEntries,
          mimeType: ''
        },
        url: req.url
      };

      resolveResponse(response);
      loader.cancel();
    });

    loader.on('data', (event, data, resume) => {
      try {
        if (!streamFinished && controller) {
          controller.enqueue(new Uint8Array(data));
        }
      } finally {
        resume();
      }
    });

    loader.on('complete', () => {
      if (settled && !responseStarted) return;
      if (!responseStarted) {
        rejectResponse(new Error('Response completed before headers were received'));
        return;
      }

      closeBody();
    });

    loader.on('error', (event, netErrorString) => {
      const error = new Error(netErrorString);
      if (settled && !responseStarted) return;
      if (!responseStarted) {
        rejectResponse(error);
        return;
      }

      errorBody(error);
    });
  });
}

function validateResponse(res: Response) {
  if (!res || typeof res !== 'object') return false;

  if (res.type === 'error') return true;

  const exists = (key: string) => Object.hasOwn(res, key);

  if (exists('status') && typeof res.status !== 'number') return false;
  if (exists('statusText') && typeof res.statusText !== 'string') return false;
  if (exists('headers') && typeof res.headers !== 'object') return false;

  if (exists('body')) {
    if (typeof res.body !== 'object') return false;
    if (res.body !== null && !(res.body instanceof ReadableStream)) return false;
  }

  return true;
}

type ProtocolHandler = (req: Request, next: () => Promise<Response>) => Response | Promise<Response>;
type ProtocolHandlerTerminal = (protocol: Electron.Protocol, req: Request) => Response | Promise<Response>;
type ProtocolHandlerChain = {
  handlers: ProtocolHandler[];
  terminal: ProtocolHandlerTerminal;
};

const interceptedSchemeTerminal: ProtocolHandlerTerminal = (protocol, req) => {
  return continueInterceptedRequest(protocol, req);
};

const registeredSchemeTerminal: ProtocolHandlerTerminal = () => {
  throw new Error('next() called inside the last handler for a registered protocol');
};

const chainsByProtocol = new WeakMap<Electron.Protocol, Map<string, ProtocolHandlerChain>>();

function getOrCreateProtocolHandlerChains(protocol: Electron.Protocol): Map<string, ProtocolHandlerChain> {
  const chains = chainsByProtocol.get(protocol);
  if (!chains) {
    chainsByProtocol.set(protocol, new Map<string, ProtocolHandlerChain>());
    return chainsByProtocol.get(protocol)!;
  }
  return chains;
}

function getOrCreateProtocolHandlerChain(protocol: Electron.Protocol, scheme: string): ProtocolHandlerChain {
  const chains = getOrCreateProtocolHandlerChains(protocol);
  const chain = chains.get(scheme);
  if (!chain) {
    chains.set(scheme, {
      handlers: [],
      terminal: isBuiltInScheme(scheme) ? interceptedSchemeTerminal : registeredSchemeTerminal
    });
    return chains.get(scheme)!;
  }
  return chain;
}

async function dispatchProtocolHandlerChain(
  protocol: Electron.Protocol,
  scheme: string,
  req: Request
): Promise<Response> {
  const original = getOrCreateProtocolHandlerChain(protocol, scheme);
  const chain: ProtocolHandlerChain = {
    handlers: [...original.handlers],
    terminal: original.terminal
  };
  async function run(index: number): Promise<Response> {
    if (index > chain.handlers.length) throw new Error('Internal overflow in protocol handler chain');
    if (index === chain.handlers.length) return chain.terminal(protocol, req);
    let called = false;
    return await chain.handlers[index](req, async () => {
      if (called) throw new Error('next() called multiple times in protocol handler chain');
      called = true;
      return await run(index + 1);
    });
  }
  return run(0);
}

Protocol.prototype.handle = function (this: Electron.Protocol, scheme: string, handler: ProtocolHandler) {
  const chains = getOrCreateProtocolHandlerChains(this);
  let chain = chains.get(scheme);
  if (!chain) {
    const handleProtocol = isBuiltInScheme(scheme) ? this.interceptProtocol : this.registerProtocol;
    const success = handleProtocol.call(this, scheme, async (preq: ProtocolRequest, cb: any) => {
      try {
        const body = convertToRequestBody(preq.uploadData);
        const headers = new Headers(preq.headers);
        if (headers.get('origin') === 'null') {
          headers.delete('origin');
        }
        const req = new Request(preq.url, {
          headers,
          method: preq.method,
          referrer: preq.referrer,
          body,
          duplex: body instanceof ReadableStream ? 'half' : undefined
        } as any);
        const res = await dispatchProtocolHandlerChain(this, scheme, req);
        if (!validateResponse(res)) {
          return cb({ error: ERR_UNEXPECTED });
        } else if (res.type === 'error') {
          cb({ error: ERR_FAILED });
        } else {
          const originalHeaders = (res as any).__original_resp?._responseHead?.headers;
          cb({
            data: res.body ? Readable.fromWeb(res.body as ReadableStream<ArrayBufferView>) : null,
            headers: originalHeaders ?? (res.headers ? Object.fromEntries(res.headers) : {}),
            statusCode: res.status,
            statusText: res.statusText,
            mimeType: (res as any).__original_resp?._responseHead?.mimeType
          });
        }
      } catch (e) {
        console.error(e);
        cb({ error: ERR_UNEXPECTED });
      }
    });
    if (!success) throw new Error(`Failed to handle protocol: ${scheme}`);
    chain = getOrCreateProtocolHandlerChain(this, scheme);
  }
  chain.handlers.push(handler);
};

Protocol.prototype.unhandle = function (this: Electron.Protocol, scheme: string) {
  const unhandleProtocol = isBuiltInScheme(scheme) ? this.uninterceptProtocol : this.unregisterProtocol;
  if (!unhandleProtocol.call(this, scheme)) {
    throw new Error(`Failed to unhandle protocol: ${scheme}`);
  }
  getOrCreateProtocolHandlerChains(this).delete(scheme);
};

Protocol.prototype.isProtocolHandled = function (this: Electron.Protocol, scheme: string) {
  const isRegistered = isBuiltInScheme(scheme) ? this.isProtocolIntercepted : this.isProtocolRegistered;
  return isRegistered.call(this, scheme);
};

const protocol = {
  registerSchemesAsPrivileged,
  getStandardSchemes,
  get session() {
    return session.defaultSession;
  },
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
  unhandle: (...args) => session.defaultSession.protocol.unhandle(...args),
  isProtocolHandled: (...args) => session.defaultSession.protocol.isProtocolHandled(...args)
} as typeof Electron.protocol;

export default protocol;
