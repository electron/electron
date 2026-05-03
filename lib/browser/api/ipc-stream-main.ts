import { EventEmitter } from 'events';
import { MessageChannelMain, MessagePortMain } from '@electron/internal/browser/message-port-main';
import { ipcMain, WebContents } from 'electron/main';

export interface IpcStreamMessage {
  type: 'data' | 'end' | 'error' | 'ping' | 'pong';
  data?: any;
  error?: string;
  id: string;
}

export interface IpcStreamOptions {
  highWaterMark?: number;
  objectMode?: boolean;
  encoding?: string;
  timeout?: number;
  signal?: AbortSignal;
}

class IpcStreamPort extends EventEmitter {
  private _port: MessagePortMain;
  private _id: string;
  private _isOpen: boolean = true;
  private _buffer: any[] = [];
  private _highWaterMark: number;
  private _objectMode: boolean;

  constructor(port: MessagePortMain, id: string, options: IpcStreamOptions = {}) {
    super();
    this._port = port;
    this._id = id;
    this._highWaterMark = options.highWaterMark || 16;
    this._objectMode = options.objectMode || false;

    this._setupPortListeners();
  }

  private _setupPortListeners(): void {
    this._port.on('message', (event: { data: IpcStreamMessage }) => {
      this._handleMessage(event.data);
    });

    this._port.on('close', () => {
      this._isOpen = false;
      this.emit('close');
    });

    this._port.start();
  }

  private _handleMessage(message: IpcStreamMessage): void {
    if (message.id !== this._id) return;

    switch (message.type) {
      case 'data':
        this._handleData(message.data);
        break;
      case 'end':
        this._handleEnd();
        break;
      case 'error':
        this._handleError(message.error || 'Unknown error');
        break;
      case 'ping':
        this._sendPong();
        break;
      case 'pong':
        this.emit('pong');
        break;
    }
  }

  private _handleData(data: any): void {
    if (!this._objectMode && typeof data === 'string') {
      this._buffer.push(Buffer.from(data));
    } else {
      this._buffer.push(data);
    }

    this.emit('data', data);

    if (this._buffer.length >= this._highWaterMark) {
      this.emit('drain');
    }
  }

  private _handleEnd(): void {
    this._isOpen = false;
    this.emit('end');
  }

  private _handleError(error: string): void {
    this.emit('error', new Error(error));
  }

  private _sendPong(): void {
    this._port.postMessage({
      type: 'pong',
      id: this._id
    } as IpcStreamMessage);
  }

  get id(): string {
    return this._id;
  }

  get isOpen(): boolean {
    return this._isOpen;
  }

  write(data: any): boolean {
    if (!this._isOpen) {
      this.emit('error', new Error('Stream is closed'));
      return false;
    }

    const message: IpcStreamMessage = {
      type: 'data',
      data,
      id: this._id
    };

    this._port.postMessage(message);
    return true;
  }

  end(data?: any): void {
    if (data !== undefined) {
      this.write(data);
    }

    if (!this._isOpen) return;

    const message: IpcStreamMessage = {
      type: 'end',
      id: this._id
    };

    this._port.postMessage(message);
    this._isOpen = false;
    this._port.close();
  }

  error(error: Error): void {
    if (!this._isOpen) return;

    const message: IpcStreamMessage = {
      type: 'error',
      error: error.message,
      id: this._id
    };

    this._port.postMessage(message);
    this._isOpen = false;
    this._port.close();
  }

  ping(): void {
    if (!this._isOpen) return;

    const message: IpcStreamMessage = {
      type: 'ping',
      id: this._id
    };

    this._port.postMessage(message);
  }

  close(): void {
    if (!this._isOpen) return;

    this._isOpen = false;
    this._port.close();
  }
}

class IpcStreamServer extends EventEmitter {
  private _channels: Map<string, (stream: IpcStreamPort) => void> = new Map();
  private _isListening: boolean = false;

  constructor() {
    super();
    this._setupListeners();
  }

  private _setupListeners(): void {
    ipcMain.on('electron-ipc-stream-connect', (event, channel: string, id: string) => {
      this._handleConnection(event, channel, id);
    });
  }

  private _handleConnection(event: any, channel: string, id: string): void {
    const handler = this._channels.get(channel);
    if (!handler) {
      event.senderFrame.postMessage('electron-ipc-stream-reject', { id, error: `Channel '${channel}' not found` });
      return;
    }

    const { port1, port2 } = new MessageChannelMain();
    const stream = new IpcStreamPort(port1, id);

    event.senderFrame.postMessage('electron-ipc-stream-accept', { id }, [port2]);

    handler(stream);
    this.emit('connection', stream, channel);
  }

