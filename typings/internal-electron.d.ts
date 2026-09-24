/// <reference path="../electron.d.ts" />

/**
 * This file augments the Electron TS namespace with the internal APIs
 * that are not documented but are used by Electron internally
 */

declare namespace Electron {
  interface App {
    setVersion(version: string): void;
    setDesktopName(name: string): void;
    setAppPath(path: string | null): void;
  }

  interface AutoUpdater {
    isVersionAllowedForUpdate?(currentVersion: string, targetVersion: string): boolean;
  }

  type TouchBarItemType = NonNullable<Electron.TouchBarConstructorOptions['items']>[0];

  interface BaseWindow {
    _touchBar: Electron.TouchBar | null;
    _setTouchBarItems: (items: TouchBarItemType[]) => void;
    _setEscapeTouchBarItem: (item: TouchBarItemType | {}) => void;
    _refreshTouchBarItem: (itemID: string) => void;
    on(event: '-touch-bar-interaction', listener: (event: Event, itemID: string, details: any) => void): this;
    removeListener(
      event: '-touch-bar-interaction',
      listener: (event: Event, itemID: string, details: any) => void
    ): this;
  }

  interface BrowserWindow extends BaseWindow {
    _init(): void;
    _getWindowButtonVisibility: () => boolean;
    _getAlwaysOnTopLevel: () => string;
    devToolsWebContents: WebContents;
    frameName: string;
    _browserViews: BrowserView[];
    on(event: '-touch-bar-interaction', listener: (event: Event, itemID: string, details: any) => void): this;
    removeListener(
      event: '-touch-bar-interaction',
      listener: (event: Event, itemID: string, details: any) => void
    ): this;
  }

  interface BrowserView {
    ownerWindow: BrowserWindow | null;
    webContentsView: WebContentsView;
  }

  interface BrowserWindowConstructorOptions {
    webContents?: WebContents;
  }

  interface ContextBridge {
    internalContextBridge?: {
      contextIsolationEnabled: boolean;
      overrideGlobalValueFromIsolatedWorld(keys: string[], value: any): void;
      overrideGlobalValueWithDynamicPropsFromIsolatedWorld(keys: string[], value: any): void;
      overrideGlobalPropertyFromIsolatedWorld(keys: string[], getter: Function, setter?: Function): void;
    };
  }

  interface ServiceWorkers {
    _getWorkerFromVersionIDIfExists(versionId: number): Electron.ServiceWorkerMain | undefined;
    _stopAllWorkers(): Promise<void>;
  }

  interface ServiceWorkerMain {
    _countExternalRequests(): number;
  }

  interface Session {
    _init(): void;
  }

  interface TouchBar {
    _removeFromWindow: (win: BaseWindow) => void;
  }

  interface WebContents {
    _setConsoleMessageObserved(observed: boolean): void;
    getOwnerBrowserWindow(): Electron.BrowserWindow | null;
    getLastWebPreferences(): Electron.WebPreferences | null;
    browserWindowOptions: BrowserWindowConstructorOptions;
    _windowOpenHandler: ((details: Electron.HandlerDetails) => any) | null;
    _callWindowOpenHandler(
      event: any,
      details: Electron.HandlerDetails
    ): {
      browserWindowConstructorOptions: Electron.BrowserWindowConstructorOptions | null;
      outlivesOpener: boolean;
      createWindow?: Electron.CreateWindowFunction;
    };
    _setNextChildWebPreferences(
      prefs: Partial<Electron.BrowserWindowConstructorOptions['webPreferences']> &
        Pick<Electron.BrowserWindowConstructorOptions, 'backgroundColor'>
    ): void;
    _sendInternal(channel: string, ...args: any[]): void;
    _executeJavaScript(worldId: number, sources: Electron.WebSource[], hasUserGesture: boolean): Promise<any>;
    _init(): void;
    _getNavigationEntryAtIndex(index: number): Electron.NavigationEntry | null;
    _getActiveIndex(): number;
    _historyLength(): number;
    _canGoBack(): boolean;
    _canGoForward(): boolean;
    _canGoToOffset(index: number): boolean;
    _goBack(): void;
    _goForward(): void;
    _goToOffset(index: number): void;
    _goToIndex(index: number): void;
    _removeNavigationEntryAtIndex(index: number): boolean;
    _getHistory(): Electron.NavigationEntry[];
    _restoreHistory(index: number, entries: Electron.NavigationEntry[]): Promise<void>;
    _clearHistory(): void;
    destroy(): void;
    // <webview>
    attachToIframe(embedderWebContents: Electron.WebContents, embedderFrameToken: string): void;
    detachFromOuterFrame(): void;
    setEmbedder(embedder: Electron.WebContents): void;
    viewInstanceId: number;
    _setOwnerWindow(w: BaseWindow | null): void;
  }

  interface WebFrameMain {
    _transferSharedTexture(transfer: any, textureId: string, args: any[]): Promise<Electron.SharedTextureSyncToken>;
    _lifecycleStateForTesting: string;
  }

  interface WebFrame extends NodeJS.EventEmitter {
    getIsolatedWorlds(): number[];
    on(event: 'isolated-world-created', listener: (worldId: number) => void): this;
    once(event: 'isolated-world-created', listener: (worldId: number) => void): this;
  }

  interface WebPreferences {
    disablePopups?: boolean;
    embedder?: Electron.WebContents;
    openerSandboxFlags?: number;
    type?: 'backgroundPage' | 'window' | 'browserView' | 'remote' | 'webview' | 'offscreen';
  }

