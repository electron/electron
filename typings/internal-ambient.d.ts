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
  }
  interface Process {
    /**
     * DO NOT USE DIRECTLY, USE process.atomBinding
     */
    binding(name: string): any;
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
}
