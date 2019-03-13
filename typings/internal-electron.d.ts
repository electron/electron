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
    setVersion(version: string): void;
    setDesktopName(name: string): void;
    setAppPath(path: string | null): void;
  }

  interface SerializedError {
    message: string;
    stack?: string,
    name: string,
    from: Electron.ProcessType,
    cause: SerializedError,
    __ELECTRON_SERIALIZED_ERROR__: true
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

  interface RendererProcessPreference {
    contentScripts: Array<ContentScript>
    extensionId: string;
  }

  interface IpcRendererInternal extends Electron.IpcRenderer {
    sendToAll(webContentsId: number, channel: string, ...args: any[]): void
  }

  interface WebContentsInternal extends Electron.WebContents {
    _sendInternal(channel: string, ...args: any[]): void;
  }

  const deprecate: ElectronInternal.DeprecationUtil;
}

declare namespace ElectronInternal {
  type DeprecationHandler = (message: string) => void;
  interface DeprecationUtil {
    setHandler(handler: DeprecationHandler): void;
    getHandler(): DeprecationHandler | null;
    warn(oldName: string, newName: string): void;
    log(message: string): void;
    function(fn: Function, newName: string): Function;
    event(emitter: NodeJS.EventEmitter, oldName: string, newName: string): void;
    removeProperty<T, K extends (keyof T & string)>(object: T, propertyName: K): T;
    renameProperty<T, K extends (keyof T & string)>(object: T, oldName: string, newName: K): T;

    promisify<T extends (...args: any[]) => any>(fn: T): T;

    // convertPromiseValue: Temporarily disabled until it's used
    promisifyMultiArg<T extends (...args: any[]) => any>(fn: T, /*convertPromiseValue: (v: any) => any*/): T;
  }

  // Internal IPC has _replyInternal and NO reply method
  interface IpcMainInternalEvent extends Omit<Electron.IpcMainEvent, 'reply'> {
    _replyInternal(...args: any[]): void;
  }

  interface IpcMainInternal extends Electron.EventEmitter {
    on(channel: string, listener: (event: IpcMainInternalEvent, ...args: any[]) => void): this;
    once(channel: string, listener: (event: IpcMainInternalEvent, ...args: any[]) => void): this;
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

  abstract class WebViewElement extends HTMLElement {
    static observedAttributes: Array<string>;

    public contentWindow: Window;

    public connectedCallback(): void;
    public attributeChangedCallback(): void;
    public disconnectedCallback(): void;

    // Created in web-view-impl
    public getWebContents(): Electron.WebContents;
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