  interface Session {
    _setDisplayMediaRequestHandler: Electron.Session['setDisplayMediaRequestHandler'];
    _registerLocalAIHandler(handler: ElectronInternal.UtilityProcessWrapper | null): void;
  }

  type CreateWindowFunction = (options: BrowserWindowConstructorOptions) => WebContents;

  namespace Menu {
    function _roleDefaults(): Record<string, { label: string; accelerator?: string }>;
  }

  interface Menu {
    _activate(commandId: number): void;
    _menuWillShow(): void;
    getItemCount(): number;
    getIndexOfCommandId(commandId: number): number;
    _getAcceleratorTextAt(index: number): string;
  }

  interface MenuItem {
    acceleratorWorksWhenHidden?: boolean;
    getDefaultRoleAccelerator(): Accelerator | undefined;
  }

  interface IpcMainEvent {
    frameTreeNodeId?: number;
  }

  interface IpcMainInvokeEvent {
    frameTreeNodeId?: number;
  }

  // Deprecated / undocumented BrowserWindow methods
  interface BrowserWindow {
    getURL(): string;
    send(channel: string, ...args: any[]): void;
    openDevTools(options?: Electron.OpenDevToolsOptions): void;
    closeDevTools(): void;
    isDevToolsOpened(): void;
    isDevToolsFocused(): void;
    toggleDevTools(): void;
    inspectElement(x: number, y: number): void;
    inspectSharedWorker(): void;
    inspectServiceWorker(): void;
    getBackgroundThrottling(): void;
    setBackgroundThrottling(allowed: boolean): void;
  }

  interface Protocol {
    registerProtocol(scheme: string, handler: any): boolean;
    interceptProtocol(scheme: string, handler: any): boolean;
  }

  interface WebContents {
    on(
      event: '-new-window',
      listener: (
        event: Electron.Event,
        url: string,
        frameName: string,
        disposition: Electron.HandlerDetails['disposition'],
        rawFeatures: string,
        referrer: Electron.Referrer,
        postData: LoadURLOptions['postData'],
        inheritedSandboxFlags: number,
        navigate: (webContents: Electron.WebContents) => void
      ) => void
    ): this;
    on(
      event: '-add-new-contents',
      listener: (
        event: Event,
        webContents: Electron.WebContents,
        disposition: string,
        _userGesture: boolean,
        _left: number,
        _top: number,
        _width: number,
        _height: number,
        url: string,
        frameName: string,
        referrer: Electron.Referrer,
        rawFeatures: string,
        postData: LoadURLOptions['postData']
      ) => void
    ): this;
    on(
      event: '-will-add-new-contents',
      listener: (
        event: Electron.Event,
        url: string,
        frameName: string,
        rawFeatures: string,
        disposition: Electron.HandlerDetails['disposition'],
        referrer: Electron.Referrer,
        postData: LoadURLOptions['postData']
      ) => void
    ): this;
    on(
      event: '-run-dialog',
      listener: (
        info: {
          frame: WebFrameMain;
          dialogType: 'prompt' | 'confirm' | 'alert';
          messageText: string;
          defaultPromptText: string;
        },
        callback: (success: boolean, user_input: string) => void
      ) => void
    ): this;
    on(event: '-cancel-dialogs', listener: () => void): this;
    on(event: 'ready-to-show', listener: () => void): this;
    on(event: '-before-unload-fired', listener: (event: Electron.Event, proceed: boolean) => void): this;

    once(event: 'destroyed', listener: (event: Electron.Event) => void): this;
  }

  interface WebContentsWillFrameNavigateEventParams {
    processId: number;
    routingId: number;
  }
}

declare namespace ElectronInternal {
  interface IpcRendererInternal
    extends NodeJS.EventEmitter, Pick<Electron.IpcRenderer, 'send' | 'sendSync' | 'invoke'> {
    invoke<T>(channel: string, ...args: any[]): Promise<T>;
  }

  type IpcMainInternalEvent = Omit<Electron.IpcMainEvent, 'reply'> | Omit<Electron.IpcMainServiceWorkerEvent, 'reply'>;
  type IpcMainInternalInvokeEvent = Electron.IpcMainInvokeEvent | Electron.IpcMainServiceWorkerInvokeEvent;

  interface IpcMainInternal extends NodeJS.EventEmitter {
    handle(channel: string, listener: (event: IpcMainInternalInvokeEvent, ...args: any[]) => Promise<any> | any): void;
    on(channel: string, listener: (event: IpcMainInternalEvent, ...args: any[]) => void): this;
    once(channel: string, listener: (event: IpcMainInternalEvent, ...args: any[]) => void): this;
  }

  type PageSize = {
    width: number;
    height: number;
  };

  type ModuleLoader = () => any;

  interface ModuleEntry {
    name: string;
    loader: ModuleLoader;
  }

  interface UtilityProcessWrapper extends NodeJS.EventEmitter {
    readonly pid: number | undefined;
    kill(): boolean;
    postMessage(message: any, transfer?: any[]): void;
  }

  interface ParentPort extends NodeJS.EventEmitter {
    start(): void;
    pause(): void;
    postMessage(message: any): void;
  }

  class WebViewElement extends HTMLElement {
    static observedAttributes: Array<string>;

    public contentWindow: Window;

    public connectedCallback?(): void;
    public attributeChangedCallback?(): void;
    public disconnectedCallback?(): void;

    // Created in web-view-impl
    public getWebContentsId(): number;
    public capturePage(rect?: Electron.Rectangle): Promise<Electron.NativeImage>;
  }

  class WebContents extends Electron.WebContents {
    static create(opts?: Electron.WebPreferences): Electron.WebContents;
  }
}
