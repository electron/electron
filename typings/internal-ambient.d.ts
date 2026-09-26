declare const BUILDFLAG: (flag: boolean) => boolean;

declare namespace NodeJS {
  interface ModuleInternal extends NodeJS.Module {
    new (id: string, parent?: NodeJS.Module | null): NodeJS.Module;
    _load(request: string, parent?: NodeJS.Module | null, isMain?: boolean): any;
    _resolveFilename(
      request: string,
      parent?: NodeJS.Module | null,
      isMain?: boolean,
      options?: { paths: string[] }
    ): string;
    _preloadModules(requests: string[]): void;
    _nodeModulePaths(from: string): string[];
    _extensions: Record<string, (module: NodeJS.Module, filename: string) => any>;
    _cache: Record<string, NodeJS.Module>;
    wrapper: [string, string];
  }

  interface FeaturesBinding {
    isBuiltinSpellCheckerEnabled(): boolean;
    isPDFViewerEnabled(): boolean;
    isFakeLocationProviderEnabled(): boolean;
    isPrintingEnabled(): boolean;
    isPromptAPIEnabled(): boolean;
    isComponentBuild(): boolean;
    isRunAsNodeEnabled(): boolean;
  }

  interface IpcRendererBinding {
    ipcRenderer: Electron.IpcRenderer;
    ipcRendererInternal: ElectronInternal.IpcRendererInternal;
  }

  interface V8UtilBinding {
    getHiddenValue<T>(obj: any, key: string): T;
    setHiddenValue<T>(obj: any, key: string, value: T): void;
    requestGarbageCollectionForTesting(): void;
    requestGarbageCollectionForTesting(options: { execution: 'sync' }): void;
    requestGarbageCollectionForTesting(options: { execution: 'async' }): Promise<void>;
    runUntilIdle(): void;
    triggerFatalErrorForTesting(): void;
    exitImmediately(code: number): never;
  }

  type AsarFileInfo = {
    size: number;
    unpacked: boolean;
    offset: number;
    executable: boolean;
    integrity?: {
      algorithm: 'SHA256';
      hash: string;
      blockSize: number;
      blocks: string[];
    };
  };

  type AsarFileStat = {
    size: number;
    offset: number;
    type: number;
    executable: boolean;
  };

  interface AsarArchive {
    getFileInfo(path: string): AsarFileInfo | false;
    stat(path: string): AsarFileStat | false;
    readdir(path: string): string[] | false;
    readdirWithTypes(path: string): [names: string[], types: number[]] | false;
    realpath(path: string): string | false;
    copyFileOut(path: string): string | false;
    getFdAndValidateIntegrityLater(): number | -1;
  }

  interface AsarBinding {
    Archive: { new (path: string): AsarArchive };
    createSentinelFd(): number | -1;
    // Length of the leading part of |path| that names an archive file, -1 if
    // none, or -2 if |requireNormalized| and the path has "."/".."/empty
    // components (normalize and ask again).
    splitPath(path: string, requireNormalized: boolean): number;
  }

  interface NetBinding {
    isOnline(): boolean;
    isValidHeaderName: (headerName: string) => boolean;
    isValidHeaderValue: (headerValue: string) => boolean;
    fileURLToFilePath: (url: string) => string;
    createURLLoader(options: CreateURLLoaderOptions): URLLoader;
    createWebSocket(options: CreateWebSocketOptions): WebSocketWrapper;
    resolveHost(host: string, options?: Electron.ResolveHostOptions): Promise<Electron.ResolvedHost>;
  }

  type CreateWebSocketOptions = {
    url: string;
    protocols?: string[];
    headers?: Record<string, string>;
    origin?: string;
    useSessionCookies?: boolean;
    session?: Electron.Session;
    partition?: string;
  };

  interface WebSocketWrapper extends EventEmitter {
    send(isText: boolean, data: Uint8Array): void;
    close(code?: number, reason?: string): void;
    getBufferedAmount(): number;
    on(eventName: 'open', listener: (event: any, protocol: string, extensions: string) => void): this;
    on(eventName: 'message', listener: (event: any, isText: boolean, data: Buffer) => void): this;
    on(eventName: 'closing', listener: (event: any) => void): this;
    on(eventName: 'close', listener: (event: any, wasClean: boolean, code: number, reason: string) => void): this;
    on(eventName: 'error', listener: (event: any) => void): this;
  }

  interface PowerMonitorBinding extends Electron.PowerMonitor {
    createPowerMonitor(): PowerMonitorBinding;
    setListeningForShutdown(listening: boolean): void;
  }

  interface ServiceWorkerMainBinding {
    ServiceWorkerMain: typeof Electron.ServiceWorkerMain;
  }

  interface SessionBinding {
    fromPartition: typeof Electron.Session.fromPartition;
    fromPath: typeof Electron.Session.fromPath;
    Session: typeof Electron.Session;
  }

  interface WebViewManagerBinding {
    addGuest(
      guestInstanceId: number,
      embedder: Electron.WebContents,
      guest: Electron.WebContents,
      webPreferences: Electron.WebPreferences
    ): void;
    removeGuest(embedder: Electron.WebContents, guestInstanceId: number): void;
  }

