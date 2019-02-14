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
  }
}
