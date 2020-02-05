declare var internalBinding: any;

declare namespace NodeJS {
  interface FeaturesBinding {
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

  interface IpcBinding {
    send(internal: boolean, channel: string, args: any[]): void;
    sendSync(internal: boolean, channel: string, args: any[]): any;
    sendToHost(channel: string, args: any[]): void;
    sendTo(internal: boolean, sendToAll: boolean, webContentsId: number, channel: string, args: any[]): void;
    invoke<T>(internal: boolean, channel: string, args: any[]): Promise<{ error: string, result: T }>;
  }

  interface V8UtilBinding {
    getHiddenValue<T>(obj: any, key: string): T;
    setHiddenValue<T>(obj: any, key: string, value: T): void;
    deleteHiddenValue(obj: any, key: string): void;
    requestGarbageCollectionForTesting(): void;
    createIDWeakMap<V>(): ElectronInternal.KeyWeakMap<number, V>;
    createDoubleIDWeakMap<V>(): ElectronInternal.KeyWeakMap<[string, number], V>;
    setRemoteCallbackFreer(fn: Function, frameId: number, contextId: String, id: number, sender: any): void
  }

  interface Process {
    /**
     * DO NOT USE DIRECTLY, USE process.electronBinding
     */
    _linkedBinding(name: string): any;
    electronBinding(name: string): any;
    electronBinding(name: 'features'): FeaturesBinding;
    electronBinding(name: 'ipc'): { ipc: IpcBinding };
    electronBinding(name: 'v8_util'): V8UtilBinding;
    electronBinding(name: 'app'): { app: Electron.App, App: Function };
    electronBinding(name: 'command_line'): Electron.CommandLine;
    electronBinding(name: 'desktop_capturer'): { createDesktopCapturer(): ElectronInternal.DesktopCapturer };
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

interface PageSize {
  custom_display_name: string;
  height_microns: number;
  name: string;
  is_default?: string;
  width_microns: number;
}

interface MarginType {
  marginType: 'default' | 'none' | 'printableArea' | 'custom';
  top?: number;
  bottom?: number;
  left?: number;
  right?: number;
}

type PageRange = {from: number, to: number};
type Size = { width: number, height: number};

interface PrintSettings {
  // Sent to Chromium
  bottom?: number;
  collate: boolean;
  color: 1 | 2;
  copies: number;
  deviceName?: string;
  dpiHorizontal: number;
  dpiVertical: number;
  duplex: 0 | 1 | 2;
  generateDraftData: boolean;
  headerFooterEnabled: boolean;
  isFirstRequest: boolean;
  landscape: boolean;
  left?: number;
  marginsType: 0 | 1 | 2 | 3;
  mediaSize: PageSize;
  pageRanges?: PageRange[];
  pagesPerSheet: 1,
  previewModifiable: boolean;
  previewUIID: number;
  printerType?: 0 | 1 | 2 | 3 | 4;
  printWithCloudPrint: boolean;
  printWithExtension: boolean;
  printWithPrivet: boolean;
  rasterizePDF: boolean;
  requestID?: number;
  right?: number;
  scaleFactor: number;
  shouldPrintBackgrounds: boolean;
  shouldPrintSelectionOnly: boolean;
  silent?: boolean;
  title?: string;
  top?: number;
  url?: string;
  // Used internally only
  dpi?: {horizontal: number, vertical: number}
  duplexMode?: 'simplex' | 'longEdge' | 'shortEdge';
  headerFooter?: {title: string, url: string};
  pageSize?: Size | string;
  printBackground?: boolean;
  printSelectionOnly?: boolean;
  printToPDF?: boolean;
  pageRange?: PageRange[];
  margins?: MarginType;
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
