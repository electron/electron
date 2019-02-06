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
  interface Process {
    atomBinding(name: string): any;
    atomBinding(name: 'features'): FeaturesBinding;
    log: NodeJS.WriteStream['write'];
  }
}
