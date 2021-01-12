/* eslint-disable no-var */
declare var internalBinding: any;
declare var nodeProcess: any;
declare var isolatedWorld: any;
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
    isWinDarkModeWindowUiEnabled(): boolean;
  }

  interface IpcRendererBinding {
    send(internal: boolean, channel: string, args: any[]): void;
    sendSync(internal: boolean, channel: string, args: any[]): any;
    sendToHost(channel: string, args: any[]): void;
    sendTo(internal: boolean, webContentsId: number, channel: string, args: any[]): void;
    invoke<T>(internal: boolean, channel: string, args: any[]): Promise<{ error: string, result: T }>;
    postMessage(channel: string, message: any, transferables: MessagePort[]): void;
  }

  interface V8UtilBinding {
    getHiddenValue<T>(obj: any, key: string): T;
    setHiddenValue<T>(obj: any, key: string, value: T): void;
    deleteHiddenValue(obj: any, key: string): void;
    requestGarbageCollectionForTesting(): void;
    weaklyTrackValue(value: any): void;
    clearWeaklyTrackedValues(): void;
    getWeaklyTrackedValues(): any[];
    addRemoteObjectRef(contextId: string, id: number): void;
    isSameOrigin(a: string, b: string): boolean;
    triggerFatalErrorForTesting(): void;
  }

  interface EnvironmentBinding {
    getVar(name: string): string | null;
    hasVar(name: string): boolean;
    setVar(name: string, value: string): boolean;
    unSetVar(name: string): boolean;
  }

  type AsarFileInfo = {
    size: number;
    unpacked: boolean;
    offset: number;
  };

  type AsarFileStat = {
    size: number;
    offset: number;
    isFile: boolean;
    isDirectory: boolean;
    isLink: boolean;
  }

  interface AsarArchive {
    readonly path: string;
    getFileInfo(path: string): AsarFileInfo | false;
    stat(path: string): AsarFileStat | false;
    readdir(path: string): string[] | false;
    realpath(path: string): string | false;
    copyFileOut(path: string): string | false;
    read(offset: number, size: number): Promise<ArrayBuffer>;
    readSync(offset: number, size: number): ArrayBuffer;
  }

  interface AsarBinding {
    createArchive(path: string): AsarArchive;
    splitPath(path: string): {
      isAsar: false;
    } | {
      isAsar: true;
      asarPath: string;
      filePath: string;
    };
    initAsarSupport(require: NodeJS.Require): void;
  }

  interface PowerMonitorBinding extends Electron.PowerMonitor {
    createPowerMonitor(): PowerMonitorBinding;
    setListeningForShutdown(listening: boolean): void;
  }

  interface WebViewManagerBinding {
    addGuest(guestInstanceId: number, elementInstanceId: number, embedder: Electron.WebContents, guest: Electron.WebContents, webPreferences: Electron.WebPreferences): void;
    removeGuest(embedder: Electron.WebContents, guestInstanceId: number): void;
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
    credentials?: 'include' | 'omit';
    body: Uint8Array | BodyFunc;
    session?: Electron.Session;
    partition?: string;
    referrer?: string;
    origin?: string;
    hasUserActivation?: boolean;
    mode?: string;
    destination?: string;
  };
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
    on(eventName: 'data', listener: (event: any, data: ArrayBuffer, resume: () => void) => void): this;
    on(eventName: 'response-started', listener: (event: any, finalUrl: string, responseHead: ResponseHead) => void): this;
    on(eventName: 'complete', listener: (event: any) => void): this;
    on(eventName: 'error', listener: (event: any, netErrorString: string) => void): this;
    on(eventName: 'login', listener: (event: any, authInfo: Electron.AuthInfo, callback: (username?: string, password?: string) => void) => void): this;
    on(eventName: 'redirect', listener: (event: any, redirectInfo: RedirectInfo, headers: Record<string, string>) => void): this;
    on(eventName: 'upload-progress', listener: (event: any, position: number, total: number) => void): this;
    on(eventName: 'download-progress', listener: (event: any, current: number) => void): this;
  }

  interface Process {
    internalBinding?(name: string): any;
    _linkedBinding(name: string): any;
    _linkedBinding(name: 'electron_common_asar'): AsarBinding;
    _linkedBinding(name: 'electron_common_clipboard'): Electron.Clipboard;
    _linkedBinding(name: 'electron_common_command_line'): Electron.CommandLine;
    _linkedBinding(name: 'electron_common_environment'): EnvironmentBinding;
    _linkedBinding(name: 'electron_common_features'): FeaturesBinding;
    _linkedBinding(name: 'electron_common_native_image'): { nativeImage: typeof Electron.NativeImage };
    _linkedBinding(name: 'electron_common_native_theme'): { nativeTheme: Electron.NativeTheme };
    _linkedBinding(name: 'electron_common_notification'): {
      isSupported(): boolean;
      Notification: typeof Electron.Notification;
    }
    _linkedBinding(name: 'electron_common_screen'): { createScreen(): Electron.Screen };
    _linkedBinding(name: 'electron_common_shell'): Electron.Shell;
    _linkedBinding(name: 'electron_common_v8_util'): V8UtilBinding;
    _linkedBinding(name: 'electron_browser_app'): { app: Electron.App, App: Function };
    _linkedBinding(name: 'electron_browser_auto_updater'): { autoUpdater: Electron.AutoUpdater };
    _linkedBinding(name: 'electron_browser_browser_view'): { BrowserView: typeof Electron.BrowserView };
    _linkedBinding(name: 'electron_browser_crash_reporter'): Omit<Electron.CrashReporter, 'start'> & {
      start(submitUrl: string,
        uploadToServer: boolean,
        ignoreSystemCrashHandler: boolean,
        rateLimit: boolean,
        compress: boolean,
        globalExtra: Record<string, string>,
        extra: Record<string, string>,
        isNodeProcess: boolean): void;
    };
    _linkedBinding(name: 'electron_browser_desktop_capturer'): {
      createDesktopCapturer(): ElectronInternal.DesktopCapturer;
    };
    _linkedBinding(name: 'electron_browser_event'): {
      createWithSender(sender: Electron.WebContents): Electron.Event;
      createEmpty(): Electron.Event;
    };
    _linkedBinding(name: 'electron_browser_event_emitter'): {
      setEventEmitterPrototype(prototype: Object): void;
    };
    _linkedBinding(name: 'electron_browser_global_shortcut'): { globalShortcut: Electron.GlobalShortcut };
    _linkedBinding(name: 'electron_browser_image_view'): { ImageView: any };
    _linkedBinding(name: 'electron_browser_in_app_purchase'): { inAppPurchase: Electron.InAppPurchase };
    _linkedBinding(name: 'electron_browser_message_port'): {
      createPair(): { port1: Electron.MessagePortMain, port2: Electron.MessagePortMain };
    };
    _linkedBinding(name: 'electron_browser_net'): {
      isOnline(): boolean;
      isValidHeaderName: (headerName: string) => boolean;
      isValidHeaderValue: (headerValue: string) => boolean;
      Net: any;
      net: any;
      createURLLoader(options: CreateURLLoaderOptions): URLLoader;
    };
    _linkedBinding(name: 'electron_browser_power_monitor'): PowerMonitorBinding;
    _linkedBinding(name: 'electron_browser_power_save_blocker'): { powerSaveBlocker: Electron.PowerSaveBlocker };
    _linkedBinding(name: 'electron_browser_session'): typeof Electron.Session;
    _linkedBinding(name: 'electron_browser_system_preferences'): { systemPreferences: Electron.SystemPreferences };
    _linkedBinding(name: 'electron_browser_tray'): { Tray: Electron.Tray };
    _linkedBinding(name: 'electron_browser_view'): { View: Electron.View };
    _linkedBinding(name: 'electron_browser_web_contents_view'): { WebContentsView: typeof Electron.WebContentsView };
    _linkedBinding(name: 'electron_browser_web_view_manager'): WebViewManagerBinding;
    _linkedBinding(name: 'electron_renderer_crash_reporter'): Electron.CrashReporter;
    _linkedBinding(name: 'electron_renderer_ipc'): { ipc: IpcRendererBinding };
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

declare module NodeJS {
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

// https://github.com/microsoft/TypeScript/pull/38232

interface WeakRef<T extends object> {
  readonly [Symbol.toStringTag]: 'WeakRef';

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
  readonly [Symbol.toStringTag]: 'FinalizationRegistry';

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
