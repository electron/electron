declare var internalBinding: any;
declare var binding: { get: (name: string) => any; process: NodeJS.Process; createPreloadScript: (src: string) => Function };

declare const BUILDFLAG: (flag: boolean) => boolean;

declare const ENABLE_DESKTOP_CAPTURER: boolean;
declare const ENABLE_REMOTE_MODULE: boolean;
declare const ENABLE_VIEWS_API: boolean;

declare namespace NodeJS {
  interface FeaturesBinding {
    isBuiltinSpellCheckerEnabled(): boolean;
    isDesktopCapturerEnabled(): boolean;
    isOffscreenRenderingEnabled(): boolean;
    isRemoteModuleEnabled(): boolean;
    isPDFViewerEnabled(): boolean;
    isRunAsNodeEnabled(): boolean;
    isFakeLocationProviderEnabled(): boolean;
    isViewApiEnabled(): boolean;
    isTtsEnabled(): boolean;
    isPrintingEnabled(): boolean;
    isPictureInPictureEnabled(): boolean;
    isExtensionsEnabled(): boolean;
    isComponentBuild(): boolean;
  }

  interface IpcRendererBinding {
    send(internal: boolean, channel: string, args: any[]): void;
    sendSync(internal: boolean, channel: string, args: any[]): any;
    sendToHost(channel: string, args: any[]): void;
    sendTo(internal: boolean, sendToAll: boolean, webContentsId: number, channel: string, args: any[]): void;
    invoke<T>(internal: boolean, channel: string, args: any[]): Promise<{ error: string, result: T }>;
    postMessage(channel: string, message: any, transferables: MessagePort[]): void;
  }

  interface V8UtilBinding {
    getHiddenValue<T>(obj: any, key: string): T;
    setHiddenValue<T>(obj: any, key: string, value: T): void;
    deleteHiddenValue(obj: any, key: string): void;
    requestGarbageCollectionForTesting(): void;
    createIDWeakMap<V>(): ElectronInternal.KeyWeakMap<number, V>;
    createDoubleIDWeakMap<V>(): ElectronInternal.KeyWeakMap<[string, number], V>;
    setRemoteCallbackFreer(fn: Function, frameId: number, contextId: String, id: number, sender: any): void
    weaklyTrackValue(value: any): void;
    clearWeaklyTrackedValues(): void;
    getWeaklyTrackedValues(): any[];
  }

  type DataPipe = {
    write: (buf: Uint8Array) => Promise<void>;
    done: () => void;
  };
  type BodyFunc = (pipe: DataPipe) => void;
  type CreateURLLoaderOptions = {
    method: string;
    url: string;
    extraHeaders?: Record<string, string>;
    useSessionCookies?: boolean;
    body: Uint8Array | BodyFunc;
    session?: Electron.Session;
    partition?: string;
    referrer?: string;
  }
  type ResponseHead = {
    statusCode: number;
    statusMessage: string;
    httpVersion: { major: number, minor: number };
    rawHeaders: { key: string, value: string }[];
  };

  type RedirectInfo = {
    statusCode: number;
    newMethod: string;
    newUrl: string;
    newSiteForCookies: string;
    newReferrer: string;
    insecureSchemeWasUpgraded: boolean;
    isSignedExchangeFallbackRedirect: boolean;
  }

  interface URLLoader extends EventEmitter {
    cancel(): void;
    on(eventName: 'data', listener: (event: any, data: ArrayBuffer) => void): this;
    on(eventName: 'response-started', listener: (event: any, finalUrl: string, responseHead: ResponseHead) => void): this;
    on(eventName: 'complete', listener: (event: any) => void): this;
    on(eventName: 'error', listener: (event: any, netErrorString: string) => void): this;
    on(eventName: 'login', listener: (event: any, authInfo: Electron.AuthInfo, callback: (username?: string, password?: string) => void) => void): this;
    on(eventName: 'redirect', listener: (event: any, redirectInfo: RedirectInfo, headers: Record<string, string>) => void): this;
    on(eventName: 'upload-progress', listener: (event: any, position: number, total: number) => void): this;
    on(eventName: 'download-progress', listener: (event: any, current: number) => void): this;
  }

  interface Process {
    /**
     * DO NOT USE DIRECTLY, USE process.electronBinding
     */
    _linkedBinding(name: string): any;
    electronBinding(name: string): any;
    electronBinding(name: 'features'): FeaturesBinding;
    electronBinding(name: 'ipc'): { ipc: IpcRendererBinding };
    electronBinding(name: 'v8_util'): V8UtilBinding;
    electronBinding(name: 'app'): { app: Electron.App, App: Function };
    electronBinding(name: 'command_line'): Electron.CommandLine;
    electronBinding(name: 'desktop_capturer'): {
      createDesktopCapturer(): ElectronInternal.DesktopCapturer;
      getMediaSourceIdForWebContents(requestWebContentsId: number, webContentsId: number): string;
    };
    electronBinding(name: 'net'): {
      isValidHeaderName: (headerName: string) => boolean;
      isValidHeaderValue: (headerValue: string) => boolean;
      Net: any;
      net: any;
      createURLLoader(options: CreateURLLoaderOptions): URLLoader;
    };
    log: NodeJS.WriteStream['write'];
    activateUvLoop(): void;