  interface WebFrameMainBinding {
    WebFrameMain: typeof Electron.WebFrameMain;
    fromId(processId: number, routingId: number): Electron.WebFrameMain | undefined;
    fromFrameToken(processId: number, frameToken: string): Electron.WebFrameMain | null;
  }

  interface InternalWebPreferences {
    isWebView: boolean;
    hiddenPage: boolean;
    nodeIntegration: boolean;
    webviewTag: boolean;
  }

  interface InternalWebFrame extends Electron.WebFrame {
    getWebPreference<K extends keyof InternalWebPreferences>(name: K): InternalWebPreferences[K];
    _findFrameByWindow(window: Window): Electron.WebFrame | null;
    allowGuestViewElementDefinition(context: object, callback: Function): void;
  }

  interface WebFrameBinding {
    mainFrame: InternalWebFrame;
    WebFrame: Electron.WebFrame;
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
    credentials?: 'include' | 'omit' | 'same-origin';
    body: Uint8Array | BodyFunc;
    session?: Electron.Session;
    partition?: string;
    referrer?: string;
    referrerPolicy?: string;
    cache?: string;
    origin?: string;
    hasUserActivation?: boolean;
    mode?: string;
    destination?: string;
    bypassCustomProtocolHandlers?: boolean;
    priority?: 'throttled' | 'idle' | 'lowest' | 'low' | 'medium' | 'highest';
    priorityIncremental?: boolean;
  };
  type ResponseHead = {
    statusCode: number;
    statusMessage: string;
    httpVersion: { major: number; minor: number };
    rawHeaders: { key: string; value: string }[];
    headers: Record<string, string[]>;
  };

  type RedirectInfo = {
    statusCode: number;
    newMethod: string;
    newUrl: string;
    newSiteForCookies: string;
    newReferrer: string;
    insecureSchemeWasUpgraded: boolean;
    isSignedExchangeFallbackRedirect: boolean;
  };

  interface URLLoader extends EventEmitter {
    cancel(): void;
    hold(): void;
    on(eventName: 'data', listener: (event: any, data: ArrayBuffer, resume: () => void) => void): this;
    on(
      eventName: 'response-started',
      listener: (event: any, finalUrl: string, responseHead: ResponseHead) => void
    ): this;
    on(eventName: 'complete', listener: (event: any) => void): this;
    on(eventName: 'error', listener: (event: any, netErrorString: string) => void): this;
    on(
      eventName: 'login',
      listener: (
        event: any,
        authInfo: Electron.AuthInfo,
        callback: (username?: string, password?: string) => void
      ) => void
    ): this;
    on(
      eventName: 'redirect',
      listener: (event: any, redirectInfo: RedirectInfo, headers: Record<string, string>) => void
    ): this;
    on(eventName: 'upload-progress', listener: (event: any, position: number, total: number) => void): this;
    on(eventName: 'download-progress', listener: (event: any, current: number) => void): this;
  }

