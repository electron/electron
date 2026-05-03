
declare namespace Electron {
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

  export interface IpcStreamPort extends NodeJS.EventEmitter {
    readonly id: string;
    readonly isOpen: boolean;
    
    write(data: any): boolean;
    end(data?: any): void;
    error(error: Error): void;
    ping(): void;
    close(): void;
    
    on(event: 'data', listener: (data: any) => void): this;
    on(event: 'end', listener: () => void): this;
    on(event: 'error', listener: (error: Error) => void): this;
    on(event: 'close', listener: () => void): this;
    on(event: 'drain', listener: () => void): this;
    on(event: 'pong', listener: () => void): this;
    on(event: string | symbol, listener: (...args: any[]) => void): this;
    
    once(event: 'data', listener: (data: any) => void): this;
    once(event: 'end', listener: () => void): this;
    once(event: 'error', listener: (error: Error) => void): this;
    once(event: 'close', listener: () => void): this;
    once(event: 'drain', listener: () => void): this;
    once(event: 'pong', listener: () => void): this;
    once(event: string | symbol, listener: (...args: any[]) => void): this;
  }

  export interface IpcStreamServer extends NodeJS.EventEmitter {
    listen(channel: string, handler: (stream: IpcStreamPort) => void): void;
    unlisten(channel: string): void;
    isListening(channel: string): boolean;
    close(): void;
    
    on(event: 'connection', listener: (stream: IpcStreamPort, channel: string) => void): this;
    on(event: string | symbol, listener: (...args: any[]) => void): this;
    
    once(event: 'connection', listener: (stream: IpcStreamPort, channel: string) => void): this;
    once(event: string | symbol, listener: (...args: any[]) => void): this;
  }

  export interface IpcStreamClient {
    cancelConnection(id: string): void;
    cancelAllConnections(): void;
    readonly pendingConnectionCount: number;
  }

  export interface IpcStreamMainClient extends IpcStreamClient {
    connect(webContents: WebContents, channel: string, options?: IpcStreamOptions): Promise<IpcStreamPort>;
  }

  export interface IpcStreamRendererClient extends IpcStreamClient {
    connect(channel: string, options?: IpcStreamOptions): Promise<IpcStreamPort>;
  }

  export interface IpcStreamMain {
    createServer(): IpcStreamServer;
    createClient(): IpcStreamMainClient;
    IpcStreamPort: new (port: any, id: string, options?: IpcStreamOptions) => IpcStreamPort;
    IpcStreamServer: new () => IpcStreamServer;
    IpcStreamClient: new () => IpcStreamMainClient;
  }

  export interface IpcStreamRenderer {
    createServer(): IpcStreamServer;
    createClient(): IpcStreamRendererClient;
    IpcStreamPort: new (port: any, id: string, options?: IpcStreamOptions) => IpcStreamPort;
    IpcStreamServer: new () => IpcStreamServer;
    IpcStreamClient: new () => IpcStreamRendererClient;
  }
}

declare namespace ElectronInternal {
  // 内部类型定义
}