    // Additional events
    once(event: 'document-start', listener: () => any): this;
    once(event: 'document-end', listener: () => any): this;

    // Additional properties
    _firstFileName?: string;

    helperExecPath: string;
    isRemoteModuleEnabled: boolean;
  }
}

declare module NodeJS  {
  interface Global {
    require: NodeRequire;
    module: NodeModule;
    __filename: string;
    __dirname: string;
  }
}

interface ContextMenuItem {
  id: number;
  label: string;
  type: 'normal' | 'separator' | 'subMenu' | 'checkbox';
  checked: boolean;
  enabled: boolean;
  subItems: ContextMenuItem[];
}

declare interface Window {
  ELECTRON_DISABLE_SECURITY_WARNINGS?: boolean;
  ELECTRON_ENABLE_SECURITY_WARNINGS?: boolean;
  InspectorFrontendHost?: {
    showContextMenuAtPoint: (x: number, y: number, items: ContextMenuItem[]) => void
  };
  DevToolsAPI?: {
    contextMenuItemSelected: (id: number) => void;
    contextMenuCleared: () => void
  };
  UI?: {
    createFileSelectorElement: (callback: () => void) => HTMLSpanElement
  };
  Persistence?: {
    FileSystemWorkspaceBinding: {
      completeURL: (project: string, path: string) => string;
    }
  };
  ResizeObserver: ResizeObserver;
}

/**
 * The ResizeObserver interface is used to observe changes to Element's content
 * rect.
 *
 * It is modeled after MutationObserver and IntersectionObserver.
 */
declare class ResizeObserver {
  constructor (callback: ResizeObserverCallback);

  /**
   * Adds target to the list of observed elements.
   */
  observe: (target: Element) => void;

  /**
   * Removes target from the list of observed elements.
   */
  unobserve: (target: Element) => void;

  /**
   * Clears both the observationTargets and activeTargets lists.
   */
  disconnect: () => void;
}

/**
 * This callback delivers ResizeObserver's notifications. It is invoked by a
 * broadcast active observations algorithm.
 */
interface ResizeObserverCallback {
  (entries: ResizeObserverEntry[], observer: ResizeObserver): void;
}

interface ResizeObserverEntry {
  /**
   * @param target The Element whose size has changed.
   */
  new (target: Element): ResizeObserverEntry;

  /**
   * The Element whose size has changed.
   */
  readonly target: Element;

  /**
   * Element's content rect when ResizeObserverCallback is invoked.
   */
  readonly contentRect: DOMRectReadOnly;
}

interface WeakRef<T extends object> {
  readonly [Symbol.toStringTag]: "WeakRef";

  /**
   * Returns the WeakRef instance's target object, or undefined if the target object has been
   * reclaimed.
   */
  deref(): T | undefined;
}

interface WeakRefConstructor {
  readonly prototype: WeakRef<any>;

  /**
   * Creates a WeakRef instance for the given target object.
   * @param target The target object for the WeakRef instance.
   */
  new<T extends object>(target?: T): WeakRef<T>;
}

declare var WeakRef: WeakRefConstructor;

interface FinalizationRegistry {
  readonly [Symbol.toStringTag]: "FinalizationRegistry";

  /**
   * Registers an object with the registry.
   * @param target The target object to register.
   * @param heldValue The value to pass to the finalizer for this object. This cannot be the
   * target object.
   * @param unregisterToken The token to pass to the unregister method to unregister the target
   * object. If provided (and not undefined), this must be an object. If not provided, the target
   * cannot be unregistered.
   */
  register(target: object, heldValue: any, unregisterToken?: object): void;

  /**
   * Unregisters an object from the registry.
   * @param unregisterToken The token that was used as the unregisterToken argument when calling
   * register to register the target object.
   */
  unregister(unregisterToken: object): void;

  /**
   * Triggers callbacks for an implementation-chosen number of objects in the registry that have
   * been reclaimed but whose callbacks have not yet been called.
   *
   * Note: The number of entries for reclaimed objects that are cleaned up from the registry
   * (calling the cleanup callbacks) is implementation-defined. An implementation might remove
   * just one eligible entry, or all eligible entries, or somewhere in between.
   * @param callback If given, the registry uses the given callback instead of the one it was
   * created with.
   */
  cleanupSome?(callback?: (heldValue: any) => void): void;
}

interface FinalizationRegistryConstructor {
  readonly prototype: FinalizationRegistry;

  /**
   * Creates a finalization registry with an associated cleanup callback
   * @param cleanupCallback The callback to call after an object in the registry has been reclaimed.
   */
  new(cleanupCallback: (heldValue: any) => void): FinalizationRegistry;
}

declare var FinalizationRegistry: FinalizationRegistryConstructor;
