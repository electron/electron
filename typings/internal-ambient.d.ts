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
    sendTo(internal: boolean, sendToAll: boolean, webContentsId: number, channel: string, args: any[]): void;
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
    triggerFatalErrorForTesting(): void;
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
    getFd(): number | -1;
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
    _linkedBinding(name: string): any;
    _linkedBinding(name: 'electron_renderer_ipc'): { ipc: IpcRendererBinding };
    _linkedBinding(name: 'electron_common_v8_util'): V8UtilBinding;
    _linkedBinding(name: 'electron_common_features'): FeaturesBinding;
    _linkedBinding(name: 'electron_browser_app'): { app: Electron.App, App: Function };
    _linkedBinding(name: 'electron_common_command_line'): Electron.CommandLine;
    _linkedBinding(name: 'electron_browser_desktop_capturer'): {
      createDesktopCapturer(): ElectronInternal.DesktopCapturer;
    };
    _linkedBinding(name: 'electron_browser_net'): {
      isValidHeaderName: (headerName: string) => boolean;
      isValidHeaderValue: (headerValue: string) => boolean;
      Net: any;
      net: any;
      createURLLoader(options: CreateURLLoaderOptions): URLLoader;
    };
    _linkedBinding(name: 'electron_common_asar'): AsarBinding;
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
  trustedTypes: TrustedTypePolicyFactory;
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
  createPolicy(policyName: string, policyOptions: TrustedTypePolicyOptions): TrustedTypePolicy
  isHTML(value: any): boolean;
  isScript(value: any): boolean;
  isScriptURL(value: any): boolean;
  readonly emptyHTML: TrustedHTML;
  readonly emptyScript: TrustedScript;
  getAttributeType(tagName: string, attribute: string, elementNs?: string, attrNs?: string): StringContext | null;
  getPropertyType(tagName: string, property: string, elementNs?: string): StringContext | null;
  readonly defaultPolicy: TrustedTypePolicy | null;
}
