import { app } from 'electron/main';

const { createWebSocket } = process._linkedBinding('electron_common_net');

const CONNECTING = 0 as const;
const OPEN = 1 as const;
const CLOSING = 2 as const;
const CLOSED = 3 as const;

type BinaryType = 'blob' | 'arraybuffer' | 'nodebuffer';

const kWrapper = Symbol('wrapper');
const kReadyState = Symbol('readyState');
const kUrl = Symbol('url');
const kProtocol = Symbol('protocol');
const kExtensions = Symbol('extensions');
const kBinaryType = Symbol('binaryType');
const kHandlers = Symbol('handlers');
const kPendingBlobBytes = Symbol('pendingBlobBytes');

// RFC 7230 token (HTTP header value element). Used to validate WebSocket
// subprotocol names per the WHATWG WebSocket spec.
const TOKEN_RE = /^[!#$%&'*+\-.^_`|~0-9a-zA-Z]+$/;

// Node does not expose CloseEvent as a global, so provide a small spec-shaped
// polyfill so `event.code` / `event.reason` / `event.wasClean` are available
// in close handlers exactly as they are in the renderer.
class CloseEvent extends Event {
  readonly code: number;
  readonly reason: string;
  readonly wasClean: boolean;

  constructor(type: string, init: { code?: number; reason?: string; wasClean?: boolean } = {}) {
    super(type);
    this.code = init.code ?? 0;
    this.reason = init.reason ?? '';
    this.wasClean = init.wasClean ?? false;
  }
}

export interface WebSocketOptions {
  protocols?: string | string[];
  headers?: Record<string, string>;
  origin?: string;
  useSessionCookies?: boolean;
  session?: Electron.Session;
  partition?: string;
}

export class WebSocket extends EventTarget {
  static readonly CONNECTING = CONNECTING;
  static readonly OPEN = OPEN;
  static readonly CLOSING = CLOSING;
  static readonly CLOSED = CLOSED;

  readonly CONNECTING = CONNECTING;
  readonly OPEN = OPEN;
  readonly CLOSING = CLOSING;
  readonly CLOSED = CLOSED;

  private [kWrapper]: NodeJS.WebSocketWrapper | null = null;
  private [kReadyState]: number = CONNECTING;
  private [kUrl]: string;
  private [kProtocol]: string = '';
  private [kExtensions]: string = '';
  private [kBinaryType]: BinaryType = 'nodebuffer';
  private [kHandlers] = new Map<string, EventListener | null>();
  private [kPendingBlobBytes] = 0;

  constructor(url: string | URL, protocolsOrOptions?: string | string[] | WebSocketOptions) {
    super();

    if (!app.isReady()) {
      throw new Error('net.WebSocket can only be used after app is ready');
    }

    // Parse the URL per https://websockets.spec.whatwg.org/#dom-websocket-websocket
    let urlRecord: URL;
    try {
      urlRecord = new URL(url);
    } catch {
      throw new DOMException(`Invalid URL: '${url}'`, 'SyntaxError');
    }
    if (urlRecord.protocol === 'http:') urlRecord.protocol = 'ws:';
    else if (urlRecord.protocol === 'https:') urlRecord.protocol = 'wss:';
    if (urlRecord.protocol !== 'ws:' && urlRecord.protocol !== 'wss:') {
      throw new DOMException(
        `The URL's scheme must be either 'ws', 'wss', 'http', or 'https'. '${urlRecord.protocol.slice(0, -1)}' is not allowed.`,
        'SyntaxError'
      );
    }
    if (urlRecord.hash !== '' || urlRecord.href.endsWith('#')) {
      throw new DOMException('The URL contains a fragment identifier, which is not allowed.', 'SyntaxError');
    }
    this[kUrl] = urlRecord.href;

    // Tease apart `protocols` (WHATWG: string | string[]) from the optional
    // Electron options bag. Passing a non-array object as the second argument
    // would otherwise throw per WebIDL, so this is a strict superset.
    let protocols: string[] = [];
    let options: WebSocketOptions = {};
    if (typeof protocolsOrOptions === 'string') {
      protocols = [protocolsOrOptions];
    } else if (Array.isArray(protocolsOrOptions)) {
      protocols = protocolsOrOptions.map(String);
    } else if (protocolsOrOptions != null && typeof protocolsOrOptions === 'object') {
      options = protocolsOrOptions;
      const p = options.protocols;
      if (typeof p === 'string') protocols = [p];
      else if (Array.isArray(p)) protocols = p.map(String);
      else if (p != null) throw new DOMException('protocols must be a string or array of strings', 'SyntaxError');
    } else if (protocolsOrOptions != null) {
      throw new DOMException('protocols must be a string or array of strings', 'SyntaxError');
    }

    const seen = new Set<string>();
    for (const p of protocols) {
      if (!TOKEN_RE.test(p) || seen.has(p)) {
        throw new DOMException(`The subprotocol '${p}' is invalid.`, 'SyntaxError');
      }
      seen.add(p);
    }

    const wrapper = createWebSocket({
      url: urlRecord.href,
      protocols,
      headers: options.headers,
      origin: options.origin,
      useSessionCookies: options.useSessionCookies,
      session: options.session,
      partition: options.partition
    });
    this[kWrapper] = wrapper;

    wrapper.on('open', (_event, protocol, extensions) => {
      this[kReadyState] = OPEN;
      this[kProtocol] = protocol;
      this[kExtensions] = extensions;
      this.dispatchEvent(new Event('open'));
    });

    wrapper.on('message', (_event, isText, data) => {
      if (this[kReadyState] !== OPEN) return;
      let payload: any;
      if (isText) {
        payload = Buffer.from(data).toString('utf8');
      } else if (this[kBinaryType] === 'arraybuffer') {
        payload = data;
      } else if (this[kBinaryType] === 'blob') {
        payload = new Blob([data]);
      } else {
        payload = Buffer.from(data);
      }
      this.dispatchEvent(
        new MessageEvent('message', {
          data: payload,
          origin: new URL(this[kUrl]).origin
        })
      );
    });

    wrapper.on('closing', () => {
      if (this[kReadyState] === OPEN) this[kReadyState] = CLOSING;
    });

    wrapper.on('error', () => {
      // Per spec the 'error' event is a plain Event with no payload.
      this.dispatchEvent(new Event('error'));
    });

    wrapper.on('close', (_event, wasClean, code, reason) => {
      this[kReadyState] = CLOSED;
      this[kWrapper] = null;
      this.dispatchEvent(new CloseEvent('close', { wasClean, code, reason }));
    });
  }

  get [Symbol.toStringTag]() {
    return 'WebSocket';
  }

  get url(): string {
    return this[kUrl];
  }

  get readyState(): number {
    return this[kReadyState];
  }

  get bufferedAmount(): number {
    return (this[kWrapper]?.getBufferedAmount() ?? 0) + this[kPendingBlobBytes];
  }

  get extensions(): string {
    return this[kExtensions];
  }

  get protocol(): string {
    return this[kProtocol];
  }

  get binaryType(): BinaryType {
    return this[kBinaryType];
  }

  set binaryType(value: BinaryType) {
    if (value === 'blob' || value === 'arraybuffer' || value === 'nodebuffer') {
      this[kBinaryType] = value;
    }
  }

  send(data: string | ArrayBufferLike | ArrayBufferView | Blob): void {
    if (this[kReadyState] === CONNECTING) {
      throw new DOMException('Still in CONNECTING state.', 'InvalidStateError');
    }

    if (typeof data === 'string') {
      this[kWrapper]?.send(true, Buffer.from(data, 'utf8'));
    } else if (data instanceof Blob) {
      // Blobs must be read asynchronously. Track their size in bufferedAmount
      // until the bytes have been handed to the network stack.
      this[kPendingBlobBytes] += data.size;
      data.arrayBuffer().then(
        (ab) => {
          this[kPendingBlobBytes] -= data.size;
          this[kWrapper]?.send(false, new Uint8Array(ab));
        },
        () => {
          this[kPendingBlobBytes] -= data.size;
        }
      );
    } else if (ArrayBuffer.isView(data)) {
      this[kWrapper]?.send(false, new Uint8Array(data.buffer, data.byteOffset, data.byteLength));
    } else if (
      data instanceof ArrayBuffer ||
      (typeof SharedArrayBuffer !== 'undefined' && data instanceof SharedArrayBuffer)
    ) {
      this[kWrapper]?.send(false, new Uint8Array(data));
    } else {
      // Per WebIDL, anything else is coerced to a USVString.
      this[kWrapper]?.send(true, Buffer.from(String(data), 'utf8'));
    }
  }

  close(code?: number, reason?: string): void {
    if (code !== undefined) {
      code = Number(code) | 0;
      if (code !== 1000 && (code < 3000 || code > 4999)) {
        throw new DOMException(
          `The close code must be either 1000, or between 3000 and 4999. ${code} is neither.`,
          'InvalidAccessError'
        );
      }
    }
    if (reason !== undefined) {
      reason = String(reason);
      if (Buffer.byteLength(reason, 'utf8') > 123) {
        throw new DOMException('The close reason must not be greater than 123 UTF-8 bytes.', 'SyntaxError');
      }
    }

    if (this[kReadyState] === CLOSING || this[kReadyState] === CLOSED) return;
    this[kReadyState] = CLOSING;
    this[kWrapper]?.close(code, reason);
  }

  // ----- EventHandler IDL attributes -------------------------------------
  get onopen() {
    return this[kHandlers].get('open') ?? null;
  }
  set onopen(fn) {
    setHandler(this, 'open', fn);
  }
  get onmessage() {
    return this[kHandlers].get('message') ?? null;
  }
  set onmessage(fn) {
    setHandler(this, 'message', fn);
  }
  get onerror() {
    return this[kHandlers].get('error') ?? null;
  }
  set onerror(fn) {
    setHandler(this, 'error', fn);
  }
  get onclose() {
    return this[kHandlers].get('close') ?? null;
  }
  set onclose(fn) {
    setHandler(this, 'close', fn);
  }
}

function setHandler(target: WebSocket, type: string, fn: EventListener | null | undefined) {
  const handlers = (target as any)[kHandlers] as Map<string, EventListener | null>;
  const prev = handlers.get(type);
  if (prev) target.removeEventListener(type, prev);
  if (typeof fn === 'function') {
    handlers.set(type, fn);
    target.addEventListener(type, fn);
  } else {
    handlers.delete(type);
  }
}