  listen(channel: string, handler: (stream: IpcStreamPort) => void): void {
    if (this._channels.has(channel)) {
      throw new Error(`Channel '${channel}' is already in use`);
    }

    this._channels.set(channel, handler);
  }

  unlisten(channel: string): void {
    this._channels.delete(channel);
  }

  isListening(channel: string): boolean {
    return this._channels.has(channel);
  }

  close(): void {
    this._channels.clear();
    this._isListening = false;
  }
}

interface PendingConnection {
  resolve: (stream: IpcStreamPort) => void;
  reject: (error: Error) => void;
  options: IpcStreamOptions;
  timeoutTimer?: NodeJS.Timeout;
  abortListener?: () => void;
}

class IpcStreamClient {
  private _pendingConnections: Map<string, PendingConnection> = new Map();
  private _isSetup: boolean = false;
  private _defaultTimeout: number = 10000;

  constructor() {
    this._setupListeners();
  }

  private _setupListeners(): void {
    if (this._isSetup) return;

    ipcMain.on('electron-ipc-stream-accept', (event, { id }) => {
      this._handleAccept(event, id);
    });

    ipcMain.on('electron-ipc-stream-reject', (event, { id, error }) => {
      this._handleReject(id, error);
    });

    this._isSetup = true;
  }

  private _generateId(): string {
    return `stream-${Date.now()}-${Math.random().toString(36).substr(2, 9)}`;
  }

  private _clearConnection(id: string): void {
    const pending = this._pendingConnections.get(id);
    if (!pending) return;

    if (pending.timeoutTimer) {
      clearTimeout(pending.timeoutTimer);
    }

    if (pending.abortListener && pending.options.signal) {
      pending.options.signal.removeEventListener('abort', pending.abortListener);
    }

    this._pendingConnections.delete(id);
  }

  private _handleAccept(event: any, id: string): void {
    const pending = this._pendingConnections.get(id);
    if (!pending) return;

    this._clearConnection(id);

    const port = event.ports[0] as MessagePortMain;
    const stream = new IpcStreamPort(port, id, pending.options);

    pending.resolve(stream);
  }

  private _handleReject(id: string, error: string): void {
    const pending = this._pendingConnections.get(id);
    if (!pending) return;

    this._clearConnection(id);
    pending.reject(new Error(error));
  }

  private _handleTimeout(id: string): void {
    const pending = this._pendingConnections.get(id);
    if (!pending) return;

    this._clearConnection(id);
    pending.reject(new Error(`Connection timeout after ${pending.options.timeout || this._defaultTimeout}ms`));
  }

  private _handleCancel(id: string): void {
    const pending = this._pendingConnections.get(id);
    if (!pending) return;

    this._clearConnection(id);
    pending.reject(new Error('Connection cancelled'));
  }

  connect(webContents: WebContents, channel: string, options: IpcStreamOptions = {}): Promise<IpcStreamPort> {
    return new Promise((resolve, reject) => {
      const id = this._generateId();
      const timeout = options.timeout ?? this._defaultTimeout;

      const pending: PendingConnection = {
        resolve,
        reject,
        options
      };

      if (timeout > 0) {
        pending.timeoutTimer = setTimeout(() => {
          this._handleTimeout(id);
        }, timeout);
      }

      if (options.signal) {
        if (options.signal.aborted) {
          reject(new Error('Connection cancelled'));
          return;
        }

        pending.abortListener = () => {
          this._handleCancel(id);
        };

        options.signal.addEventListener('abort', pending.abortListener, { once: true });
      }

      this._pendingConnections.set(id, pending);
      webContents.postMessage('electron-ipc-stream-connect', { channel, id });
    });
  }

  cancelConnection(id: string): void {
    this._handleCancel(id);
  }

  cancelAllConnections(): void {
    const ids = Array.from(this._pendingConnections.keys());
    for (const id of ids) {
      this._handleCancel(id);
    }
  }

  get pendingConnectionCount(): number {
    return this._pendingConnections.size;
  }
}

export const ipcStreamMain = {
  createServer: (): IpcStreamServer => new IpcStreamServer(),
  createClient: (): IpcStreamClient => new IpcStreamClient(),
  IpcStreamPort,
  IpcStreamServer,
  IpcStreamClient
};

export default ipcStreamMain;