  interface Process {
    _linkedBinding(name: string): any;
    _linkedBinding(name: 'electron_common_asar'): AsarBinding;
    _linkedBinding(name: 'electron_common_command_line'): Electron.CommandLine;
    _linkedBinding(name: 'electron_common_crashpad_support'): {
      getCrashdumpSignalFD(): number;
      getCrashpadHandlerPID(): number;
    };
    _linkedBinding(name: 'electron_common_events'): { EventEmitter: typeof import('events').EventEmitter };
    _linkedBinding(name: 'electron_common_features'): FeaturesBinding;
    _linkedBinding(name: 'electron_common_native_image'): { nativeImage: typeof Electron.NativeImage };
    _linkedBinding(name: 'electron_common_shared_texture'): Electron.SharedTextureSubtle & {
      setSharedTextureReceiver: Electron.SharedTexture['setSharedTextureReceiver'];
    };
    _linkedBinding(name: 'electron_common_net'): NetBinding;
    _linkedBinding(name: 'electron_common_shell'): Electron.Shell;
    _linkedBinding(name: 'electron_common_v8_util'): V8UtilBinding;
    _linkedBinding(name: 'electron_browser_app'): {
      app: Electron.App;
      defaultDesktopName(name: string | undefined): string;
    };
    _linkedBinding(name: 'electron_browser_auto_updater'): { autoUpdater: Electron.AutoUpdater };
    _linkedBinding(name: 'electron_browser_clipboard'): Electron.Clipboard;
    _linkedBinding(name: 'electron_browser_clipboard_item'): Electron.ClipboardItem;
    _linkedBinding(name: 'electron_browser_crash_reporter'): Electron.CrashReporter;
    _linkedBinding(name: 'electron_browser_desktop_capturer'): {
      createDesktopCapturer(): ElectronInternal.DesktopCapturer;
      isDisplayMediaSystemPickerAvailable(): boolean;
    };
    _linkedBinding(name: 'electron_browser_event_emitter'): { setEventEmitterPrototype(prototype: Object): void };
    _linkedBinding(name: 'electron_browser_ipc_dispatch'): {
      setup(objects: { ipcMain: NodeJS.EventEmitter; ipcMainInternal: NodeJS.EventEmitter }): void;
    };
    _linkedBinding(name: 'electron_browser_global_shortcut'): { createGlobalShortcut(): Electron.GlobalShortcut };
    _linkedBinding(name: 'electron_browser_image_view'): { ImageView: any };
    _linkedBinding(name: 'electron_browser_in_app_purchase'): { inAppPurchase: Electron.InAppPurchase };
    _linkedBinding(name: 'electron_browser_menu'): { Menu: typeof Electron.Menu; MenuItem: typeof Electron.MenuItem };
    _linkedBinding(name: 'electron_browser_message_port'): { MessageChannelMain: typeof Electron.MessageChannelMain };
    _linkedBinding(name: 'electron_browser_native_theme'): { nativeTheme: Electron.NativeTheme };
    _linkedBinding(name: 'electron_browser_notification'): { Notification: typeof Electron.Notification };
    _linkedBinding(name: 'electron_browser_power_monitor'): PowerMonitorBinding;
    _linkedBinding(name: 'electron_browser_power_save_blocker'): { powerSaveBlocker: Electron.PowerSaveBlocker };
    _linkedBinding(name: 'electron_browser_push_notifications'): { pushNotifications: Electron.PushNotifications };
    _linkedBinding(name: 'electron_browser_safe_storage'): { safeStorage: Electron.SafeStorage };
    _linkedBinding(name: 'electron_browser_session'): SessionBinding;
    _linkedBinding(name: 'electron_browser_screen'): { createScreen(): Electron.Screen };
    _linkedBinding(name: 'electron_browser_service_worker_main'): ServiceWorkerMainBinding;
    _linkedBinding(name: 'electron_browser_system_preferences'): { systemPreferences: Electron.SystemPreferences };
    _linkedBinding(name: 'electron_browser_tray'): { Tray: Electron.Tray };
    _linkedBinding(name: 'electron_browser_view'): { View: typeof Electron.View };
    _linkedBinding(name: 'electron_browser_web_contents_view'): { WebContentsView: typeof Electron.WebContentsView };
    _linkedBinding(name: 'electron_browser_web_view_manager'): WebViewManagerBinding;
    _linkedBinding(name: 'electron_browser_web_frame_main'): WebFrameMainBinding;
    _linkedBinding(name: 'electron_renderer_context_bridge'): {
      contextBridge: Electron.ContextBridge;
    };
    _linkedBinding(name: 'electron_renderer_crash_reporter'): Electron.CrashReporter;
    _linkedBinding(name: 'electron_renderer_ipc'): IpcRendererBinding;
    _linkedBinding(name: 'electron_renderer_web_frame'): WebFrameBinding;
    _linkedBinding(name: 'electron_utility_parent_port'): { createParentPort(): ElectronInternal.ParentPort };
    activateUvLoop(): void;

    helperExecPath: string;

    appCodeLoaded?: () => void;
  }
}

declare interface Window {
  WebView: typeof ElectronInternal.WebViewElement;
  trustedTypes: TrustedTypePolicyFactory;
}

// https://github.com/electron/electron/blob/main/docs/tutorial/message-ports.md#extension-close-event

interface MessagePort {
  onclose: () => void;
}

// https://w3c.github.io/webappsec-trusted-types/dist/spec/#trusted-types

type TrustedHTML = string;
type TrustedScript = string;
type TrustedScriptURL = string;
type TrustedType = TrustedHTML | TrustedScript | TrustedScriptURL;
type StringContext = 'TrustedHTML' | 'TrustedScript' | 'TrustedScriptURL';

// https://w3c.github.io/webappsec-trusted-types/dist/spec/#typedef-trustedtypepolicy

interface TrustedTypePolicy {
  createHTML(input: string, ...arguments: any[]): TrustedHTML;
  createScript(input: string, ...arguments: any[]): TrustedScript;
  createScriptURL(input: string, ...arguments: any[]): TrustedScriptURL;
}

// https://w3c.github.io/webappsec-trusted-types/dist/spec/#typedef-trustedtypepolicyoptions

interface TrustedTypePolicyOptions {
  createHTML?: (input: string, ...arguments: any[]) => TrustedHTML;
  createScript?: (input: string, ...arguments: any[]) => TrustedScript;
  createScriptURL?: (input: string, ...arguments: any[]) => TrustedScriptURL;
}

// https://w3c.github.io/webappsec-trusted-types/dist/spec/#typedef-trustedtypepolicyfactory

interface TrustedTypePolicyFactory {
  createPolicy(policyName: string, policyOptions: TrustedTypePolicyOptions): TrustedTypePolicy;
  isHTML(value: any): boolean;
  isScript(value: any): boolean;
  isScriptURL(value: any): boolean;
  readonly emptyHTML: TrustedHTML;
  readonly emptyScript: TrustedScript;
  getAttributeType(tagName: string, attribute: string, elementNs?: string, attrNs?: string): StringContext | null;
  getPropertyType(tagName: string, property: string, elementNs?: string): StringContext | null;
  readonly defaultPolicy: TrustedTypePolicy | null;
}
