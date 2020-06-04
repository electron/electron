/// <reference path="../electron.d.ts" />

 /**
 * This file augments the Electron TS namespace with the internal APIs
 * that are not documented but are used by Electron internally
 */

declare namespace Electron {
  enum ProcessType {
    browser = 'browser',
    renderer = 'renderer',
    worker = 'worker'
  }

  interface App {
    _setDefaultAppPaths(packagePath: string | null): void;
    setVersion(version: string): void;
    setDesktopName(name: string): void;
    setAppPath(path: string | null): void;
  }

  interface ContextBridge {
    internalContextBridge: {
      contextIsolationEnabled: boolean;
      overrideGlobalValueFromIsolatedWorld(keys: string[], value: any): void;
      overrideGlobalValueWithDynamicPropsFromIsolatedWorld(keys: string[], value: any): void;
      overrideGlobalPropertyFromIsolatedWorld(keys: string[], getter: Function, setter?: Function): void;
      isInMainWorld(): boolean;
    }
  }

  interface WebContents {
    _getURL(): string;
    getOwnerBrowserWindow(): Electron.BrowserWindow;
  }

  interface SerializedError {
    message: string;
    stack?: string,
    name: string,
    from: Electron.ProcessType,
    cause: SerializedError,
    __ELECTRON_SERIALIZED_ERROR__: true
  }

  interface ErrorWithCause extends Error {
    from?: string;
    cause?: ErrorWithCause;
  }

  interface InjectionBase {
    url: string;
    code: string
  }

  interface ContentScript {
    js: Array<InjectionBase>;
    css: Array<InjectionBase>;
    runAt: string;
    matches: {
      some: (input: (pattern: string) => boolean | RegExpMatchArray | null) => boolean;
    }
    /**
     * Whether to match all frames, or only the top one.
     * https://developer.chrome.com/extensions/content_scripts#frames
     */
    allFrames: boolean
  }

  type ContentScriptEntry = {
    extensionId: string;
    contentScripts: ContentScript[];
  }

  interface IpcRendererInternal extends Electron.IpcRenderer {
    invoke<T>(channel: string, ...args: any[]): Promise<T>;
    sendToAll(webContentsId: number, channel: string, ...args: any[]): void
  }

  interface WebContentsInternal extends Electron.WebContents {
    _sendInternal(channel: string, ...args: any[]): void;
    _sendInternalToAll(channel: string, ...args: any[]): void;
  }

  const deprecate: ElectronInternal.DeprecationUtil;

  namespace Main {
    const deprecate: ElectronInternal.DeprecationUtil;
  }

  class View {}
}

declare namespace ElectronInternal {
  type DeprecationHandler = (message: string) => void;
  interface DeprecationUtil {
    warnOnce(oldName: string, newName?: string): () => void;
    setHandler(handler: DeprecationHandler | null): void;
    getHandler(): DeprecationHandler | null;
    warn(oldName: string, newName: string): void;
    log(message: string): void;
    removeFunction(fn: Function, removedName: string): Function;
    renameFunction(fn: Function, newName: string | Function): Function;
    event(emitter: NodeJS.EventEmitter, oldName: string, newName: string): void;
    fnToProperty(module: any, prop: string, getter: string, setter?: string): void;
    removeProperty<T, K extends (keyof T & string)>(object: T, propertyName: K, onlyForValues?: any[]): T;
    renameProperty<T, K extends (keyof T & string)>(object: T, oldName: string, newName: K): T;
    moveAPI(fn: Function, oldUsage: string, newUsage: string): Function;
  }

  interface DesktopCapturer {
    startHandling(captureWindow: boolean, captureScreen: boolean, thumbnailSize: Electron.Size, fetchWindowIcons: boolean): void;
    _onerror: (error: string) => void;
    _onfinished: (sources: Electron.DesktopCapturerSource[], fetchWindowIcons: boolean) => void;
  }

  interface GetSourcesOptions {
    captureWindow: boolean;
    captureScreen: boolean;
    thumbnailSize: Electron.Size;
    fetchWindowIcons: boolean;
  }

  interface GetSourcesResult {
    id: string;
    name: string;
    thumbnail: Electron.NativeImage;
    display_id: string;
    appIcon: Electron.NativeImage | null;
  }

  interface KeyWeakMap<K, V> {
    set(key: K, value: V): void;
    get(key: K): V | undefined;
    has(key: K): boolean;
    remove(key: K): void;
  }

  // Internal IPC has _replyInternal and NO reply method
  interface IpcMainInternalEvent extends Omit<Electron.IpcMainEvent, 'reply'> {
    _replyInternal(...args: any[]): void;
  }

  interface IpcMainInternal extends NodeJS.EventEmitter {
    handle(channel: string, listener: (event: Electron.IpcMainInvokeEvent, ...args: any[]) => Promise<any> | any): void;
    on(channel: string, listener: (event: IpcMainInternalEvent, ...args: any[]) => void): this;
    once(channel: string, listener: (event: IpcMainInternalEvent, ...args: any[]) => void): this;
  }

  type ModuleLoader = () => any;

  interface ModuleEntry {
    name: string;
    private?: boolean;
    loader: ModuleLoader;
  }

  interface WebFrameInternal extends Electron.WebFrame {
    getWebFrameId(window: Window): number;
    allowGuestViewElementDefinition(window: Window, context: any): void;
  }

  interface WebFrameResizeEvent extends Electron.Event {
    newWidth: number;
    newHeight: number;
  }

  interface WebViewEvent extends Event {
    url: string;
    isMainFrame: boolean;
  }

  class WebViewElement extends HTMLElement {
    static observedAttributes: Array<string>;

    public contentWindow: Window;

    public connectedCallback(): void;
    public attributeChangedCallback(): void;
    public disconnectedCallback(): void;

    // Created in web-view-impl
    public getWebContentsId(): number;
    public capturePage(rect?: Electron.Rectangle): Promise<Electron.NativeImage>;
  }
}

declare namespace Chrome {
  namespace Tabs {
    // https://developer.chrome.com/extensions/tabs#method-executeScript
    interface ExecuteScriptDetails {
      code?: string;
      file?: string;
      allFrames?: boolean;
      frameId?: number;
      matchAboutBlank?: boolean;
      runAt?: 'document-start' | 'document-end' | 'document_idle';
      cssOrigin: 'author' | 'user';
    }

    type ExecuteScriptCallback = (result: Array<any>) => void;

    // https://developer.chrome.com/extensions/tabs#method-sendMessage
    interface SendMessageDetails {
      frameId?: number;
    }

    type SendMessageCallback = (result: any) => void;
  }
}

interface Global extends NodeJS.Global {
  require: NodeRequire;
  module: NodeModule;
  __filename: string;
  __dirname: string;
}
