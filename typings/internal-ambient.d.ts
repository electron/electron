declare var internalBinding: any;

declare namespace NodeJS {
  interface FeaturesBinding {
    isDesktopCapturerEnabled(): boolean;
    isOffscreenRenderingEnabled(): boolean;
    isPDFViewerEnabled(): boolean;
    isRunAsNodeEnabled(): boolean;
    isFakeLocationProviderEnabled(): boolean;
    isViewApiEnabled(): boolean;
    isTtsEnabled(): boolean;
    isPrintingEnabled(): boolean;
    isComponentBuild(): boolean;
  }

  interface V8UtilBinding {
    getHiddenValue<T>(obj: any, key: string): T;
    setHiddenValue<T>(obj: any, key: string, value: T): void;
    deleteHiddenValue(obj: any, key: string): void;
    requestGarbageCollectionForTesting(): void;
  }

  interface NativeImageBinding {
    nativeImage: typeof Electron.NativeImage;
    NativeImage: typeof Electron.NativeImage;
  }

  interface NotificationBinding {
    isSupported(): boolean;
    Notification: typeof Electron.Notification;
  }

  interface PowerMonitor {
    createPowerMonitor(): Electron.PowerMonitor;
    PowerMonitor: Function;
  }

  interface ScreenBinding {
    createScreen(): Electron.Screen;
    Screen: Function;
  }

  interface SessionBinding {
    fromPartition: typeof Electron.Session.fromPartition;
    Cookies: Function;
    Session: Function;
  }

  interface SystemPreferencesBinding {
    systemPreferences: Electron.SystemPreferences;
    SystemPreferences: Function;
  }

  interface Process {
    /**
     * DO NOT USE DIRECTLY, USE process.electronBinding
     */
    _linkedBinding(name: string): any;
    electronBinding(name: string): any;
    electronBinding(name: 'features'): FeaturesBinding;
    electronBinding(name: 'v8_util'): V8UtilBinding;
    electronBinding(name: 'app'): { app: Electron.App, App: Function };
    electronBinding(name: 'browser_view'): { BrowserView: typeof Electron.BrowserView };
    electronBinding(name: 'command_line'): Electron.CommandLine;
    electronBinding(name: 'content_tracing'): Electron.ContentTracing;
    electronBinding(name: 'desktop_capturer'): { createDesktopCapturer(): ElectronInternal.DesktopCapturer };
    electronBinding(name: 'global_shortcut'): { globalShortcut: Electron.GlobalShortcut };
    electronBinding(name: 'native_image'): NativeImageBinding;
    electronBinding(name: 'notification'): NotificationBinding;
    electronBinding(name: 'power_monitor'): PowerMonitor;
    electronBinding(name: 'power_save_blocker'): { powerSaveBlocker: Electron.PowerSaveBlocker };
    electronBinding(name: 'screen'): ScreenBinding;
    electronBinding(name: 'shell'): Electron.Shell;
    electronBinding(name: 'session'): SessionBinding;
    electronBinding(name: 'system_preferences'): SystemPreferencesBinding;
    electronBinding(name: 'tray'): { Tray: Function };

    isRemoteModuleEnabled: boolean;
    log: NodeJS.WriteStream['write'];
    activateUvLoop(): void;

    // Additional events
    once(event: 'document-start', listener: () => any): this;
    once(event: 'document-end', listener: () => any): this;

    // Additional properties
    _firstFileName?: string;
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

declare interface Window {
  ELECTRON_DISABLE_SECURITY_WARNINGS?: boolean;
  ELECTRON_ENABLE_SECURITY_WARNINGS?: boolean;
  InspectorFrontendHost?: {
    showContextMenuAtPoint: (x: number, y: number, items: any[]) => void
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

interface ObjectConstructor {
  /**
   * Returns an object created by key-value entries for properties and methods
   * @param entries An iterable object that contains key-value entries for properties and methods.
   */
  fromEntries<T = any>(entries: Iterable<readonly [PropertyKey, T]>): { [k in PropertyKey]: T };

  /**
   * Returns an object created by key-value entries for properties and methods
   * @param entries An iterable object that contains key-value entries for properties and methods.
   */
  fromEntries(entries: Iterable<readonly any[]>): any;
}
