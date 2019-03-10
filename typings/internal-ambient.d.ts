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
  }

  interface V8UtilBinding {
    getHiddenValue<T>(obj: any, key: string): T;
    setHiddenValue<T>(obj: any, key: string, value: T): void;
    requestGarbageCollectionForTesting(): void;
  }
  interface Process {
    /**
     * DO NOT USE DIRECTLY, USE process.atomBinding
     */
    _linkedBinding(name: string): any;
    atomBinding(name: string): any;
    atomBinding(name: 'features'): FeaturesBinding;
    atomBinding(name: 'v8_util'): V8UtilBinding;
    atomBinding(name: 'app'): { app: Electron.App, App: Function };
    atomBinding(name: 'command_line'): Electron.CommandLine;
    log: NodeJS.WriteStream['write'];
    activateUvLoop(): void;

    // Additional methods
    getRenderProcessPreferences(): Array<Electron.RendererProcessPreference> | null;

    // Additional events
    once(event: 'document-start', listener: () => any): this;
    once(event: 'document-end', listener: () => any): this;
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