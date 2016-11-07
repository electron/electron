// Type definitions for Electron 0.0.6
// Project: http://electron.atom.io/


declare namespace Electron {
  class EventEmitter extends NodeJS.EventEmitter {
    addListener(event: string, listener: Function): this;
    on(event: string, listener: Function): this;
    once(event: string, listener: Function): this;
    removeListener(event: string, listener: Function): this;
    removeAllListeners(event?: string): this;
    setMaxListeners(n: number): this;
    getMaxListeners(): number;
    listeners(event: string): Function[];
    emit(event: string, ...args: any[]): boolean;
    listenerCount(type: string): number;
  }

  class Accelerator extends String {

  }

  /**
   * DownloadItem represents a download item in Electron.
   */
  interface DownloadItem extends NodeJS.EventEmitter {
    /**
    * Emitted when the download has been updated and is not done.
    */
    on(event: 'updated', listener: (event: Event, state: 'progressing' | 'interrupted') => void): this;
    /**
    * Emits when the download is in a terminal state. This includes a completed download,
    * a cancelled download (via downloadItem.cancel()), and interrupted download that can’t be resumed.
    */
    on(event: 'done', listener: (event: Event, state: 'completed' | 'cancelled' | 'interrupted') => void): this;
    on(event: string, listener: Function): this;
    /**
    * Set the save file path of the download item.
    * Note: The API is only available in session’s will-download callback function.
    * If user doesn’t set the save path via the API, Electron will use the original
    * routine to determine the save path (Usually prompts a save dialog).
    */
    setSavePath(path: string): void;
    /**
    * @returns The save path of the download item.
    * This will be either the path set via downloadItem.setSavePath(path) or the path selected from the shown save dialog.
    */
    getSavePath(): string;
    /**
    * Pauses the download.
    */
    pause(): void;
    /**
    * @returns Whether the download is paused.
    */
    isPaused(): boolean;
    /**
    * Resumes the download that has been paused.
    */
    resume(): void;
    /**
    * @returns Whether the download can resume.
    */
    canResume(): boolean;
    /**
    * Cancels the download operation.
    */
    cancel(): void;
    /**
    * @returns The origin url where the item is downloaded from.
    */
    getURL(): string;
    /**
    * @returns The mime type.
    */
    getMimeType(): string;
    /**
    * @returns Whether the download has user gesture.
    */
    hasUserGesture(): boolean;
    /**
    * @returns The file name of the download item.
    * Note: The file name is not always the same as the actual one saved in local disk.
    * If user changes the file name in a prompted download saving dialog,
    * the actual name of saved file will be different.
    */
    getFilename(): string;
    /**
    * @returns The total size in bytes of the download item. If the size is unknown, it returns 0.
    */
    getTotalBytes(): number;
    /**
    * @returns The received bytes of the download item.
    */
    getReceivedBytes(): number;
    /**
    * @returns The Content-Disposition field from the response header.
    */
    getContentDisposition(): string;
    /**
    * @returns The current state.
    */
    getState(): 'progressing' | 'completed' | 'cancelled' | 'interrupted';
  }

  interface Event {
    preventDefault: Function;
    sender: WebContents;
    returnValue?: any;
  }

  interface CommonInterface {
    clipboard: Electron.Clipboard;
    crashReporter: Electron.CrashReporter;
    nativeImage: typeof Electron.NativeImage;
    NativeImage: typeof Electron.NativeImage;
    shell: Electron.Shell;
    screen: Electron.Screen;
    BluetoothDevice: Electron.BluetoothDevice;
    Certificate: Electron.Certificate;
    Cookie: Electron.Cookie;
    DesktopCapturerSource: Electron.DesktopCapturerSource;
    Display: Electron.Display;
    JumpListCategory: Electron.JumpListCategory;
    JumpListItem: Electron.JumpListItem;
    MemoryUsageDetails: Electron.MemoryUsageDetails;
    Rectangle: Electron.Rectangle;
    ShortcutDetails: Electron.ShortcutDetails;
    Task: Electron.Task;
    ThumbarButton: Electron.ThumbarButton;
    UploadData: Electron.UploadData;
  }

  interface MainInterface extends CommonInterface {
    app: Electron.App;
    autoUpdater: Electron.AutoUpdater;
    BrowserWindow: typeof Electron.BrowserWindow;
    contentTracing: Electron.ContentTracing;
    dialog: Electron.Dialog;
    ipcMain: Electron.IpcMain;
    globalShortcut: Electron.GlobalShortcut;
    Menu: typeof Electron.Menu;
    net: Electron.Net;
    ClientRequest: typeof Electron.ClientRequest;
    IncomingMessage: typeof Electron.IncomingMessage;
    MenuItem: typeof Electron.MenuItem;
    powerMonitor: Electron.PowerMonitor;
    powerSaveBlocker: Electron.PowerSaveBlocker;
    protocol: Electron.Protocol;
    session: typeof Electron.Session;
    Session: typeof Electron.Session;
    Cookies: typeof Electron.Cookies;
    WebRequest: typeof Electron.WebRequest;
    systemPreferences: Electron.SystemPreferences;
    Tray: typeof Electron.Tray;
    webContents: typeof Electron.WebContents;
    WebContents: typeof Electron.WebContents;
    Debugger: typeof Electron.Debugger;
    process: Electron.Process;
  }

  interface RendererInterface extends CommonInterface {
    desktopCapturer: Electron.DesktopCapturer;
    ipcRenderer: Electron.IpcRenderer;
    remote: Electron.Remote;
    webFrame: Electron.WebFrame;
    BrowserWindowProxy: typeof Electron.BrowserWindowProxy;
  }

  interface AllElectron {
    clipboard: Electron.Clipboard;
    crashReporter: Electron.CrashReporter;
    nativeImage: typeof Electron.NativeImage;
    shell: Electron.Shell;
    app: Electron.App;
    autoUpdater: Electron.AutoUpdater;
    BrowserWindow: typeof Electron.BrowserWindow;
    contentTracing: Electron.ContentTracing;
    dialog: Electron.Dialog;
    ipcMain: Electron.IpcMain;
    globalShortcut: Electron.GlobalShortcut;
    Menu: typeof Electron.Menu;
    net: Electron.Net;
    ClientRequest: typeof Electron.ClientRequest;
    IncomingMessage: typeof Electron.IncomingMessage;
    MenuItem: typeof Electron.MenuItem;
    powerMonitor: Electron.PowerMonitor;
    powerSaveBlocker: Electron.PowerSaveBlocker;
    protocol: Electron.Protocol;
    screen: Electron.Screen;
    session: typeof Electron.Session;
    Cookies: typeof Electron.Cookies;
    WebRequest: typeof Electron.WebRequest;
    systemPreferences: Electron.SystemPreferences;
    Tray: typeof Electron.Tray;
    webContents: typeof Electron.WebContents;
    Debugger: typeof Electron.Debugger;
    process: Electron.Process;
    desktopCapturer: Electron.DesktopCapturer;
    ipcRenderer: Electron.IpcRenderer;
    remote: Electron.Remote;
    webFrame: Electron.WebFrame;
    BrowserWindowProxy: typeof Electron.BrowserWindowProxy;
    BluetoothDevice: Electron.BluetoothDevice;
    Certificate: Electron.Certificate;
    Cookie: Electron.Cookie;
    DesktopCapturerSource: Electron.DesktopCapturerSource;
    Display: Electron.Display;
    JumpListCategory: Electron.JumpListCategory;
    JumpListItem: Electron.JumpListItem;
    MemoryUsageDetails: Electron.MemoryUsageDetails;
    Rectangle: Electron.Rectangle;
    ShortcutDetails: Electron.ShortcutDetails;
    Task: Electron.Task;
    ThumbarButton: Electron.ThumbarButton;
    UploadData: Electron.UploadData;
  }

  interface App extends EventEmitter {

    // Docs: http://electron.atom.io/docs/api/app

    /**
     * Emitted when Chrome's accessibility support changes. This event fires when
     * assistive technologies, such as screen readers, are enabled or disabled. See
     * https://www.chromium.org/developers/design-documents/accessibility for more
     * details.
     */
    on(event: 'accessibility-support-changed', listener: (event: Event,
                                                          /**
                                                           * `true` when Chrome's accessibility support is enabled, `false` otherwise.
                                                           */
                                                          accessibilitySupportEnabled: boolean) => void): this;
    /**
     * Emitted when the application is activated, which usually happens when the user
     * clicks on the application's dock icon.
     */
    on(event: 'activate', listener: (event: Event,
                                     hasVisibleWindows: boolean) => void): this;
    /**
     * Emitted before the application starts closing its windows. Calling
     * event.preventDefault() will prevent the default behaviour, which is terminating
     * the application.
     */
    on(event: 'before-quit', listener: (event: Event) => void): this;
    /**
     * Emitted when a browserWindow gets blurred.
     */
    on(event: 'browser-window-blur', listener: (event: Event,
                                                window: BrowserWindow) => void): this;
    /**
     * Emitted when a new browserWindow is created.
     */
    on(event: 'browser-window-created', listener: (event: Event,
                                                   window: BrowserWindow) => void): this;
    /**
     * Emitted when a browserWindow gets focused.
     */
    on(event: 'browser-window-focus', listener: (event: Event,
                                                 window: BrowserWindow) => void): this;
    /**
     * Emitted when failed to verify the certificate for url, to trust the certificate
     * you should prevent the default behavior with event.preventDefault() and call
     * callback(true).
     */
    on(event: 'certificate-error', listener: (event: Event,
                                              webContents: WebContents,
                                              url: URL,
                                              /**
                                               * The error code
                                               */
                                              error: string,
                                              certificate: Certificate,
                                              callback: Function) => void): this;
    /**
     * Emitted during Handoff when an activity from a different device wants to be
     * resumed. You should call event.preventDefault() if you want to handle this
     * event. A user activity can be continued only in an app that has the same
     * developer Team ID as the activity's source app and that supports the activity's
     * type. Supported activity types are specified in the app's Info.plist under the
     * NSUserActivityTypes key.
     */
    on(event: 'continue-activity', listener: (event: Event,
                                              /**
                                               * A string identifying the activity. Maps to .
                                               */
                                              type: string,
                                              /**
                                               * Contains app-specific state stored by the activity on another device.
                                               */
                                              userInfo: any) => void): this;
    /**
     * Emitted when the gpu process crashes or is killed.
     */
    on(event: 'gpu-process-crashed', listener: (event: Event,
                                                killed: boolean) => void): this;
    /**
     * Emitted when webContents wants to do basic auth. The default behavior is to
     * cancel all authentications, to override this you should prevent the default
     * behavior with event.preventDefault() and call callback(username, password) with
     * the credentials.
     */
    on(event: 'login', listener: (event: Event,
                                  webContents: WebContents,
                                  request: Request,
                                  authInfo: AuthInfo,
                                  callback: Function) => void): this;
    /**
     * Emitted when the user wants to open a file with the application. The open-file
     * event is usually emitted when the application is already open and the OS wants
     * to reuse the application to open the file. open-file is also emitted when a file
     * is dropped onto the dock and the application is not yet running. Make sure to
     * listen for the open-file event very early in your application startup to handle
     * this case (even before the ready event is emitted). You should call
     * event.preventDefault() if you want to handle this event. On Windows, you have to
     * parse process.argv (in the main process) to get the filepath.
     */
    on(event: 'open-file', listener: (event: Event,
                                      path: string) => void): this;
    /**
     * Emitted when the user wants to open a URL with the application. The URL scheme
     * must be registered to be opened by your application. You should call
     * event.preventDefault() if you want to handle this event.
     */
    on(event: 'open-url', listener: (event: Event,
                                     url: string) => void): this;
    /**
     * Emitted when the application is quitting.
     */
    on(event: 'quit', listener: (event: Event,
                                 exitCode: number) => void): this;
    /**
     * Emitted when Electron has finished initializing. On macOS, launchInfo holds the
     * userInfo of the NSUserNotification that was used to open the application, if it
     * was launched from Notification Center. You can call app.isReady() to check if
     * this event has already fired.
     */
    on(event: 'ready', listener: (launchInfo: any) => void): this;
    /**
     * Emitted when a client certificate is requested. The url corresponds to the
     * navigation entry requesting the client certificate and callback needs to be
     * called with an entry filtered from the list. Using event.preventDefault()
     * prevents the application from using the first certificate from the store.
     */
    on(event: 'select-client-certificate', listener: (event: Event,
                                                      webContents: WebContents,
                                                      url: URL,
                                                      certificateList: Certificate[],
                                                      callback: Function) => void): this;
    /**
     * Emitted when a new webContents is created.
     */
    on(event: 'web-contents-created', listener: (event: Event,
                                                 webContents: WebContents) => void): this;
    /**
     * Emitted when the application has finished basic startup. On Windows and Linux,
     * the will-finish-launching event is the same as the ready event; on macOS, this
     * event represents the applicationWillFinishLaunching notification of
     * NSApplication. You would usually set up listeners for the open-file and open-url
     * events here, and start the crash reporter and auto updater. In most cases, you
     * should just do everything in the ready event handler.
     */
    on(event: 'will-finish-launching', listener: Function): this;
    /**
     * Emitted when all windows have been closed and the application will quit. Calling
     * event.preventDefault() will prevent the default behaviour, which is terminating
     * the application. See the description of the window-all-closed event for the
     * differences between the will-quit and window-all-closed events.
     */
    on(event: 'will-quit', listener: (event: Event) => void): this;
    /**
     * Emitted when all windows have been closed. If you do not subscribe to this event
     * and all windows are closed, the default behavior is to quit the app; however, if
     * you subscribe, you control whether the app quits or not. If the user pressed Cmd
     * + Q, or the developer called app.quit(), Electron will first try to close all
     * the windows and then emit the will-quit event, and in this case the
     * window-all-closed event would not be emitted.
     */
    on(event: 'window-all-closed', listener: Function): this;
    /**
     * Adds path to the recent documents list. This list is managed by the OS. On
     * Windows you can visit the list from the task bar, and on macOS you can visit it
     * from dock menu.
     */
    addRecentDocument(path: string): void;
    /**
     * Append an argument to Chromium's command line. The argument will be quoted
     * correctly. Note: This will not affect process.argv.
     */
    appendArgument(value: string): void;
    /**
     * Append a switch (with optional value) to Chromium's command line. Note: This
     * will not affect process.argv, and is mainly used by developers to control some
     * low-level Chromium behaviors.
     */
    appendSwitch(the_switch: string, value?: string): void;
    /**
     * When critical is passed, the dock icon will bounce until either the application
     * becomes active or the request is canceled. When informational is passed, the
     * dock icon will bounce for one second. However, the request remains active until
     * either the application becomes active or the request is canceled. Returns an ID
     * representing the request.
     */
    bounce(type?: 'critical' | 'informational'): void;
    /**
     * Cancel the bounce of id.
     */
    cancelBounce(id: number): void;
    /**
     * Clears the recent documents list.
     */
    clearRecentDocuments(): void;
    /**
     * Disables hardware acceleration for current app. This method can only be called
     * before app is ready.
     */
    disableHardwareAcceleration(): void;
    /**
     * Bounces the Downloads stack if the filePath is inside the Downloads folder.
     */
    downloadFinished(filePath: string): void;
    /**
     * Exits immediately with exitCode.  exitCode defaults to 0. All windows will be
     * closed immediately without asking user and the before-quit and will-quit events
     * will not be emitted.
     */
    exit(exitCode?: number): void;
    /**
     * On Linux, focuses on the first visible window. On macOS, makes the application
     * the active app. On Windows, focuses on the application's first window.
     */
    focus(): void;
    getAppPath(): string;
    getBadge(): string;
    getBadgeCount(): number;
    getCurrentActivityType(): string;
    getJumpListSettings(): JumpListSettings;
    /**
     * Note: When distributing your packaged app, you have to also ship the locales
     * folder. Note: On Windows you have to call it after the ready events gets
     * emitted.
     */
    getLocale(): string;
    /**
     * Note: This API has no effect on MAS builds.
     */
    getLoginItemSettings(): LoginItemSettings;
    /**
     * Usually the name field of package.json is a short lowercased name, according to
     * the npm modules spec. You should usually also specify a productName field, which
     * is your application's full capitalized name, and which will be preferred over
     * name by Electron.
     */
    getName(): string;
    /**
     * You can request the following paths by the name:
     */
    getPath(name: string): string;
    getVersion(): string;
    /**
     * Hides all application windows without minimizing them.
     */
    hide(): void;
    /**
     * Hides the dock icon.
     */
    hide(): void;
    /**
     * Imports the certificate in pkcs12 format into the platform certificate store.
     * callback is called with the result of import operation, a value of 0 indicates
     * success while any other value indicates failure according to chromium
     * net_error_list.
     */
    importCertificate(options: ImportCertificateOptions, callback: (result: number) => void): void;
    isAccessibilitySupportEnabled(): boolean;
    /**
     * This method checks if the current executable is the default handler for a
     * protocol (aka URI scheme). If so, it will return true. Otherwise, it will return
     * false. Note: On macOS, you can use this method to check if the app has been
     * registered as the default protocol handler for a protocol. You can also verify
     * this by checking ~/Library/Preferences/com.apple.LaunchServices.plist on the
     * macOS machine. Please refer to Apple's documentation for details. The API uses
     * the Windows Registry and LSCopyDefaultHandlerForURLScheme internally.
     */
    isDefaultProtocolClient(protocol: string, path?: string, args?: string[]): boolean;
    isReady(): boolean;
    isUnityRunning(): boolean;
    isVisible(): boolean;
    /**
     * This method makes your application a Single Instance Application - instead of
     * allowing multiple instances of your app to run, this will ensure that only a
     * single instance of your app is running, and other instances signal this instance
     * and exit. callback will be called with callback(argv, workingDirectory) when a
     * second instance has been executed. argv is an Array of the second instance's
     * command line arguments, and workingDirectory is its current working directory.
     * Usually applications respond to this by making their primary window focused and
     * non-minimized. The callback is guaranteed to be executed after the ready event
     * of app gets emitted. This method returns false if your process is the primary
     * instance of the application and your app should continue loading. And returns
     * true if your process has sent its parameters to another instance, and you should
     * immediately quit. On macOS the system enforces single instance automatically
     * when users try to open a second instance of your app in Finder, and the
     * open-file and open-url events will be emitted for that. However when users start
     * your app in command line the system's single instance mechanism will be bypassed
     * and you have to use this method to ensure single instance. An example of
     * activating the window of primary instance when a second instance starts:
     */
    makeSingleInstance(callback: (argv: string[], workingDirectory: string) => void): void;
    /**
     * Try to close all windows. The before-quit event will be emitted first. If all
     * windows are successfully closed, the will-quit event will be emitted and by
     * default the application will terminate. This method guarantees that all
     * beforeunload and unload event handlers are correctly executed. It is possible
     * that a window cancels the quitting by returning false in the beforeunload event
     * handler.
     */
    quit(): void;
    /**
     * Relaunches the app when current instance exits. By default the new instance will
     * use the same working directory and command line arguments with current instance.
     * When args is specified, the args will be passed as command line arguments
     * instead. When execPath is specified, the execPath will be executed for relaunch
     * instead of current app. Note that this method does not quit the app when
     * executed, you have to call app.quit or app.exit after calling app.relaunch to
     * make the app restart. When app.relaunch is called for multiple times, multiple
     * instances will be started after current instance exited. An example of
     * restarting current instance immediately and adding a new command line argument
     * to the new instance:
     */
    relaunch(options?: RelaunchOptions): void;
    /**
     * Releases all locks that were created by makeSingleInstance. This will allow
     * multiple instances of the application to once again run side by side.
     */
    releaseSingleInstance(): void;
    /**
     * This method checks if the current executable as the default handler for a
     * protocol (aka URI scheme). If so, it will remove the app as the default handler.
     */
    removeAsDefaultProtocolClient(protocol: string, path?: string, args?: string[]): boolean;
    /**
     * Set the about panel options. This will override the values defined in the app's
     * .plist file. See the Apple docs for more details.
     */
    setAboutPanelOptions(options: AboutPanelOptionsOptions): void;
    /**
     * Changes the Application User Model ID to id.
     */
    setAppUserModelId(id: string): void;
    /**
     * This method sets the current executable as the default handler for a protocol
     * (aka URI scheme). It allows you to integrate your app deeper into the operating
     * system. Once registered, all links with your-protocol:// will be opened with the
     * current executable. The whole link, including protocol, will be passed to your
     * application as a parameter. On Windows you can provide optional parameters path,
     * the path to your executable, and args, an array of arguments to be passed to
     * your executable when it launches. Note: On macOS, you can only register
     * protocols that have been added to your app's info.plist, which can not be
     * modified at runtime. You can however change the file with a simple text editor
     * or script during build time. Please refer to Apple's documentation for details.
     * The API uses the Windows Registry and LSSetDefaultHandlerForURLScheme
     * internally.
     */
    setAsDefaultProtocolClient(protocol: string, path?: string, args?: string[]): boolean;
    /**
     * Sets the string to be displayed in the dock’s badging area.
     */
    setBadge(text: string): void;
    /**
     * Sets the counter badge for current app. Setting the count to 0 will hide the
     * badge. On macOS it shows on the dock icon. On Linux it only works for Unity
     * launcher, Note: Unity launcher requires the exsistence of a .desktop file to
     * work, for more information please read Desktop Environment Integration.
     */
    setBadgeCount(count: number): boolean;
    /**
     * Sets the image associated with this dock icon.
     */
    setIcon(image: NativeImage): void;
    /**
     * Sets or removes a custom Jump List for the application, and returns one of the
     * following strings: If categories is null the previously set custom Jump List (if
     * any) will be replaced by the standard Jump List for the app (managed by
     * Windows). JumpListCategory objects should have the following properties: Note:
     * If a JumpListCategory object has neither the type nor the name property set then
     * its type is assumed to be tasks. If the name property is set but the type
     * property is omitted then the type is assumed to be custom. Note: Users can
     * remove items from custom categories, and Windows will not allow a removed item
     * to be added back into a custom category until after the next successful call to
     * app.setJumpList(categories). Any attempt to re-add a removed item to a custom
     * category earlier than that will result in the entire custom category being
     * omitted from the Jump List. The list of removed items can be obtained using
     * app.getJumpListSettings(). JumpListItem objects should have the following
     * properties: Here's a very simple example of creating a custom Jump List:
     */
    setJumpList(categories: JumpListCategory[]): void;
    /**
     * Set the app's login item settings. Note: This API has no effect on MAS builds.
     */
    setLoginItemSettings(settings: Settings): void;
    /**
     * Sets the application's dock menu.
     */
    setMenu(menu: Menu): void;
    /**
     * Overrides the current application's name.
     */
    setName(name: string): void;
    /**
     * Overrides the path to a special directory or file associated with name. If the
     * path specifies a directory that does not exist, the directory will be created by
     * this method. On failure an Error is thrown. You can only override paths of a
     * name defined in app.getPath. By default, web pages' cookies and caches will be
     * stored under the userData directory. If you want to change this location, you
     * have to override the userData path before the ready event of the app module is
     * emitted.
     */
    setPath(name: string, path: string): void;
    /**
     * Creates an NSUserActivity and sets it as the current activity. The activity is
     * eligible for Handoff to another device afterward.
     */
    setUserActivity(type: string, userInfo: any, webpageURL: string): void;
    /**
     * Adds tasks to the Tasks category of the JumpList on Windows. tasks is an array
     * of Task objects in the following format: Task Object: Note: If you'd like to
     * customize the Jump List even more use app.setJumpList(categories) instead.
     */
    setUserTasks(tasks: Task[]): boolean;
    /**
     * Shows the dock icon.
     */
    show(): void;
    /**
     * Shows application windows after they were hidden. Does not automatically focus
     * them.
     */
    show(): void;
  }

  interface AutoUpdater extends EventEmitter {

    // Docs: http://electron.atom.io/docs/api/auto-updater

    /**
     * Emitted when checking if an update has started.
     */
    on(event: 'checking-for-update', listener: Function): this;
    /**
     * Emitted when there is an error while updating.
     */
    on(event: 'error', listener: (error: Error) => void): this;
    /**
     * Emitted when there is an available update. The update is downloaded
     * automatically.
     */
    on(event: 'update-available', listener: Function): this;
    /**
     * Emitted when an update has been downloaded. On Windows only releaseName is
     * available.
     */
    on(event: 'update-downloaded', listener: (event: Event,
                                              releaseNotes: string,
                                              releaseName: string,
                                              releaseDate: Date,
                                              updateURL: string) => void): this;
    /**
     * Emitted when there is no available update.
     */
    on(event: 'update-not-available', listener: Function): this;
    /**
     * Asks the server whether there is an update. You must call setFeedURL before
     * using this API.
     */
    checkForUpdates(): void;
    getFeedURL(): string;
    /**
     * Restarts the app and installs the update after it has been downloaded. It should
     * only be called after update-downloaded has been emitted.
     */
    quitAndInstall(): void;
    /**
     * Sets the url and initialize the auto updater.
     */
    setFeedURL(url: string, requestHeaders: any): void;
  }

  type BluetoothDevice = {

    // Docs: http://electron.atom.io/docs/api/structures/bluetooth-device

    deviceId: string;
    deviceName: string;
  }

  class BrowserWindow extends EventEmitter {

    // Docs: http://electron.atom.io/docs/api/browser-window

    /**
     * Emitted when an App Command is invoked. These are typically related to keyboard
     * media keys or browser commands, as well as the "Back" button built into some
     * mice on Windows. Commands are lowercased, underscores are replaced with hyphens,
     * and the APPCOMMAND_ prefix is stripped off. e.g. APPCOMMAND_BROWSER_BACKWARD is
     * emitted as browser-backward.
     */
    on(event: 'app-command', listener: (event: Event,
                                        command: string) => void): this;
    /**
     * Emitted when the window loses focus.
     */
    on(event: 'blur', listener: Function): this;
    /**
     * Emitted when the window is going to be closed. It's emitted before the
     * beforeunload and unload event of the DOM. Calling event.preventDefault() will
     * cancel the close. Usually you would want to use the beforeunload handler to
     * decide whether the window should be closed, which will also be called when the
     * window is reloaded. In Electron, returning any value other than undefined would
     * cancel the close. For example:
     */
    on(event: 'close', listener: (event: Event) => void): this;
    /**
     * Emitted when the window is closed. After you have received this event you should
     * remove the reference to the window and avoid using it any more.
     */
    on(event: 'closed', listener: Function): this;
    /**
     * Emitted when the window enters a full-screen state.
     */
    on(event: 'enter-full-screen', listener: Function): this;
    /**
     * Emitted when the window enters a full-screen state triggered by HTML API.
     */
    on(event: 'enter-html-full-screen', listener: Function): this;
    /**
     * Emitted when the window gains focus.
     */
    on(event: 'focus', listener: Function): this;
    /**
     * Emitted when the window is hidden.
     */
    on(event: 'hide', listener: Function): this;
    /**
     * Emitted when the window leaves a full-screen state.
     */
    on(event: 'leave-full-screen', listener: Function): this;
    /**
     * Emitted when the window leaves a full-screen state triggered by HTML API.
     */
    on(event: 'leave-html-full-screen', listener: Function): this;
    /**
     * Emitted when window is maximized.
     */
    on(event: 'maximize', listener: Function): this;
    /**
     * Emitted when the window is minimized.
     */
    on(event: 'minimize', listener: Function): this;
    /**
     * Emitted when the window is being moved to a new position. Note: On macOS this
     * event is just an alias of moved.
     */
    on(event: 'move', listener: Function): this;
    /**
     * Emitted once when the window is moved to a new position.
     */
    on(event: 'moved', listener: Function): this;
    /**
     * Emitted when the document changed its title, calling event.preventDefault() will
     * prevent the native window's title from changing.
     */
    on(event: 'page-title-updated', listener: (event: Event,
                                               title: string) => void): this;
    /**
     * Emitted when the web page has been rendered and window can be displayed without
     * a visual flash.
     */
    on(event: 'ready-to-show', listener: Function): this;
    /**
     * Emitted when the window is being resized.
     */
    on(event: 'resize', listener: Function): this;
    /**
     * Emitted when the unresponsive web page becomes responsive again.
     */
    on(event: 'responsive', listener: Function): this;
    /**
     * Emitted when the window is restored from a minimized state.
     */
    on(event: 'restore', listener: Function): this;
    /**
     * Emitted when scroll wheel event phase has begun.
     */
    on(event: 'scroll-touch-begin', listener: Function): this;
    /**
     * Emitted when scroll wheel event phase filed upon reaching the edge of element.
     */
    on(event: 'scroll-touch-edge', listener: Function): this;
    /**
     * Emitted when scroll wheel event phase has ended.
     */
    on(event: 'scroll-touch-end', listener: Function): this;
    /**
     * Emitted when the window is shown.
     */
    on(event: 'show', listener: Function): this;
    /**
     * Emitted on 3-finger swipe. Possible directions are up, right, down, left.
     */
    on(event: 'swipe', listener: (event: Event,
                                  direction: string) => void): this;
    /**
     * Emitted when the window exits from a maximized state.
     */
    on(event: 'unmaximize', listener: Function): this;
    /**
     * Emitted when the web page becomes unresponsive.
     */
    on(event: 'unresponsive', listener: Function): this;
    constructor(options: BrowserWindowConstructorOptions);
    /**
     * Adds DevTools extension located at path, and returns extension's name. The
     * extension will be remembered so you only need to call this API once, this API is
     * not for programming use. If you try to add an extension that has already been
     * loaded, this method will not return and instead log a warning to the console.
     * The method will also not return if the extension's manifest is missing or
     * incomplete. Note: This API cannot be called before the ready event of the app
     * module is emitted.
     */
    static addDevToolsExtension(path: string): void;
    static fromId(id: number): BrowserWindow;
    static fromWebContents(webContents: WebContents): BrowserWindow;
    static getAllWindows(): BrowserWindow[];
    /**
     * To check if a DevTools extension is installed you can run the following: Note:
     * This API cannot be called before the ready event of the app module is emitted.
     */
    static getDevToolsExtensions(): DevToolsExtensions;
    static getFocusedWindow(): BrowserWindow;
    /**
     * Remove a DevTools extension by name. Note: This API cannot be called before the
     * ready event of the app module is emitted.
     */
    static removeDevToolsExtension(name: string): void;
    /**
     * Removes focus from the window.
     */
    blur(): void;
    blurWebView(): void;
    /**
     * Same as webContents.capturePage([rect, ]callback).
     */
    capturePage(callback: (image: NativeImage) => void): void;
    /**
     * Same as webContents.capturePage([rect, ]callback).
     */
    capturePage(rect: Rectangle, callback: (image: NativeImage) => void): void;
    /**
     * Moves window to the center of the screen.
     */
    center(): void;
    /**
     * Try to close the window. This has the same effect as a user manually clicking
     * the close button of the window. The web page may cancel the close though. See
     * the close event.
     */
    close(): void;
    /**
     * Force closing the window, the unload and beforeunload event won't be emitted for
     * the web page, and close event will also not be emitted for this window, but it
     * guarantees the closed event will be emitted.
     */
    destroy(): void;
    /**
     * Starts or stops flashing the window to attract user's attention.
     */
    flashFrame(flag: boolean): void;
    /**
     * Focuses on the window.
     */
    focus(): void;
    focusOnWebView(): void;
    getBounds(): Rectangle;
    getChildWindows(): BrowserWindow[];
    getContentBounds(): Rectangle;
    getContentSize(): number[];
    getMaximumSize(): number[];
    getMinimumSize(): number[];
    /**
     * The native type of the handle is HWND on Windows, NSView* on macOS, and Window
     * (unsigned long) on Linux.
     */
    getNativeWindowHandle(): Buffer;
    getParentWindow(): BrowserWindow;
    getPosition(): number[];
    getRepresentedFilename(): string;
    getSize(): number[];
    /**
     * Note: The title of web page can be different from the title of the native
     * window.
     */
    getTitle(): string;
    /**
     * On Windows and Linux always returns true.
     */
    hasShadow(): boolean;
    /**
     * Hides the window.
     */
    hide(): void;
    /**
     * Hooks a windows message. The callback is called when the message is received in
     * the WndProc.
     */
    hookWindowMessage(message: number, callback: Function): void;
    isAlwaysOnTop(): boolean;
    /**
     * On Linux always returns true.
     */
    isClosable(): boolean;
    isDestroyed(): boolean;
    /**
     * Whether Boolean - Whether the window's document has been edited.
     */
    isDocumentEdited(): void;
    isFocused(): boolean;
    isFullScreen(): boolean;
    isFullScreenable(): boolean;
    isKiosk(): boolean;
    /**
     * On Linux always returns true.
     */
    isMaximizable(): boolean;
    isMaximized(): boolean;
    isMenuBarAutoHide(): boolean;
    isMenuBarVisible(): boolean;
    /**
     * On Linux always returns true.
     */
    isMinimizable(): boolean;
    isMinimized(): boolean;
    isModal(): boolean;
    /**
     * On Linux always returns true.
     */
    isMovable(): boolean;
    isResizable(): boolean;
    isVisible(): boolean;
    /**
     * Note: This API always returns false on Windows.
     */
    isVisibleOnAllWorkspaces(): boolean;
    isWindowMessageHooked(message: number): boolean;
    /**
     * Same as webContents.loadURL(url[, options]). The url can be a remote address
     * (e.g. http://) or a path to a local HTML file using the file:// protocol. To
     * ensure that file URLs are properly formatted, it is recommended to use Node's
     * url.format method:
     */
    loadURL(url: URL, options?: LoadURLOptions): void;
    /**
     * Maximizes the window.
     */
    maximize(): void;
    /**
     * Minimizes the window. On some platforms the minimized window will be shown in
     * the Dock.
     */
    minimize(): void;
    /**
     * Uses Quick Look to preview a file at a given path.
     */
    previewFile(path: string, displayName?: string): void;
    /**
     * Same as webContents.reload.
     */
    reload(): void;
    /**
     * Restores the window from minimized state to its previous state.
     */
    restore(): void;
    /**
     * Sets whether the window should show always on top of other windows. After
     * setting this, the window is still a normal window, not a toolbox window which
     * can not be focused on.
     */
    setAlwaysOnTop(flag: boolean, level?: 'normal' | 'floating' | 'torn-off-menu' | 'modal-panel' | 'main-menu' | 'status' | 'pop-up-menu' | 'screen-saver'): void;
    /**
     * This will make a window maintain an aspect ratio. The extra size allows a
     * developer to have space, specified in pixels, not included within the aspect
     * ratio calculations. This API already takes into account the difference between a
     * window's size and its content size. Consider a normal window with an HD video
     * player and associated controls. Perhaps there are 15 pixels of controls on the
     * left edge, 25 pixels of controls on the right edge and 50 pixels of controls
     * below the player. In order to maintain a 16:9 aspect ratio (standard aspect
     * ratio for HD @1920x1080) within the player itself we would call this function
     * with arguments of 16/9 and [ 40, 50 ]. The second argument doesn't care where
     * the extra width and height are within the content view--only that they exist.
     * Just sum any extra width and height areas you have within the overall content
     * view.
     */
    setAspectRatio(aspectRatio: number, extraSize?: ExtraSize): void;
    /**
     * Sets whether the window menu bar should hide itself automatically. Once set the
     * menu bar will only show when users press the single Alt key. If the menu bar is
     * already visible, calling setAutoHideMenuBar(true) won't hide it immediately.
     */
    setAutoHideMenuBar(hide: boolean): void;
    /**
     * Resizes and moves the window to the supplied bounds
     */
    setBounds(bounds: Rectangle, animate?: boolean): void;
    /**
     * Sets whether the window can be manually closed by user. On Linux does nothing.
     */
    setClosable(closable: boolean): void;
    /**
     * Resizes and moves the window's client area (e.g. the web page) to the supplied
     * bounds.
     */
    setContentBounds(bounds: Rectangle, animate?: boolean): void;
    /**
     * Prevents the window contents from being captured by other apps. On macOS it sets
     * the NSWindow's sharingType to NSWindowSharingNone. On Windows it calls
     * SetWindowDisplayAffinity with WDA_MONITOR.
     */
    setContentProtection(enable: boolean): void;
    /**
     * Resizes the window's client area (e.g. the web page) to width and height.
     */
    setContentSize(width: number, height: number, animate?: boolean): void;
    /**
     * Specifies whether the window’s document has been edited, and the icon in title
     * bar will become gray when set to true.
     */
    setDocumentEdited(edited: boolean): void;
    /**
     * Changes whether the window can be focused.
     */
    setFocusable(focusable: boolean): void;
    /**
     * Sets whether the window should be in fullscreen mode.
     */
    setFullScreen(flag: boolean): void;
    /**
     * Sets whether the maximize/zoom window button toggles fullscreen mode or
     * maximizes the window.
     */
    setFullScreenable(fullscreenable: boolean): void;
    /**
     * Sets whether the window should have a shadow. On Windows and Linux does nothing.
     */
    setHasShadow(hasShadow: boolean): void;
    /**
     * Changes window icon.
     */
    setIcon(icon: NativeImage): void;
    /**
     * Makes the window ignore all mouse events. All mouse events happened in this
     * window will be passed to the window below this window, but if this window has
     * focus, it will still receive keyboard events.
     */
    setIgnoreMouseEvents(ignore: boolean): void;
    /**
     * Enters or leaves the kiosk mode.
     */
    setKiosk(flag: boolean): void;
    /**
     * Sets whether the window can be manually maximized by user. On Linux does
     * nothing.
     */
    setMaximizable(maximizable: boolean): void;
    /**
     * Sets the maximum size of window to width and height.
     */
    setMaximumSize(width: number, height: number): void;
    /**
     * Sets the menu as the window's menu bar, setting it to null will remove the menu
     * bar.
     */
    setMenu(menu: Menu): void;
    /**
     * Sets whether the menu bar should be visible. If the menu bar is auto-hide, users
     * can still bring up the menu bar by pressing the single Alt key.
     */
    setMenuBarVisibility(visible: boolean): void;
    /**
     * Sets whether the window can be manually minimized by user. On Linux does
     * nothing.
     */
    setMinimizable(minimizable: boolean): void;
    /**
     * Sets the minimum size of window to width and height.
     */
    setMinimumSize(width: number, height: number): void;
    /**
     * Sets whether the window can be moved by user. On Linux does nothing.
     */
    setMovable(movable: boolean): void;
    /**
     * Sets a 16 x 16 pixel overlay onto the current taskbar icon, usually used to
     * convey some sort of application status or to passively notify the user.
     */
    setOverlayIcon(overlay: NativeImage, description: string): void;
    /**
     * Sets parent as current window's parent window, passing null will turn current
     * window into a top-level window.
     */
    setParentWindow(parent: BrowserWindow): void;
    /**
     * Moves window to x and y.
     */
    setPosition(x: number, y: number, animate?: boolean): void;
    /**
     * Sets progress value in progress bar. Valid range is [0, 1.0]. Remove progress
     * bar when progress < 0; Change to indeterminate mode when progress > 1. On Linux
     * platform, only supports Unity desktop environment, you need to specify the
     * *.desktop file name to desktopName field in package.json. By default, it will
     * assume app.getName().desktop. On Windows, a mode can be passed. Accepted values
     * are none, normal, indeterminate, error, and paused. If you call setProgressBar
     * without a mode set (but with a value within the valid range), normal will be
     * assumed.
     */
    setProgressBar(progress: number, options?: ProgressBarOptions): void;
    /**
     * Sets the pathname of the file the window represents, and the icon of the file
     * will show in window's title bar.
     */
    setRepresentedFilename(filename: string): void;
    /**
     * Sets whether the window can be manually resized by user.
     */
    setResizable(resizable: boolean): void;
    /**
     * Changes the attachment point for sheets on macOS. By default, sheets are
     * attached just below the window frame, but you may want to display them beneath a
     * HTML-rendered toolbar. For example:
     */
    setSheetOffset(offsetY: number, offsetX?: number): void;
    /**
     * Resizes the window to width and height.
     */
    setSize(width: number, height: number, animate?: boolean): void;
    /**
     * Makes the window not show in the taskbar.
     */
    setSkipTaskbar(skip: boolean): void;
    /**
     * Add a thumbnail toolbar with a specified set of buttons to the thumbnail image
     * of a window in a taskbar button layout. Returns a Boolean object indicates
     * whether the thumbnail has been added successfully. The number of buttons in
     * thumbnail toolbar should be no greater than 7 due to the limited room. Once you
     * setup the thumbnail toolbar, the toolbar cannot be removed due to the platform's
     * limitation. But you can call the API with an empty array to clean the buttons.
     * The buttons is an array of Button objects: The flags is an array that can
     * include following Strings:
     */
    setThumbarButtons(buttons: ThumbarButton[]): boolean;
    /**
     * Sets the region of the window to show as the thumbnail image displayed when
     * hovering over the window in the taskbar. You can reset the thumbnail to be the
     * entire window by specifying an empty region: {x: 0, y: 0, width: 0, height: 0}.
     */
    setThumbnailClip(region: Rectangle): void;
    /**
     * Sets the toolTip that is displayed when hovering over the window thumbnail in
     * the taskbar.
     */
    setThumbnailToolTip(toolTip: string): void;
    /**
     * Changes the title of native window to title.
     */
    setTitle(title: string): void;
    /**
     * Sets whether the window should be visible on all workspaces. Note: This API does
     * nothing on Windows.
     */
    setVisibleOnAllWorkspaces(visible: boolean): void;
    /**
     * Shows and gives focus to the window.
     */
    show(): void;
    /**
     * Same as webContents.showDefinitionForSelection().
     */
    showDefinitionForSelection(): void;
    /**
     * Shows the window but doesn't focus on it.
     */
    showInactive(): void;
    /**
     * Unhooks all of the window messages.
     */
    unhookAllWindowMessages(): void;
    /**
     * Unhook the window message.
     */
    unhookWindowMessage(message: number): void;
    /**
     * Unmaximizes the window.
     */
    unmaximize(): void;
    id: number;
    webContents: WebContents;
  }

  class BrowserWindowProxy extends EventEmitter {

    // Docs: http://electron.atom.io/docs/api/browser-window-proxy

    /**
     * Removes focus from the child window.
     */
    blur(): void;
    /**
     * Forcefully closes the child window without calling its unload event.
     */
    close(): void;
    /**
     * Evaluates the code in the child window.
     */
    eval(code: string): void;
    /**
     * Focuses the child window (brings the window to front).
     */
    focus(): void;
    /**
     * Sends a message to the child window with the specified origin or * for no origin
     * preference. In addition to these methods, the child window implements
     * window.opener object with no properties and a single method.
     */
    postMessage(message: string, targetOrigin: string): void;
    /**
     * Invokes the print dialog on the child window.
     */
    print(): void;
    closed: boolean;
  }

  type Certificate = {

    // Docs: http://electron.atom.io/docs/api/structures/certificate

    data: string;
    fingerprint: string;
    issuerName: string;
    serialNumber: string;
    subjectName: string;
    validExpiry: number;
    validStart: number;
  }

  class ClientRequest extends EventEmitter {

    // Docs: http://electron.atom.io/docs/api/client-request

    /**
     * Emitted when the request is aborted. The abort event will not be fired if the
     * request is already closed.
     */
    on(event: 'abort', listener: Function): this;
    /**
     * Emitted as the last event in the HTTP request-response transaction. The close
     * event indicates that no more events will be emitted on either the request or
     * response objects.
     */
    on(event: 'close', listener: Function): this;
    /**
     * Emitted when the net module fails to issue a network request. Typically when the
     * request object emits an error event, a close event will subsequently follow and
     * no response object will be provided.
     */
    on(event: 'error', listener: (/**
                                   * an error object providing some information about the failure.
                                   */
                                  error: Error) => void): this;
    /**
     * Emitted just after the last chunk of the request's data has been written into
     * the request object.
     */
    on(event: 'finish', listener: Function): this;
    /**
     * Emitted when an authenticating proxy is asking for user credentials. The
     * callback function is expected to be called back with user credentials: Providing
     * empty credentials will cancel the request and report an authentication error on
     * the response object:
     */
    on(event: 'login', listener: (authInfo: AuthInfo,
                                  callback: Function) => void): this;
    on(event: 'response', listener: (/**
                                      * An object representing the HTTP response message.
                                      */
                                     response: IncomingMessage) => void): this;
    constructor(options: any | string);
    /**
     * Cancels an ongoing HTTP transaction. If the request has already emitted the
     * close event, the abort operation will have no effect. Otherwise an ongoing event
     * will emit abort and close events. Additionally, if there is an ongoing response
     * object,it will emit the aborted event.
     */
    abort(): void;
    /**
     * Sends the last chunk of the request data. Subsequent write or end operations
     * will not be allowed. The finish event is emitted just after the end operation.
     */
    end(chunk?: string | Buffer, encoding?: string, callback?: Function): void;
    /**
     * Returns String - The value of a previously set extra header name.
     */
    getHeader(name: string): void;
    /**
     * Removes a previously set extra header name. This method can be called only
     * before first write. Trying to call it after the first write will throw an error.
     */
    removeHeader(name: string): void;
    /**
     * Adds an extra HTTP header. The header name will issued as it is without
     * lowercasing. It can be called only before first write. Calling this method after
     * the first write will throw an error.
     */
    setHeader(name: string, value: string): void;
    /**
     * callback is essentially a dummy function introduced in the purpose of keeping
     * similarity with the Node.js API. It is called asynchronously in the next tick
     * after chunk content have been delivered to the Chromium networking layer.
     * Contrary to the Node.js implementation, it is not guaranteed that chunk content
     * have been flushed on the wire before callback is called. Adds a chunk of data to
     * the request body. The first write operation may cause the request headers to be
     * issued on the wire. After the first write operation, it is not allowed to add or
     * remove a custom header.
     */
    write(chunk: string | Buffer, encoding?: string, callback?: Function): void;
    chunkedEncoding: boolean;
  }

  interface Clipboard extends EventEmitter {

    // Docs: http://electron.atom.io/docs/api/clipboard

    availableFormats(type?: string): string[];
    /**
     * Clears the clipboard content.
     */
    clear(type?: string): void;
    has(data: string, type?: string): boolean;
    read(data: string, type?: string): string;
    /**
     * Returns an Object containing title and url keys representing the bookmark in the
     * clipboard. The title and url values will be empty strings when the bookmark is
     * unavailable.
     */
    readBookmark(): ReadBookmark;
    readFindText(): string;
    readHTML(type?: string): string;
    readImage(type?: string): NativeImage;
    readRTF(type?: string): string;
    readText(type?: string): string;
    /**
     * Writes data to the clipboard.
     */
    write(data: Data, type?: string): void;
    /**
     * Writes the title and url into the clipboard as a bookmark. Note: Most apps on
     * Windows don't support pasting bookmarks into them so you can use clipboard.write
     * to write both a bookmark and fallback text to the clipboard.
     */
    writeBookmark(title: string, url: string, type?: string): void;
    /**
     * Writes the text into the find pasteboard as plain text. This method uses
     * synchronous IPC when called from the renderer process.
     */
    writeFindText(text: string): void;
    /**
     * Writes markup to the clipboard.
     */
    writeHTML(markup: string, type?: string): void;
    /**
     * Writes image to the clipboard.
     */
    writeImage(image: NativeImage, type?: string): void;
    /**
     * Writes the text into the clipboard in RTF.
     */
    writeRTF(text: string, type?: string): void;
    /**
     * Writes the text into the clipboard as plain text.
     */
    writeText(text: string, type?: string): void;
  }

  interface ContentTracing extends EventEmitter {

    // Docs: http://electron.atom.io/docs/api/content-tracing

    /**
     * Cancel the watch event. This may lead to a race condition with the watch event
     * callback if tracing is enabled.
     */
    cancelWatchEvent(): void;
    /**
     * Get the current monitoring traced data. Child processes typically cache trace
     * data and only rarely flush and send trace data back to the main process. This is
     * because it may be an expensive operation to send the trace data over IPC and we
     * would like to avoid unneeded runtime overhead from tracing. So, to end tracing,
     * we must asynchronously ask all child processes to flush any pending trace data.
     * Once all child processes have acknowledged the captureMonitoringSnapshot request
     * the callback will be called with a file that contains the traced data.
     */
    captureMonitoringSnapshot(resultFilePath: string, callback: (resultFilePath: string) => void): void;
    /**
     * Get a set of category groups. The category groups can change as new code paths
     * are reached. Once all child processes have acknowledged the getCategories
     * request the callback is invoked with an array of category groups.
     */
    getCategories(callback: (categories: string[]) => void): void;
    /**
     * Get the maximum usage across processes of trace buffer as a percentage of the
     * full state. When the TraceBufferUsage value is determined the callback is
     * called.
     */
    getTraceBufferUsage(callback: (value: number, percentage: number) => void): void;
    /**
     * callback will be called every time the given event occurs on any process.
     */
    setWatchEvent(categoryName: string, eventName: string, callback: Function): void;
    /**
     * Start monitoring on all processes. Monitoring begins immediately locally and
     * asynchronously on child processes as soon as they receive the startMonitoring
     * request. Once all child processes have acknowledged the startMonitoring request
     * the callback will be called.
     */
    startMonitoring(options: StartMonitoringOptions, callback: Function): void;
    /**
     * Start recording on all processes. Recording begins immediately locally and
     * asynchronously on child processes as soon as they receive the EnableRecording
     * request. The callback will be called once all child processes have acknowledged
     * the startRecording request. categoryFilter is a filter to control what category
     * groups should be traced. A filter can have an optional - prefix to exclude
     * category groups that contain a matching category. Having both included and
     * excluded category patterns in the same list is not supported. Examples:
     * traceOptions controls what kind of tracing is enabled, it is a comma-delimited
     * list. Possible options are: The first 3 options are trace recoding modes and
     * hence mutually exclusive. If more than one trace recording modes appear in the
     * traceOptions string, the last one takes precedence. If none of the trace
     * recording modes are specified, recording mode is record-until-full. The trace
     * option will first be reset to the default option (record_mode set to
     * record-until-full, enable_sampling and enable_systrace set to false) before
     * options parsed from traceOptions are applied on it.
     */
    startRecording(options: StartRecordingOptions, callback: Function): void;
    /**
     * Stop monitoring on all processes. Once all child processes have acknowledged the
     * stopMonitoring request the callback is called.
     */
    stopMonitoring(callback: Function): void;
    /**
     * Stop recording on all processes. Child processes typically cache trace data and
     * only rarely flush and send trace data back to the main process. This helps to
     * minimize the runtime overhead of tracing since sending trace data over IPC can
     * be an expensive operation. So, to end tracing, we must asynchronously ask all
     * child processes to flush any pending trace data. Once all child processes have
     * acknowledged the stopRecording request, callback will be called with a file that
     * contains the traced data. Trace data will be written into resultFilePath if it
     * is not empty or into a temporary file. The actual file path will be passed to
     * callback if it's not null.
     */
    stopRecording(resultFilePath: string, callback: (resultFilePath: string) => void): void;
  }

  type Cookie = {

    // Docs: http://electron.atom.io/docs/api/structures/cookie

    domain: string;
    expirationDate?: number;
    hostOnly: string;
    httpOnly: boolean;
    name: string;
    path: string;
    secure: boolean;
    session: boolean;
    value: string;
  }

  class Cookies extends EventEmitter {

    // Docs: http://electron.atom.io/docs/api/cookies

    /**
     * Emitted when a cookie is changed because it was added, edited, removed, or
     * expired.
     */
    on(event: 'changed', listener: (event: Event,
                                    /**
                                     * The cookie that was changed
                                     */
                                    cookie: Cookie,
                                    /**
                                     * The cause of the change with one of the following values:
                                     */
                                    cause: string,
                                    /**
                                     * `true` if the cookie was removed, `false` otherwise.
                                     */
                                    removed: boolean) => void): this;
    /**
     * Sends a request to get all cookies matching details, callback will be called
     * with callback(error, cookies) on complete. cookies is an Array of cookie
     * objects.
     */
    get(filter: Filter, callback: (error: Error, cookies: Cookies[]) => void): void;
    /**
     * Removes the cookies matching url and name, callback will called with callback()
     * on complete.
     */
    remove(url: string, name: string, callback: Function): void;
    /**
     * Sets a cookie with details, callback will be called with callback(error) on
     * complete.
     */
    set(details: Details, callback: (error: Error) => void): void;
  }

  interface CrashReporter extends EventEmitter {

    // Docs: http://electron.atom.io/docs/api/crash-reporter

    /**
     * Returns the date and ID of the last crash report. If no crash reports have been
     * sent or the crash reporter has not been started, null is returned.
     */
    getLastCrashReport(): LastCrashReport;
    /**
     * Returns all uploaded crash reports. Each report contains the date and uploaded
     * ID.
     */
    getUploadedReports(): Object[];
    /**
     * You are required to call this method before using other crashReporter APIs.
     * Note: On macOS, Electron uses a new crashpad client, which is different from
     * breakpad on Windows and Linux. To enable the crash collection feature, you are
     * required to call the crashReporter.start API to initialize crashpad in the main
     * process and in each renderer process from which you wish to collect crash
     * reports.
     */
    start(options: CrashReporterStartOptions): void;
  }

  class Debugger extends EventEmitter {

    // Docs: http://electron.atom.io/docs/api/debugger

    /**
     * Emitted when debugging session is terminated. This happens either when
     * webContents is closed or devtools is invoked for the attached webContents.
     */
    on(event: 'detach', listener: (event: Event,
                                   /**
                                    * Reason for detaching debugger.
                                    */
                                   reason: string) => void): this;
    /**
     * Emitted whenever debugging target issues instrumentation event.
     */
    on(event: 'message', listener: (event: Event,
                                    /**
                                     * Method name.
                                     */
                                    method: string,
                                    /**
                                     * Event parameters defined by the 'parameters' attribute in the remote debugging
                                     * protocol.
                                     */
                                    params: any) => void): this;
    /**
     * Attaches the debugger to the webContents.
     */
    attach(protocolVersion?: string): void;
    /**
     * Detaches the debugger from the webContents.
     */
    detach(): void;
    isAttached(): boolean;
    /**
     * Send given command to the debugging target.
     */
    sendCommand(method: string, commandParams?: any, callback?: (error: any, result: any) => void): void;
  }

  interface DesktopCapturer extends EventEmitter {

    // Docs: http://electron.atom.io/docs/api/desktop-capturer

    /**
     * Starts gathering information about all available desktop media sources, and
     * calls callback(error, sources) when finished. sources is an array of
     * DesktopCapturerSource objects, each DesktopCapturerSource represents a screen or
     * an individual window that can be captured.
     */
    getSources(options: SourcesOptions, callback: (error: Error, sources: DesktopCapturerSource[]) => void): void;
  }

  type DesktopCapturerSource = {

    // Docs: http://electron.atom.io/docs/api/structures/desktop-capturer-source

    id: string;
    name: string;
    thumbnail: NativeImage;
  }

  interface Dialog extends EventEmitter {

    // Docs: http://electron.atom.io/docs/api/dialog

    /**
     * Displays a modal dialog that shows an error message. This API can be called
     * safely before the ready event the app module emits, it is usually used to report
     * errors in early stage of startup.  If called before the app readyevent on Linux,
     * the message will be emitted to stderr, and no GUI dialog will appear.
     */
    showErrorBox(title: string, content: string): void;
    /**
     * Shows a message box, it will block the process until the message box is closed.
     * It returns the index of the clicked button. If a callback is passed, the API
     * call will be asynchronous and the result will be passed via callback(response).
     */
    showMessageBox(browserWindow: BrowserWindow, options: MessageBoxOptions, callback: (response: number) => void): void;
    /**
     * Shows a message box, it will block the process until the message box is closed.
     * It returns the index of the clicked button. If a callback is passed, the API
     * call will be asynchronous and the result will be passed via callback(response).
     */
    showMessageBox(options: MessageBoxOptions, callback: (response: number) => void): void;
    /**
     * Shows a message box, it will block the process until the message box is closed.
     * It returns the index of the clicked button. If a callback is passed, the API
     * call will be asynchronous and the result will be passed via callback(response).
     */
    showMessageBox(browserWindow: BrowserWindow, options: MessageBoxOptions, callback: (response: number) => void): void;
    /**
     * On success this method returns an array of file paths chosen by the user,
     * otherwise it returns undefined. The filters specifies an array of file types
     * that can be displayed or selected when you want to limit the user to a specific
     * type. For example: The extensions array should contain extensions without
     * wildcards or dots (e.g. 'png' is good but '.png' and '*.png' are bad). To show
     * all files, use the '*' wildcard (no other wildcard is supported). If a callback
     * is passed, the API call will be asynchronous and the result will be passed via
     * callback(filenames) Note: On Windows and Linux an open dialog can not be both a
     * file selector and a directory selector, so if you set properties to ['openFile',
     * 'openDirectory'] on these platforms, a directory selector will be shown.
     */
    showOpenDialog(browserWindow: BrowserWindow, options: OpenDialogOptions, callback?: (filePaths: string[]) => void): void;
    /**
     * On success this method returns an array of file paths chosen by the user,
     * otherwise it returns undefined. The filters specifies an array of file types
     * that can be displayed or selected when you want to limit the user to a specific
     * type. For example: The extensions array should contain extensions without
     * wildcards or dots (e.g. 'png' is good but '.png' and '*.png' are bad). To show
     * all files, use the '*' wildcard (no other wildcard is supported). If a callback
     * is passed, the API call will be asynchronous and the result will be passed via
     * callback(filenames) Note: On Windows and Linux an open dialog can not be both a
     * file selector and a directory selector, so if you set properties to ['openFile',
     * 'openDirectory'] on these platforms, a directory selector will be shown.
     */
    showOpenDialog(options: OpenDialogOptions, callback?: (filePaths: string[]) => void): void;
    /**
     * On success this method returns the path of the file chosen by the user,
     * otherwise it returns undefined. The filters specifies an array of file types
     * that can be displayed, see dialog.showOpenDialog for an example. If a callback
     * is passed, the API call will be asynchronous and the result will be passed via
     * callback(filename)
     */
    showSaveDialog(browserWindow: BrowserWindow, options: SaveDialogOptions, callback?: (filename: string) => void): void;
    /**
     * On success this method returns the path of the file chosen by the user,
     * otherwise it returns undefined. The filters specifies an array of file types
     * that can be displayed, see dialog.showOpenDialog for an example. If a callback
     * is passed, the API call will be asynchronous and the result will be passed via
     * callback(filename)
     */
    showSaveDialog(options: SaveDialogOptions, callback?: (filename: string) => void): void;
  }

  type Display = {

    // Docs: http://electron.atom.io/docs/api/structures/display

    bounds: Rectangle;
    id: number;
    rotation: number;
    scaleFactor: number;
    size: Size;
    touchSupport: string;
    workArea: Rectangle;
    workAreaSize: WorkAreaSize;
  }

  interface GlobalShortcut extends EventEmitter {

    // Docs: http://electron.atom.io/docs/api/global-shortcut

    /**
     * When the accelerator is already taken by other applications, this call will
     * still return false. This behavior is intended by operating systems, since they
     * don't want applications to fight for global shortcuts.
     */
    isRegistered(accelerator: Accelerator): boolean;
    /**
     * Registers a global shortcut of accelerator. The callback is called when the
     * registered shortcut is pressed by the user. When the accelerator is already
     * taken by other applications, this call will silently fail. This behavior is
     * intended by operating systems, since they don't want applications to fight for
     * global shortcuts.
     */
    register(accelerator: Accelerator, callback: Function): void;
    /**
     * Unregisters the global shortcut of accelerator.
     */
    unregister(accelerator: Accelerator): void;
    /**
     * Unregisters all of the global shortcuts.
     */
    unregisterAll(): void;
  }

  class IncomingMessage extends EventEmitter {

    // Docs: http://electron.atom.io/docs/api/incoming-message

    /**
     * Emitted when a request has been canceled during an ongoing HTTP transaction.
     */
    on(event: 'aborted', listener: Function): this;
    /**
     * The data event is the usual method of transferring response data into
     * applicative code.
     */
    on(event: 'data', listener: (/**
                                  * A chunk of response body's data.
                                  */
                                 chunk: Buffer) => void): this;
    /**
     * Indicates that response body has ended.
     */
    on(event: 'end', listener: Function): this;
    /**
     * error Error - Typically holds an error string identifying failure root cause.
     * Emitted when an error was encountered while streaming response data events. For
     * instance, if the server closes the underlying while the response is still
     * streaming, an error event will be emitted on the response object and a close
     * event will subsequently follow on the request object.
     */
    on(event: 'error', listener: Function): this;
    headers: any;
    httpVersion: string;
    statusCode: number;
    statusMessage: string;
  }

  interface IpcMain extends EventEmitter {

    // Docs: http://electron.atom.io/docs/api/ipc-main

  }

  interface IpcRenderer extends EventEmitter {

    // Docs: http://electron.atom.io/docs/api/ipc-renderer

  }

  type JumpListCategory = {

    // Docs: http://electron.atom.io/docs/api/structures/jump-list-category

    items: JumpListItem[];
    name: string;
    type: string;
  }

  type JumpListItem = {

    // Docs: http://electron.atom.io/docs/api/structures/jump-list-item

    args: string;
    description: string;
    iconIndex: number;
    iconPath: string;
    path: string;
    program: string;
    title: string;
    type: string;
  }

  type MemoryUsageDetails = {

    // Docs: http://electron.atom.io/docs/api/structures/memory-usage-details

    count: number;
    decodedSize: number;
    liveSize: number;
    purgeableSize: number;
    purgedSize: number;
    size: number;
  }

  class Menu {

    // Docs: http://electron.atom.io/docs/api/menu

    constructor();
    /**
     * Generally, the template is just an array of options for constructing a MenuItem.
     * The usage can be referenced above. You can also attach other fields to the
     * element of the template and they will become properties of the constructed menu
     * items.
     */
    static buildFromTemplate(template: MenuItem[]): void;
    /**
     * Returns the application menu (an instance of Menu), if set, or null, if not set.
     */
    static getApplicationMenu(): void;
    /**
     * Sends the action to the first responder of application. This is used for
     * emulating default Cocoa menu behaviors, usually you would just use the role
     * property of MenuItem. See the macOS Cocoa Event Handling Guide for more
     * information on macOS' native actions.
     */
    static sendActionToFirstResponder(action: string): void;
    /**
     * Sets menu as the application menu on macOS. On Windows and Linux, the menu will
     * be set as each window's top menu. Note: This API has to be called after the
     * ready event of app module.
     */
    static setApplicationMenu(menu: Menu): void;
    /**
     * Appends the menuItem to the menu.
     */
    append(menuItem: MenuItem): void;
    /**
     * Inserts the menuItem to the pos position of the menu.
     */
    insert(pos: number, menuItem: MenuItem): void;
    /**
     * Pops up this menu as a context menu in the browserWindow.
     */
    popup(browserWindow?: BrowserWindow, x?: number, y?: number, positioningItem?: number): void;
    items: MenuItem[];
  }

  class MenuItem {

    // Docs: http://electron.atom.io/docs/api/menu-item

    constructor(options: MenuItemConstructorOptions);
    checked: boolean;
    enabled: boolean;
    visible: boolean;
  }

  class NativeImage {

    // Docs: http://electron.atom.io/docs/api/native-image

    /**
     * Creates an empty NativeImage instance.
     */
    static createEmpty(): NativeImage;
    /**
     * Creates a new NativeImage instance from buffer. The default scaleFactor is 1.0.
     */
    static createFromBuffer(buffer: Buffer, scaleFactor?: number): NativeImage;
    /**
     * Creates a new NativeImage instance from dataURL.
     */
    static createFromDataURL(dataURL: string): void;
    /**
     * Creates a new NativeImage instance from a file located at path. This method
     * returns an empty image if the path does not exist, cannot be read, or is not a
     * valid image.
     */
    static createFromPath(path: string): NativeImage;
    crop(rect: Rect): NativeImage;
    getAspectRatio(): number;
    /**
     * The difference between getBitmap() and toBitmap() is, getBitmap() does not copy
     * the bitmap data, so you have to use the returned Buffer immediately in current
     * event loop tick, otherwise the data might be changed or destroyed.
     */
    getBitmap(): Buffer;
    /**
     * Notice that the returned pointer is a weak pointer to the underlying native
     * image instead of a copy, so you must ensure that the associated nativeImage
     * instance is kept around.
     */
    getNativeHandle(): Buffer;
    getSize(): Size;
    isEmpty(): boolean;
    isTemplateImage(): boolean;
    /**
     * If only the height or the width are specified then the current aspect ratio will
     * be preserved in the resized image.
     */
    resize(options: ResizeOptions): NativeImage;
    /**
     * Marks the image as a template image.
     */
    setTemplateImage(option: boolean): void;
    toBitmap(): Buffer;
    toDataURL(): string;
    toJPEG(quality: number): Buffer;
    toPNG(): Buffer;
  }

  interface Net extends EventEmitter {

    // Docs: http://electron.atom.io/docs/api/net

    /**
     * Creates a ClientRequest instance using the provided options which are directly
     * forwarded to the ClientRequest constructor. The net.request method would be used
     * to issue both secure and insecure HTTP requests according to the specified
     * protocol scheme in the options object.
     */
    request(options: any | string): ClientRequest;
  }

  interface PowerMonitor extends EventEmitter {

    // Docs: http://electron.atom.io/docs/api/power-monitor

    /**
     * Emitted when the system changes to AC power.
     */
    on(event: 'on-ac', listener: Function): this;
    /**
     * Emitted when system changes to battery power.
     */
    on(event: 'on-battery', listener: Function): this;
    /**
     * Emitted when system is resuming.
     */
    on(event: 'resume', listener: Function): this;
    /**
     * Emitted when the system is suspending.
     */
    on(event: 'suspend', listener: Function): this;
  }

  interface PowerSaveBlocker extends EventEmitter {

    // Docs: http://electron.atom.io/docs/api/power-save-blocker

    isStarted(id: number): boolean;
    /**
     * Starts preventing the system from entering lower-power mode. Returns an integer
     * identifying the power save blocker. Note: prevent-display-sleep has higher
     * precedence over prevent-app-suspension. Only the highest precedence type takes
     * effect. In other words, prevent-display-sleep always takes precedence over
     * prevent-app-suspension. For example, an API calling A requests for
     * prevent-app-suspension, and another calling B requests for
     * prevent-display-sleep. prevent-display-sleep will be used until B stops its
     * request. After that, prevent-app-suspension is used.
     */
    start(type: 'prevent-app-suspension' | 'prevent-display-sleep'): number;
    /**
     * Stops the specified power save blocker.
     */
    stop(id: number): void;
  }

  interface Process extends EventEmitter {

    // Docs: http://electron.atom.io/docs/api/process

    /**
     * Emitted when Electron has loaded its internal initialization script and is
     * beginning to load the web page or the main script. It can be used by the preload
     * script to add removed Node global symbols back to the global scope when node
     * integration is turned off:
     */
    on(event: 'loaded', listener: Function): this;
    /**
     * Causes the main thread of the current process crash.
     */
    crash(): void;
    /**
     * Returns an object giving memory usage statistics about the current process. Note
     * that all statistics are reported in Kilobytes.
     */
    getProcessMemoryInfo(): ProcessMemoryInfo;
    /**
     * Returns an object giving memory usage statistics about the entire system. Note
     * that all statistics are reported in Kilobytes.
     */
    getSystemMemoryInfo(): SystemMemoryInfo;
    /**
     * Causes the main thread of the current process hang.
     */
    hang(): void;
    /**
     * Sets the file descriptor soft limit to maxDescriptors or the OS hard limit,
     * whichever is lower for the current process.
     */
    setFdLimit(maxDescriptors: number): void;
  }

  interface Protocol extends EventEmitter {

    // Docs: http://electron.atom.io/docs/api/protocol

    /**
     * Intercepts scheme protocol and uses handler as the protocol's new handler which
     * sends a Buffer as a response.
     */
    interceptBufferProtocol(scheme: string, handler: (request: InterceptBufferProtocolRequest, callback: (buffer?: Buffer) => void) => void, completion?: (error: Error) => void): void;
    /**
     * Intercepts scheme protocol and uses handler as the protocol's new handler which
     * sends a file as a response.
     */
    interceptFileProtocol(scheme: string, handler: (request: InterceptFileProtocolRequest, callback: (filePath: string) => void) => void, completion?: (error: Error) => void): void;
    /**
     * Intercepts scheme protocol and uses handler as the protocol's new handler which
     * sends a new HTTP request as a response.
     */
    interceptHttpProtocol(scheme: string, handler: (request: InterceptHttpProtocolRequest, callback: (redirectRequest: RedirectRequest) => void) => void, completion?: (error: Error) => void): void;
    /**
     * Intercepts scheme protocol and uses handler as the protocol's new handler which
     * sends a String as a response.
     */
    interceptStringProtocol(scheme: string, handler: (request: InterceptStringProtocolRequest, callback: (data?: string) => void) => void, completion?: (error: Error) => void): void;
    /**
     * The callback will be called with a boolean that indicates whether there is
     * already a handler for scheme.
     */
    isProtocolHandled(scheme: string, callback: (error: Error) => void): void;
    /**
     * Registers a protocol of scheme that will send a Buffer as a response. The usage
     * is the same with registerFileProtocol, except that the callback should be called
     * with either a Buffer object or an object that has the data, mimeType, and
     * charset properties. Example:
     */
    registerBufferProtocol(scheme: string, handler: (request: RegisterBufferProtocolRequest, callback: (buffer?: Buffer) => void) => void, completion?: (error: Error) => void): void;
    /**
     * Registers a protocol of scheme that will send the file as a response. The
     * handler will be called with handler(request, callback) when a request is going
     * to be created with scheme. completion will be called with completion(null) when
     * scheme is successfully registered or completion(error) when failed. To handle
     * the request, the callback should be called with either the file's path or an
     * object that has a path property, e.g. callback(filePath) or callback({path:
     * filePath}). When callback is called with nothing, a number, or an object that
     * has an error property, the request will fail with the error number you
     * specified. For the available error numbers you can use, please see the net error
     * list. By default the scheme is treated like http:, which is parsed differently
     * than protocols that follow the "generic URI syntax" like file:, so you probably
     * want to call protocol.registerStandardSchemes to have your scheme treated as a
     * standard scheme.
     */
    registerFileProtocol(scheme: string, handler: (request: RegisterFileProtocolRequest, callback: (filePath?: string) => void) => void, completion?: (error: Error) => void): void;
    /**
     * Registers a protocol of scheme that will send an HTTP request as a response. The
     * usage is the same with registerFileProtocol, except that the callback should be
     * called with a redirectRequest object that has the url, method, referrer,
     * uploadData and session properties. By default the HTTP request will reuse the
     * current session. If you want the request to have a different session you should
     * set session to null. For POST requests the uploadData object must be provided.
     */
    registerHttpProtocol(scheme: string, handler: (request: RegisterHttpProtocolRequest, callback: (redirectRequest: RedirectRequest) => void) => void, completion?: (error: Error) => void): void;
    registerServiceWorkerSchemes(schemes: string[]): void;
    /**
     * A standard scheme adheres to what RFC 3986 calls generic URI syntax. For example
     * http and https are standard schemes, while file is not. Registering a scheme as
     * standard, will allow relative and absolute resources to be resolved correctly
     * when served. Otherwise the scheme will behave like the file protocol, but
     * without the ability to resolve relative URLs. For example when you load
     * following page with custom protocol without registering it as standard scheme,
     * the image will not be loaded because non-standard schemes can not recognize
     * relative URLs: Registering a scheme as standard will allow access to files
     * through the FileSystem API. Otherwise the renderer will throw a security error
     * for the scheme. By default web storage apis (localStorage, sessionStorage,
     * webSQL, indexedDB, cookies) are disabled for non standard schemes. So in general
     * if you want to register a custom protocol to replace the http protocol, you have
     * to register it as a standard scheme: Note: This method can only be used before
     * the ready event of the app module gets emitted.
     */
    registerStandardSchemes(schemes: string[]): void;
    /**
     * Registers a protocol of scheme that will send a String as a response. The usage
     * is the same with registerFileProtocol, except that the callback should be called
     * with either a String or an object that has the data, mimeType, and charset
     * properties.
     */
    registerStringProtocol(scheme: string, handler: (request: RegisterStringProtocolRequest, callback: (data?: string) => void) => void, completion?: (error: Error) => void): void;
    /**
     * Remove the interceptor installed for scheme and restore its original handler.
     */
    uninterceptProtocol(scheme: string, completion?: (error: Error) => void): void;
    /**
     * Unregisters the custom protocol of scheme.
     */
    unregisterProtocol(scheme: string, completion?: (error: Error) => void): void;
  }

  type Rectangle = {

    // Docs: http://electron.atom.io/docs/api/structures/rectangle

    height: number;
    width: number;
    x: number;
    y: number;
  }

  interface Remote extends MainInterface {

    // Docs: http://electron.atom.io/docs/api/remote

    getCurrentWebContents(): WebContents;
    getCurrentWindow(): BrowserWindow;
    getGlobal(name: string): any;
    require(module: string): Require;
  }

  interface Screen extends EventEmitter {

    // Docs: http://electron.atom.io/docs/api/screen

    /**
     * Emitted when newDisplay has been added.
     */
    on(event: 'display-added', listener: (event: Event,
                                          newDisplay: Display) => void): this;
    /**
     * Emitted when one or more metrics change in a display. The changedMetrics is an
     * array of strings that describe the changes. Possible changes are bounds,
     * workArea, scaleFactor and rotation.
     */
    on(event: 'display-metrics-changed', listener: (event: Event,
                                                    display: Display,
                                                    changedMetrics: string[]) => void): this;
    /**
     * Emitted when oldDisplay has been removed.
     */
    on(event: 'display-removed', listener: (event: Event,
                                            oldDisplay: Display) => void): this;
    getAllDisplays(): Display[];
    /**
     * The current absolute position of the mouse pointer.
     */
    getCursorScreenPoint(): CursorScreenPoint;
    getDisplayMatching(rect: Rectangle): Display;
    getDisplayNearestPoint(point: Point): Display;
    getPrimaryDisplay(): Display;
  }

  class Session extends EventEmitter {

    // Docs: http://electron.atom.io/docs/api/session

    /**
     * If partition starts with persist:, the page will use a persistent session
     * available to all pages in the app with the same partition. if there is no
     * persist: prefix, the page will use an in-memory session. If the partition is
     * empty then default session of the app will be returned. To create a Session with
     * options, you have to ensure the Session with the partition has never been used
     * before. There is no way to change the options of an existing Session object.
     */
    static fromPartition(partition: string, options: FromPartitionOptions): Session;
    /**
     * Emitted when Electron is about to download item in webContents. Calling
     * event.preventDefault() will cancel the download and item will not be available
     * from next tick of the process.
     */
    on(event: 'will-download', listener: (event: Event,
                                          item: DownloadItem,
                                          webContents: WebContents) => void): this;
    /**
     * Clears the session’s HTTP cache.
     */
    clearCache(callback: Function): void;
    /**
     * Clears the data of web storages.
     */
    clearStorageData(options?: ClearStorageDataOptions, callback?: Function): void;
    /**
     * Writes any unwritten DOMStorage data to disk.
     */
    flushStorageData(): void;
    /**
     * Returns the session's current cache size.
     */
    getCacheSize(callback: (size: number) => void): void;
    /**
     * Sets the proxy settings. When pacScript and proxyRules are provided together,
     * the proxyRules option is ignored and pacScript configuration is applied. The
     * proxyRules has to follow the rules below: For example: The proxyBypassRules is a
     * comma separated list of rules described below:
     */
    setProxy(config: Config, callback: Function): void;
    cookies: Cookies;
    protocol: Protocol;
    webRequest: WebRequest;
  }

  interface Shell {

    // Docs: http://electron.atom.io/docs/api/shell

    /**
     * Play the beep sound.
     */
    beep(): void;
    /**
     * Move the given file to trash and returns a boolean status for the operation.
     */
    moveItemToTrash(fullPath: string): boolean;
    /**
     * Open the given external protocol URL in the desktop's default manner. (For
     * example, mailto: URLs in the user's default mail agent).
     */
    openExternal(url: string, options?: OpenExternalOptions): boolean;
    /**
     * Open the given file in the desktop's default manner.
     */
    openItem(fullPath: string): boolean;
    /**
     * Resolves the shortcut link at shortcutPath. An exception will be thrown when any
     * error happens.
     */
    readShortcutLink(shortcutPath: string): ShortcutDetails;
    /**
     * Show the given file in a file manager. If possible, select the file.
     */
    showItemInFolder(fullPath: string): boolean;
    /**
     * Creates or updates a shortcut link at shortcutPath.
     */
    writeShortcutLink(shortcutPath: string, operation: 'create' | 'update' | 'replace', options: ShortcutDetails): boolean;
    /**
     * Creates or updates a shortcut link at shortcutPath.
     */
    writeShortcutLink(shortcutPath: string, options: ShortcutDetails): boolean;
  }

  type ShortcutDetails = {

    // Docs: http://electron.atom.io/docs/api/structures/shortcut-details

    appUserModelId?: string;
    args?: string;
    cwd?: string;
    description?: string;
    icon?: string;
    iconIndex?: number;
    target: string;
  }

  interface SystemPreferences extends EventEmitter {

    // Docs: http://electron.atom.io/docs/api/system-preferences

    on(event: 'accent-color-changed', listener: (event: Event,
                                                 /**
                                                  * The new RGBA color the user assigned to be there system accent color.
                                                  */
                                                 newColor: string) => void): this;
    on(event: 'color-changed', listener: (event: Event) => void): this;
    on(event: 'inverted-color-scheme-changed', listener: (event: Event,
                                                          /**
                                                           * `true` if an inverted color scheme, such as a high contrast theme, is being
                                                           * used, `false` otherwise.
                                                           */
                                                          invertedColorScheme: boolean) => void): this;
    getAccentColor(): string;
    getColor(color: '3d-dark-shadow' | '3d-face' | '3d-highlight' | '3d-light' | '3d-shadow' | 'active-border' | 'active-caption' | 'active-caption-gradient' | 'app-workspace' | 'button-text' | 'caption-text' | 'desktop' | 'disabled-text' | 'highlight' | 'highlight-text' | 'hotlight' | 'inactive-border' | 'inactive-caption' | 'inactive-caption-gradient' | 'inactive-caption-text' | 'info-background' | 'info-text' | 'menu' | 'menu-highlight' | 'menubar' | 'menu-text' | 'scrollbar' | 'window' | 'window-frame' | 'window-text'): string;
    /**
     * Get the value of key in system preferences. This API reads from NSUserDefaults
     * on macOS, some popular key and types are:
     */
    getUserDefault(key: string, type: 'string' | 'boolean' | 'integer' | 'float' | 'double' | 'url' | 'array' | 'dictionary'): void;
    /**
     * This method returns true if DWM composition (Aero Glass) is enabled, and false
     * otherwise. An example of using it to determine if you should create a
     * transparent window or not (transparent windows won't work correctly when DWM
     * composition is disabled):
     */
    isAeroGlassEnabled(): void;
    isDarkMode(): boolean;
    isInvertedColorScheme(): boolean;
    isSwipeTrackingFromScrollEventsEnabled(): boolean;
    /**
     * Posts event as native notifications of macOS. The userInfo is an Object that
     * contains the user information dictionary sent along with the notification.
     */
    postLocalNotification(event: string, userInfo: any): void;
    /**
     * Posts event as native notifications of macOS. The userInfo is an Object that
     * contains the user information dictionary sent along with the notification.
     */
    postNotification(event: string, userInfo: any): void;
    /**
     * Same as subscribeNotification, but uses NSNotificationCenter for local defaults.
     * This is necessary for events such as NSUserDefaultsDidChangeNotification
     */
    subscribeLocalNotification(event: string, callback: (event: string, userInfo: any) => void): void;
    /**
     * Subscribes to native notifications of macOS, callback will be called with
     * callback(event, userInfo) when the corresponding event happens. The userInfo is
     * an Object that contains the user information dictionary sent along with the
     * notification. The id of the subscriber is returned, which can be used to
     * unsubscribe the event. Under the hood this API subscribes to
     * NSDistributedNotificationCenter, example values of event are:
     */
    subscribeNotification(event: string, callback: (event: string, userInfo: any) => void): void;
    /**
     * Same as unsubscribeNotification, but removes the subscriber from
     * NSNotificationCenter.
     */
    unsubscribeLocalNotification(id: number): void;
    /**
     * Removes the subscriber with id.
     */
    unsubscribeNotification(id: number): void;
  }

  type Task = {

    // Docs: http://electron.atom.io/docs/api/structures/task

    arguments: string;
    description: string;
    iconIndex: number;
    iconPath: string;
    program: string;
    title: string;
  }

  type ThumbarButton = {

    // Docs: http://electron.atom.io/docs/api/structures/thumbar-button

    click: Function;
    flags?: string[];
    icon: NativeImage;
    tooltip?: string;
  }

  class Tray extends EventEmitter {

    // Docs: http://electron.atom.io/docs/api/tray

    /**
     * Emitted when the tray balloon is clicked.
     */
    on(event: 'balloon-click', listener: Function): this;
    /**
     * Emitted when the tray balloon is closed because of timeout or user manually
     * closes it.
     */
    on(event: 'balloon-closed', listener: Function): this;
    /**
     * Emitted when the tray balloon shows.
     */
    on(event: 'balloon-show', listener: Function): this;
    /**
     * Emitted when the tray icon is clicked.
     */
    on(event: 'click', listener: (event: Event,
                                  /**
                                   * The bounds of tray icon
                                   */
                                  bounds: Rectangle) => void): this;
    /**
     * Emitted when the tray icon is double clicked.
     */
    on(event: 'double-click', listener: (event: Event,
                                         /**
                                          * The bounds of tray icon
                                          */
                                         bounds: Rectangle) => void): this;
    /**
     * Emitted when a drag operation ends on the tray or ends at another location.
     */
    on(event: 'drag-end', listener: Function): this;
    /**
     * Emitted when a drag operation enters the tray icon.
     */
    on(event: 'drag-enter', listener: Function): this;
    /**
     * Emitted when a drag operation exits the tray icon.
     */
    on(event: 'drag-leave', listener: Function): this;
    /**
     * Emitted when any dragged items are dropped on the tray icon.
     */
    on(event: 'drop', listener: Function): this;
    /**
     * Emitted when dragged files are dropped in the tray icon.
     */
    on(event: 'drop-files', listener: (event: Event,
                                       /**
                                        * The paths of the dropped files.
                                        */
                                       files: string[]) => void): this;
    /**
     * Emitted when dragged text is dropped in the tray icon.
     */
    on(event: 'drop-text', listener: (event: Event,
                                      /**
                                       * the dropped text string
                                       */
                                      text: string) => void): this;
    /**
     * Emitted when the tray icon is right clicked.
     */
    on(event: 'right-click', listener: (event: Event,
                                        /**
                                         * The bounds of tray icon
                                         */
                                        bounds: Rectangle) => void): this;
    constructor(image: NativeImage);
    /**
     * Destroys the tray icon immediately.
     */
    destroy(): void;
    /**
     * Displays a tray balloon.
     */
    displayBalloon(options: DisplayBalloonOptions): void;
    /**
     * The bounds of this tray icon as Object.
     */
    getBounds(): Rectangle;
    isDestroyed(): boolean;
    /**
     * Pops up the context menu of the tray icon. When menu is passed, the menu will be
     * shown instead of the tray icon's context menu. The position is only available on
     * Windows, and it is (0, 0) by default.
     */
    popUpContextMenu(menu?: Menu, position?: Position): void;
    /**
     * Sets the context menu for this icon.
     */
    setContextMenu(menu: Menu): void;
    /**
     * Sets when the tray's icon background becomes highlighted (in blue). Note: You
     * can use highlightMode with a BrowserWindow by toggling between 'never' and
     * 'always' modes when the window visibility changes.
     */
    setHighlightMode(mode: 'selection' | 'always' | 'never'): void;
    /**
     * Sets the image associated with this tray icon.
     */
    setImage(image: NativeImage): void;
    /**
     * Sets the image associated with this tray icon when pressed on macOS.
     */
    setPressedImage(image: NativeImage): void;
    /**
     * Sets the title displayed aside of the tray icon in the status bar.
     */
    setTitle(title: string): void;
    /**
     * Sets the hover text for this tray icon.
     */
    setToolTip(toolTip: string): void;
  }

  type UploadData = {

    // Docs: http://electron.atom.io/docs/api/structures/upload-data

    blobUUID: string;
    bytes: Buffer;
    file: string;
  }

  class WebContents extends EventEmitter {

    // Docs: http://electron.atom.io/docs/api/web-contents

    static fromId(id: number): WebContents;
    static getAllWebContents(): WebContents[];
    static getFocusedWebContents(): WebContents;
    /**
     * Emitted when failed to verify the certificate for url. The usage is the same
     * with the certificate-error event of app.
     */
    on(event: 'certificate-error', listener: (event: Event,
                                              url: URL,
                                              /**
                                               * The error code
                                               */
                                              error: string,
                                              certificate: Certificate,
                                              callback: Function) => void): this;
    /**
     * Emitted when there is a new context menu that needs to be handled.
     */
    on(event: 'context-menu', listener: (event: Event,
                                         params: ContextMenuParams) => void): this;
    /**
     * Emitted when the renderer process crashes or is killed.
     */
    on(event: 'crashed', listener: (event: Event,
                                    killed: boolean) => void): this;
    /**
     * Emitted when the cursor's type changes. The type parameter can be default,
     * crosshair, pointer, text, wait, help, e-resize, n-resize, ne-resize, nw-resize,
     * s-resize, se-resize, sw-resize, w-resize, ns-resize, ew-resize, nesw-resize,
     * nwse-resize, col-resize, row-resize, m-panning, e-panning, n-panning,
     * ne-panning, nw-panning, s-panning, se-panning, sw-panning, w-panning, move,
     * vertical-text, cell, context-menu, alias, progress, nodrop, copy, none,
     * not-allowed, zoom-in, zoom-out, grab, grabbing, custom. If the type parameter is
     * custom, the image parameter will hold the custom cursor image in a NativeImage,
     * and scale, size and hotspot will hold additional information about the custom
     * cursor.
     */
    on(event: 'cursor-changed', listener: (event: Event,
                                           type: string,
                                           image?: NativeImage,
                                           /**
                                            * scaling factor for the custom cursor
                                            */
                                           scale?: number,
                                           /**
                                            * the size of the `image`
                                            */
                                           size?: Size,
                                           /**
                                            * coordinates of the custom cursor's hotspot
                                            */
                                           hotspot?: Hotspot) => void): this;
    /**
     * Emitted when webContents is destroyed.
     */
    on(event: 'destroyed', listener: Function): this;
    /**
     * Emitted when DevTools is closed.
     */
    on(event: 'devtools-closed', listener: Function): this;
    /**
     * Emitted when DevTools is focused / opened.
     */
    on(event: 'devtools-focused', listener: Function): this;
    /**
     * Emitted when DevTools is opened.
     */
    on(event: 'devtools-opened', listener: Function): this;
    /**
     * Emitted when a page's theme color changes. This is usually due to encountering a
     * meta tag:
     */
    on(event: 'did-change-theme-color', listener: Function): this;
    /**
     * This event is like did-finish-load but emitted when the load failed or was
     * cancelled, e.g. window.stop() is invoked. The full list of error codes and their
     * meaning is available here. Note that redirect responses will emit errorCode -3;
     * you may want to ignore that error explicitly.
     */
    on(event: 'did-fail-load', listener: (event: Event,
                                          errorCode: number,
                                          errorDescription: string,
                                          validatedURL: string,
                                          isMainFrame: boolean) => void): this;
    /**
     * Emitted when the navigation is done, i.e. the spinner of the tab has stopped
     * spinning, and the onload event was dispatched.
     */
    on(event: 'did-finish-load', listener: Function): this;
    /**
     * Emitted when a frame has done navigation.
     */
    on(event: 'did-frame-finish-load', listener: (event: Event,
                                                  isMainFrame: boolean) => void): this;
    /**
     * Emitted when a redirect is received while requesting a resource.
     */
    on(event: 'did-get-redirect-request', listener: (event: Event,
                                                     oldURL: string,
                                                     newURL: string,
                                                     isMainFrame: boolean,
                                                     httpResponseCode: number,
                                                     requestMethod: string,
                                                     referrer: string,
                                                     headers: any) => void): this;
    /**
     * Emitted when details regarding a requested resource are available. status
     * indicates the socket connection to download the resource.
     */
    on(event: 'did-get-response-details', listener: (event: Event,
                                                     status: boolean,
                                                     newURL: string,
                                                     originalURL: string,
                                                     httpResponseCode: number,
                                                     requestMethod: string,
                                                     referrer: string,
                                                     headers: any,
                                                     resourceType: string) => void): this;
    /**
     * Emitted when a navigation is done. This event is not emitted for in-page
     * navigations, such as clicking anchor links or updating the window.location.hash.
     * Use did-navigate-in-page event for this purpose.
     */
    on(event: 'did-navigate', listener: (event: Event,
                                         url: string) => void): this;
    /**
     * Emitted when an in-page navigation happened. When in-page navigation happens,
     * the page URL changes but does not cause navigation outside of the page. Examples
     * of this occurring are when anchor links are clicked or when the DOM hashchange
     * event is triggered.
     */
    on(event: 'did-navigate-in-page', listener: (event: Event,
                                                 url: string,
                                                 isMainFrame: boolean) => void): this;
    /**
     * Corresponds to the points in time when the spinner of the tab started spinning.
     */
    on(event: 'did-start-loading', listener: Function): this;
    /**
     * Corresponds to the points in time when the spinner of the tab stopped spinning.
     */
    on(event: 'did-stop-loading', listener: Function): this;
    /**
     * Emitted when the document in the given frame is loaded.
     */
    on(event: 'dom-ready', listener: (event: Event) => void): this;
    /**
     * Emitted when a result is available for webContents.findInPage request.
     */
    on(event: 'found-in-page', listener: (event: Event,
                                          result: Result) => void): this;
    /**
     * Emitted when webContents wants to do basic auth. The usage is the same with the
     * login event of app.
     */
    on(event: 'login', listener: (event: Event,
                                  request: Request,
                                  authInfo: AuthInfo,
                                  callback: Function) => void): this;
    /**
     * Emitted when media is paused or done playing.
     */
    on(event: 'media-paused', listener: Function): this;
    /**
     * Emitted when media starts playing.
     */
    on(event: 'media-started-playing', listener: Function): this;
    /**
     * Emitted when the page requests to open a new window for a url. It could be
     * requested by window.open or an external link like <a target='_blank'>. By
     * default a new BrowserWindow will be created for the url. Calling
     * event.preventDefault() will prevent creating new windows. In such case, the
     * event.newGuest may be set with a reference to a BrowserWindow instance to make
     * it used by the Electron's runtime.
     */
    on(event: 'new-window', listener: (event: Event,
                                       url: string,
                                       frameName: string,
                                       /**
                                        * Can be `default`, `foreground-tab`, `background-tab`, `new-window`,
                                        * `save-to-disk` and `other`.
                                        */
                                       disposition: string,
                                       /**
                                        * The options which will be used for creating the new `BrowserWindow`.
                                        */
                                       options: any,
                                       /**
                                        * The non-standard features (features not handled by Chromium or Electron) given
                                        * to `window.open()`.
                                        */
                                       additionalFeatures: string[]) => void): this;
    /**
     * Emitted when page receives favicon urls.
     */
    on(event: 'page-favicon-updated', listener: (event: Event,
                                                 /**
                                                  * Array of URLs
                                                  */
                                                 favicons: string[]) => void): this;
    /**
     * Emitted when a new frame is generated. Only the dirty area is passed in the
     * buffer.
     */
    on(event: 'paint', listener: (event: Event,
                                  dirtyRect: Rectangle,
                                  /**
                                   * The image data of the whole frame.
                                   */
                                  image: NativeImage) => void): this;
    /**
     * Emitted when a plugin process has crashed.
     */
    on(event: 'plugin-crashed', listener: (event: Event,
                                           name: string,
                                           version: string) => void): this;
    /**
     * Emitted when bluetooth device needs to be selected on call to
     * navigator.bluetooth.requestDevice. To use navigator.bluetooth api webBluetooth
     * should be enabled.  If event.preventDefault is not called, first available
     * device will be selected. callback should be called with deviceId to be selected,
     * passing empty string to callback will cancel the request.
     */
    on(event: 'select-bluetooth-device', listener: (event: Event,
                                                    devices: BluetoothDevice[],
                                                    callback: Function) => void): this;
    /**
     * Emitted when a client certificate is requested. The usage is the same with the
     * select-client-certificate event of app.
     */
    on(event: 'select-client-certificate', listener: (event: Event,
                                                      url: URL,
                                                      certificateList: Certificate[],
                                                      callback: Function) => void): this;
    /**
     * Emitted when mouse moves over a link or the keyboard moves the focus to a link.
     */
    on(event: 'update-target-url', listener: (event: Event,
                                              url: string) => void): this;
    /**
     * Emitted when a user or the page wants to start navigation. It can happen when
     * the window.location object is changed or a user clicks a link in the page. This
     * event will not emit when the navigation is started programmatically with APIs
     * like webContents.loadURL and webContents.back. It is also not emitted for
     * in-page navigations, such as clicking anchor links or updating the
     * window.location.hash. Use did-navigate-in-page event for this purpose. Calling
     * event.preventDefault() will prevent the navigation.
     */
    on(event: 'will-navigate', listener: (event: Event,
                                          url: string) => void): this;
    /**
     * Adds the specified path to DevTools workspace. Must be used after DevTools
     * creation:
     */
    addWorkSpace(path: string): void;
    /**
     * Begin subscribing for presentation events and captured frames, the callback will
     * be called with callback(frameBuffer, dirtyRect) when there is a presentation
     * event. The frameBuffer is a Buffer that contains raw pixel data. On most
     * machines, the pixel data is effectively stored in 32bit BGRA format, but the
     * actual representation depends on the endianness of the processor (most modern
     * processors are little-endian, on machines with big-endian processors the data is
     * in 32bit ARGB format). The dirtyRect is an object with x, y, width, height
     * properties that describes which part of the page was repainted. If onlyDirty is
     * set to true, frameBuffer will only contain the repainted area. onlyDirty
     * defaults to false.
     */
    beginFrameSubscription(callback: (frameBuffer: Buffer, dirtyRect: Rectangle) => void): void;
    /**
     * Begin subscribing for presentation events and captured frames, the callback will
     * be called with callback(frameBuffer, dirtyRect) when there is a presentation
     * event. The frameBuffer is a Buffer that contains raw pixel data. On most
     * machines, the pixel data is effectively stored in 32bit BGRA format, but the
     * actual representation depends on the endianness of the processor (most modern
     * processors are little-endian, on machines with big-endian processors the data is
     * in 32bit ARGB format). The dirtyRect is an object with x, y, width, height
     * properties that describes which part of the page was repainted. If onlyDirty is
     * set to true, frameBuffer will only contain the repainted area. onlyDirty
     * defaults to false.
     */
    beginFrameSubscription(onlyDirty: boolean, callback: (frameBuffer: Buffer, dirtyRect: Rectangle) => void): void;
    canGoBack(): boolean;
    canGoForward(): boolean;
    canGoToOffset(offset: number): boolean;
    /**
     * Captures a snapshot of the page within rect. Upon completion callback will be
     * called with callback(image). The image is an instance of NativeImage that stores
     * data of the snapshot. Omitting rect will capture the whole visible page.
     */
    capturePage(rect: Rectangle, callback: (image: NativeImage) => void): void;
    /**
     * Captures a snapshot of the page within rect. Upon completion callback will be
     * called with callback(image). The image is an instance of NativeImage that stores
     * data of the snapshot. Omitting rect will capture the whole visible page.
     */
    capturePage(callback: (image: NativeImage) => void): void;
    /**
     * Clears the navigation history.
     */
    clearHistory(): void;
    /**
     * Closes the devtools.
     */
    closeDevTools(): void;
    /**
     * Executes the editing command copy in web page.
     */
    copy(): void;
    /**
     * Copy the image at the given position to the clipboard.
     */
    copyImageAt(x: number, y: number): void;
    /**
     * Executes the editing command cut in web page.
     */
    cut(): void;
    /**
     * Executes the editing command delete in web page.
     */
    delete(): void;
    /**
     * Disable device emulation enabled by webContents.enableDeviceEmulation.
     */
    disableDeviceEmulation(): void;
    /**
     * Initiates a download of the resource at url without navigating. The
     * will-download event of session will be triggered.
     */
    downloadURL(url: string): void;
    /**
     * Enable device emulation with the given parameters.
     */
    enableDeviceEmulation(parameters: Parameters): void;
    /**
     * End subscribing for frame presentation events.
     */
    endFrameSubscription(): void;
    /**
     * Evaluates code in page. In the browser window some HTML APIs like
     * requestFullScreen can only be invoked by a gesture from the user. Setting
     * userGesture to true will remove this limitation.
     */
    executeJavaScript(code: string, userGesture?: boolean, callback?: (result: any) => void): void;
    /**
     * Starts a request to find all matches for the text in the web page and returns an
     * Integer representing the request id used for the request. The result of the
     * request can be obtained by subscribing to found-in-page event.
     */
    findInPage(text: string, options?: FindInPageOptions): void;
    getFrameRate(): number;
    getTitle(): string;
    getURL(): string;
    getUserAgent(): string;
    /**
     * Sends a request to get current zoom factor, the callback will be called with
     * callback(zoomFactor).
     */
    getZoomFactor(callback: (zoomFactor: number) => void): void;
    /**
     * Sends a request to get current zoom level, the callback will be called with
     * callback(zoomLevel).
     */
    getZoomLevel(callback: (zoomLevel: number) => void): void;
    /**
     * Makes the browser go back a web page.
     */
    goBack(): void;
    /**
     * Makes the browser go forward a web page.
     */
    goForward(): void;
    /**
     * Navigates browser to the specified absolute web page index.
     */
    goToIndex(index: number): void;
    /**
     * Navigates to the specified offset from the "current entry".
     */
    goToOffset(offset: number): void;
    /**
     * Checks if any ServiceWorker is registered and returns a boolean as response to
     * callback.
     */
    hasServiceWorker(callback: (hasWorker: boolean) => void): void;
    /**
     * Injects CSS into the current web page.
     */
    insertCSS(css: string): void;
    /**
     * Inserts text to the focused element.
     */
    insertText(text: string): void;
    /**
     * Starts inspecting element at position (x, y).
     */
    inspectElement(x: number, y: number): void;
    /**
     * Opens the developer tools for the service worker context.
     */
    inspectServiceWorker(): void;
    /**
     * If offscreen rendering is enabled invalidates the frame and generates a new one
     * through the 'paint' event.
     */
    invalidate(): void;
    isAudioMuted(): boolean;
    isCrashed(): boolean;
    isDestroyed(): boolean;
    isDevToolsFocused(): boolean;
    isDevToolsOpened(): boolean;
    isFocused(): boolean;
    isLoading(): boolean;
    isLoadingMainFrame(): boolean;
    isOffscreen(): boolean;
    isPainting(): boolean;
    isWaitingForResponse(): boolean;
    /**
     * Loads the url in the window. The url must contain the protocol prefix, e.g. the
     * http:// or file://. If the load should bypass http cache then use the pragma
     * header to achieve it.
     */
    loadURL(url: URL, options?: LoadURLOptions): void;
    /**
     * Opens the devtools.
     */
    openDevTools(options?: OpenDevToolsOptions): void;
    /**
     * Executes the editing command paste in web page.
     */
    paste(): void;
    /**
     * Executes the editing command pasteAndMatchStyle in web page.
     */
    pasteAndMatchStyle(): void;
    /**
     * Prints window's web page. When silent is set to true, Electron will pick up
     * system's default printer and default settings for printing. Calling
     * window.print() in web page is equivalent to calling webContents.print({silent:
     * false, printBackground: false}). Use page-break-before: always; CSS style to
     * force to print to a new page.
     */
    print(options?: PrintOptions): void;
    /**
     * Prints window's web page as PDF with Chromium's preview printing custom
     * settings. The callback will be called with callback(error, data) on completion.
     * The data is a Buffer that contains the generated PDF data. By default, an empty
     * options will be regarded as: Use page-break-before: always; CSS style to force
     * to print to a new page. An example of webContents.printToPDF:
     */
    printToPDF(options: PrintToPDFOptions, callback: (error: Error, data: Buffer) => void): void;
    /**
     * Executes the editing command redo in web page.
     */
    redo(): void;
    /**
     * Reloads the current web page.
     */
    reload(): void;
    /**
     * Reloads current page and ignores cache.
     */
    reloadIgnoringCache(): void;
    /**
     * Removes the specified path from DevTools workspace.
     */
    removeWorkSpace(path: string): void;
    /**
     * Executes the editing command replace in web page.
     */
    replace(text: string): void;
    /**
     * Executes the editing command replaceMisspelling in web page.
     */
    replaceMisspelling(text: string): void;
    /**
     * Returns true if the process of saving page has been initiated successfully.
     */
    savePage(fullPath: string, saveType: 'HTMLOnly' | 'HTMLComplete' | 'MHTML', callback: (error: Error) => void): void;
    /**
     * Executes the editing command selectAll in web page.
     */
    selectAll(): void;
    /**
     * Send an asynchronous message to renderer process via channel, you can also send
     * arbitrary arguments. Arguments will be serialized in JSON internally and hence
     * no functions or prototype chain will be included. The renderer process can
     * handle the message by listening to channel with the ipcRenderer module. An
     * example of sending messages from the main process to the renderer process:
     */
    send(channel: string): void;
    /**
     * Sends an input event to the page. For keyboard events, the event object also
     * have following properties: For mouse events, the event object also have
     * following properties: For the mouseWheel event, the event object also have
     * following properties:
     */
    sendInputEvent(event: Event): void;
    /**
     * Mute the audio on the current web page.
     */
    setAudioMuted(muted: boolean): void;
    /**
     * If offscreen rendering is enabled sets the frame rate to the specified number.
     * Only values between 1 and 60 are accepted.
     */
    setFrameRate(fps: number): void;
    /**
     * Overrides the user agent for this web page.
     */
    setUserAgent(userAgent: string): void;
    /**
     * Changes the zoom factor to the specified factor. Zoom factor is zoom percent
     * divided by 100, so 300% = 3.0.
     */
    setZoomFactor(factor: number): void;
    /**
     * Changes the zoom level to the specified level. The original size is 0 and each
     * increment above or below represents zooming 20% larger or smaller to default
     * limits of 300% and 50% of original size, respectively.
     */
    setZoomLevel(level: number): void;
    /**
     * Sets the maximum and minimum zoom level.
     */
    setZoomLevelLimits(minimumLevel: number, maximumLevel: number): void;
    /**
     * Shows pop-up dictionary that searches the selected word on the page.
     */
    showDefinitionForSelection(): void;
    /**
     * Sets the item as dragging item for current drag-drop operation, file is the
     * absolute path of the file to be dragged, and icon is the image showing under the
     * cursor when dragging.
     */
    startDrag(item: Item): void;
    /**
     * If offscreen rendering is enabled and not painting, start painting.
     */
    startPainting(): void;
    /**
     * Stops any pending navigation.
     */
    stop(): void;
    /**
     * Stops any findInPage request for the webContents with the provided action.
     */
    stopFindInPage(action: 'clearSelection' | 'keepSelection' | 'activateSelection'): void;
    /**
     * If offscreen rendering is enabled and painting, stop painting.
     */
    stopPainting(): void;
    /**
     * Toggles the developer tools.
     */
    toggleDevTools(): void;
    /**
     * Executes the editing command undo in web page.
     */
    undo(): void;
    /**
     * Unregisters any ServiceWorker if present and returns a boolean as response to
     * callback when the JS promise is fulfilled or false when the JS promise is
     * rejected.
     */
    unregisterServiceWorker(callback: (success: boolean) => void): void;
    /**
     * Executes the editing command unselect in web page.
     */
    unselect(): void;
    debugger: Debugger;
    devToolsWebContents: WebContents;
    hostWebContents: WebContents;
    id: number;
    session: Session;
  }

  interface WebFrame extends EventEmitter {

    // Docs: http://electron.atom.io/docs/api/web-frame

    /**
     * Attempts to free memory that is no longer being used (like images from a
     * previous navigation). Note that blindly calling this method probably makes
     * Electron slower since it will have to refill these emptied caches, you should
     * only call it if an event in your app has occurred that makes you think your page
     * is actually using less memory (i.e. you have navigated from a super heavy page
     * to a mostly empty one, and intend to stay there).
     */
    clearCache(): void;
    /**
     * Evaluates code in page. In the browser window some HTML APIs like
     * requestFullScreen can only be invoked by a gesture from the user. Setting
     * userGesture to true will remove this limitation.
     */
    executeJavaScript(code: string, userGesture?: boolean): void;
    /**
     * Returns an object describing usage information of Blink's internal memory
     * caches. This will generate:
     */
    getResourceUsage(): ResourceUsage;
    getZoomFactor(): number;
    getZoomLevel(): number;
    /**
     * Inserts text to the focused element.
     */
    insertText(text: string): void;
    /**
     * Resources will be loaded from this scheme regardless of the current page's
     * Content Security Policy.
     */
    registerURLSchemeAsBypassingCSP(scheme: string): void;
    /**
     * Registers the scheme as secure, bypasses content security policy for resources,
     * allows registering ServiceWorker and supports fetch API. Specify an option with
     * the value of false to omit it from the registration. An example of registering a
     * privileged scheme, without bypassing Content Security Policy:
     */
    registerURLSchemeAsPrivileged(scheme: string, options?: RegisterURLSchemeAsPrivilegedOptions): void;
    /**
     * Registers the scheme as secure scheme. Secure schemes do not trigger mixed
     * content warnings. For example, https and data are secure schemes because they
     * cannot be corrupted by active network attackers.
     */
    registerURLSchemeAsSecure(scheme: string): void;
    /**
     * Sets a provider for spell checking in input fields and text areas. The provider
     * must be an object that has a spellCheck method that returns whether the word
     * passed is correctly spelled. An example of using node-spellchecker as provider:
     */
    setSpellCheckProvider(language: string, autoCorrectWord: boolean, provider: any): void;
    /**
     * Changes the zoom factor to the specified factor. Zoom factor is zoom percent
     * divided by 100, so 300% = 3.0.
     */
    setZoomFactor(factor: number): void;
    /**
     * Changes the zoom level to the specified level. The original size is 0 and each
     * increment above or below represents zooming 20% larger or smaller to default
     * limits of 300% and 50% of original size, respectively.
     */
    setZoomLevel(level: number): void;
    /**
     * Sets the maximum and minimum zoom level.
     */
    setZoomLevelLimits(minimumLevel: number, maximumLevel: number): void;
  }

  class WebRequest extends EventEmitter {

    // Docs: http://electron.atom.io/docs/api/web-request

    /**
     * The listener will be called with listener(details) when a server initiated
     * redirect is about to occur.
     */
    onBeforeRedirect(filter: any, listener: (details: OnBeforeRedirectDetails) => void): void;
    /**
     * The listener will be called with listener(details, callback) when a request is
     * about to occur. The uploadData is an array of UploadData objects. The callback
     * has to be called with an response object.
     */
    onBeforeRequest(filter: any, listener: (details: OnBeforeRequestDetails, callback: (response: Response) => void) => void): void;
    /**
     * The listener will be called with listener(details, callback) before sending an
     * HTTP request, once the request headers are available. This may occur after a TCP
     * connection is made to the server, but before any http data is sent. The callback
     * has to be called with an response object.
     */
    onBeforeSendHeaders(filter: any, listener: Function): void;
    /**
     * The listener will be called with listener(details) when a request is completed.
     */
    onCompleted(filter: any, listener: (details: OnCompletedDetails) => void): void;
    /**
     * The listener will be called with listener(details) when an error occurs.
     */
    onErrorOccurred(filter: any, listener: (details: OnErrorOccurredDetails) => void): void;
    /**
     * The listener will be called with listener(details, callback) when HTTP response
     * headers of a request have been received. The callback has to be called with an
     * response object.
     */
    onHeadersReceived(filter: any, listener: Function): void;
    /**
     * The listener will be called with listener(details) when first byte of the
     * response body is received. For HTTP requests, this means that the status line
     * and response headers are available.
     */
    onResponseStarted(filter: any, listener: (details: OnResponseStartedDetails) => void): void;
    /**
     * The listener will be called with listener(details) just before a request is
     * going to be sent to the server, modifications of previous onBeforeSendHeaders
     * response are visible by the time this listener is fired.
     */
    onSendHeaders(filter: any, listener: (details: OnSendHeadersDetails) => void): void;
  }

  interface AboutPanelOptionsOptions {
    /**
     * The app's name.
     */
    applicationName?: string;
    /**
     * The app's version.
     */
    applicationVersion?: string;
    /**
     * Copyright information.
     */
    copyright?: string;
    /**
     * Credit information.
     */
    credits?: string;
    /**
     * The app's build version number.
     */
    version?: string;
  }

  interface AuthInfo {
    isProxy: boolean;
    scheme: string;
    host: string;
    port: number;
    realm: string;
  }

  interface BrowserWindowConstructorOptions {
    /**
     * Window's width in pixels. Default is .
     */
    width: number;
    /**
     * Window's height in pixels. Default is .
     */
    height: number;
    /**
     * ( if y is used) - Window's left offset from screen. Default is to center the
     * window.
     */
    x?: number;
    /**
     * ( if x is used) - Window's top offset from screen. Default is to center the
     * window.
     */
    y?: number;
    /**
     * The and would be used as web page's size, which means the actual window's size
     * will include window frame's size and be slightly larger. Default is .
     */
    useContentSize: boolean;
    /**
     * Show window in the center of the screen.
     */
    center: boolean;
    /**
     * Window's minimum width. Default is .
     */
    minWidth: number;
    /**
     * Window's minimum height. Default is .
     */
    minHeight: number;
    /**
     * Window's maximum width. Default is no limit.
     */
    maxWidth: number;
    /**
     * Window's maximum height. Default is no limit.
     */
    maxHeight: number;
    /**
     * Whether window is resizable. Default is .
     */
    resizable: boolean;
    /**
     * Whether window is movable. This is not implemented on Linux. Default is .
     */
    movable: boolean;
    /**
     * Whether window is minimizable. This is not implemented on Linux. Default is .
     */
    minimizable: boolean;
    /**
     * Whether window is maximizable. This is not implemented on Linux. Default is .
     */
    maximizable: boolean;
    /**
     * Whether window is closable. This is not implemented on Linux. Default is .
     */
    closable: boolean;
    /**
     * Whether the window can be focused. Default is . On Windows setting also implies
     * setting . On Linux setting makes the window stop interacting with wm, so the
     * window will always stay on top in all workspaces.
     */
    focusable: boolean;
    /**
     * Whether the window should always stay on top of other windows. Default is .
     */
    alwaysOnTop: boolean;
    /**
     * Whether the window should show in fullscreen. When explicitly set to the
     * fullscreen button will be hidden or disabled on macOS. Default is .
     */
    fullscreen: boolean;
    /**
     * Whether the window can be put into fullscreen mode. On macOS, also whether the
     * maximize/zoom button should toggle full screen mode or maximize window. Default
     * is .
     */
    fullscreenable: boolean;
    /**
     * Whether to show the window in taskbar. Default is .
     */
    skipTaskbar: boolean;
    /**
     * The kiosk mode. Default is .
     */
    kiosk: boolean;
    /**
     * Default window title. Default is .
     */
    title: string;
    /**
     * The window icon. On Windows it is recommended to use icons to get best visual
     * effects, you can also leave it undefined so the executable's icon will be used.
     */
    icon: NativeImage;
    /**
     * Whether window should be shown when created. Default is .
     */
    show: boolean;
    /**
     * Specify to create a . Default is .
     */
    frame: boolean;
    /**
     * Specify parent window. Default is .
     */
    parent: BrowserWindow;
    /**
     * Whether this is a modal window. This only works when the window is a child
     * window. Default is .
     */
    modal: boolean;
    /**
     * Whether the web view accepts a single mouse-down event that simultaneously
     * activates the window. Default is .
     */
    acceptFirstMouse: boolean;
    /**
     * Whether to hide cursor when typing. Default is .
     */
    disableAutoHideCursor: boolean;
    /**
     * Auto hide the menu bar unless the key is pressed. Default is .
     */
    autoHideMenuBar: boolean;
    /**
     * Enable the window to be resized larger than screen. Default is .
     */
    enableLargerThanScreen: boolean;
    /**
     * Window's background color as Hexadecimal value, like or or (alpha is supported).
     * Default is (white).
     */
    backgroundColor: string;
    /**
     * Whether window should have a shadow. This is only implemented on macOS. Default
     * is .
     */
    hasShadow: boolean;
    /**
     * Forces using dark theme for the window, only works on some GTK+3 desktop
     * environments. Default is .
     */
    darkTheme: boolean;
    /**
     * Makes the window . Default is .
     */
    transparent: boolean;
    /**
     * The type of window, default is normal window. See more about this below.
     */
    type: string;
    /**
     * The style of window title bar. Default is . Possible values are:
     */
    titleBarStyle: string;
    /**
     * Use style for frameless windows on Windows, which adds standard window frame.
     * Setting it to will remove window shadow and window animations. Default is .
     */
    thickFrame: boolean;
    /**
     * Settings of web page's features.
     */
    webPreferences: WebPreferences;
  }

  interface ClearStorageDataOptions {
    /**
     * Should follow ’s representation .
     */
    origin: string;
    /**
     * The types of storages to clear, can contain: , , , , , , ,
     */
    storages: string[];
    /**
     * The types of quotas to clear, can contain: , , .
     */
    quotas: string[];
  }

  interface Config {
    /**
     * The URL associated with the PAC file.
     */
    pacScript: string;
    /**
     * Rules indicating which proxies to use.
     */
    proxyRules: string;
    /**
     * Rules indicating which URLs should bypass the proxy settings.
     */
    proxyBypassRules: string;
  }

  interface ContextMenuParams {
    /**
     * x coordinate
     */
    x: number;
    /**
     * y coordinate
     */
    y: number;
    /**
     * URL of the link that encloses the node the context menu was invoked on.
     */
    linkURL: string;
    /**
     * Text associated with the link. May be an empty string if the contents of the
     * link are an image.
     */
    linkText: string;
    /**
     * URL of the top level page that the context menu was invoked on.
     */
    pageURL: string;
    /**
     * URL of the subframe that the context menu was invoked on.
     */
    frameURL: string;
    /**
     * Source URL for the element that the context menu was invoked on. Elements with
     * source URLs are images, audio and video.
     */
    srcURL: string;
    /**
     * Type of the node the context menu was invoked on. Can be , , , , , or .
     */
    mediaType: string;
    /**
     * Whether the context menu was invoked on an image which has non-empty contents.
     */
    hasImageContents: boolean;
    /**
     * Whether the context is editable.
     */
    isEditable: boolean;
    /**
     * Text of the selection that the context menu was invoked on.
     */
    selectionText: string;
    /**
     * Title or alt text of the selection that the context was invoked on.
     */
    titleText: string;
    /**
     * The misspelled word under the cursor, if any.
     */
    misspelledWord: string;
    /**
     * The character encoding of the frame on which the menu was invoked.
     */
    frameCharset: string;
    /**
     * If the context menu was invoked on an input field, the type of that field.
     * Possible values are , , , .
     */
    inputFieldType: string;
    /**
     * Input source that invoked the context menu. Can be , , , , .
     */
    menuSourceType: string;
    /**
     * The flags for the media element the context menu was invoked on.
     */
    mediaFlags: MediaFlags;
    /**
     * These flags indicate whether the renderer believes it is able to perform the
     * corresponding action.
     */
    editFlags: EditFlags;
  }

  interface CrashReporterStartOptions {
    companyName: string;
    /**
     * URL that crash reports will be sent to as POST.
     */
    submitURL: string;
    /**
     * Defaults to .
     */
    productName?: string;
    /**
     * Send the crash report without user interaction. Default is .
     */
    autoSubmit: boolean;
    /**
     * Default is .
     */
    ignoreSystemCrashHandler: boolean;
    /**
     * An object you can define that will be sent along with the report. Only string
     * properties are sent correctly, Nested objects are not supported.
     */
    extra: Extra;
  }

  interface CursorScreenPoint {
    x: number;
    y: number;
  }

  interface Data {
    text: string;
    html: string;
    image: NativeImage;
    rtf: string;
    /**
     * The title of the url at .
     */
    bookmark: string;
  }

  interface Details {
    /**
     * The url to associate the cookie with.
     */
    url: string;
    /**
     * The name of the cookie. Empty by default if omitted.
     */
    name: string;
    /**
     * The value of the cookie. Empty by default if omitted.
     */
    value: string;
    /**
     * The domain of the cookie. Empty by default if omitted.
     */
    domain: string;
    /**
     * The path of the cookie. Empty by default if omitted.
     */
    path: string;
    /**
     * Whether the cookie should be marked as Secure. Defaults to false.
     */
    secure: boolean;
    /**
     * Whether the cookie should be marked as HTTP only. Defaults to false.
     */
    httpOnly: boolean;
    /**
     * - The expiration date of the cookie as the number of seconds since the UNIX
     * epoch. If omitted then the cookie becomes a session cookie and will not be
     * retained between sessions.
     */
    expirationDate: number;
  }

  interface DevToolsExtensions {
  }

  interface DisplayBalloonOptions {
    icon: NativeImage;
    title: string;
    content: string;
  }

  interface ExtraSize {
    width: number;
    height: number;
  }

  interface Filter {
    /**
     * Retrieves cookies which are associated with . Empty implies retrieving cookies
     * of all urls.
     */
    url?: string;
    /**
     * Filters cookies by name.
     */
    name?: string;
    /**
     * Retrieves cookies whose domains match or are subdomains of
     */
    domain?: string;
    /**
     * Retrieves cookies whose path matches .
     */
    path?: string;
    /**
     * Filters cookies by their Secure property.
     */
    secure?: boolean;
    /**
     * Filters out session or persistent cookies.
     */
    session?: boolean;
  }

  interface FindInPageOptions {
    /**
     * Whether to search forward or backward, defaults to .
     */
    forward: boolean;
    /**
     * Whether the operation is first request or a follow up, defaults to .
     */
    findNext: boolean;
    /**
     * Whether search should be case-sensitive, defaults to .
     */
    matchCase: boolean;
    /**
     * Whether to look only at the start of words. defaults to .
     */
    wordStart: boolean;
    /**
     * When combined with , accepts a match in the middle of a word if the match begins
     * with an uppercase letter followed by a lowercase or non-letter. Accepts several
     * other intra-word matches, defaults to .
     */
    medialCapitalAsWordStart: boolean;
  }

  interface FromPartitionOptions {
    /**
     * Whether to enable cache.
     */
    cache: boolean;
  }

  interface Hotspot {
    /**
     * x coordinate
     */
    x: number;
    /**
     * y coordinate
     */
    y: number;
  }

  interface ImportCertificateOptions {
    /**
     * Path for the pkcs12 file.
     */
    certificate: string;
    /**
     * Passphrase for the certificate.
     */
    password: string;
  }

  interface InterceptBufferProtocolRequest {
    url: string;
    referrer: string;
    method: string;
    uploadData: UploadData[];
  }

  interface InterceptFileProtocolRequest {
    url: string;
    referrer: string;
    method: string;
    uploadData: UploadData[];
  }

  interface InterceptHttpProtocolRequest {
    url: string;
    referrer: string;
    method: string;
    uploadData: UploadData[];
  }

  interface InterceptStringProtocolRequest {
    url: string;
    referrer: string;
    method: string;
    uploadData: UploadData[];
  }

  interface Item {
    file: string;
    icon: NativeImage;
  }

  interface JumpListSettings {
    /**
     * The minimum number of items that will be shown in the Jump List (for a more
     * detailed description of this value see the ).
     */
    minItems: number;
    /**
     * Array of objects that correspond to items that the user has explicitly removed
     * from custom categories in the Jump List. These items must not be re-added to the
     * Jump List in the call to , Windows will not display any custom category that
     * contains any of the removed items.
     */
    removedItems: JumpListItem[];
  }

  interface LastCrashReport {
    date: string;
    ID: number;
  }

  interface LoadURLOptions {
    /**
     * A HTTP Referrer url.
     */
    httpReferrer: string;
    /**
     * A user agent originating the request.
     */
    userAgent: string;
    /**
     * Extra headers separated by "\n"
     */
    extraHeaders: string;
  }

  interface LoginItemSettings {
    /**
     * if the app is set to open at login.
     */
    openAtLogin: boolean;
    /**
     * if the app is set to open as hidden at login. This setting is only supported on
     * macOS.
     */
    openAsHidden: boolean;
    /**
     * if the app was opened at login automatically. This setting is only supported on
     * macOS.
     */
    wasOpenedAtLogin: boolean;
    /**
     * if the app was opened as a hidden login item. This indicates that the app should
     * not open any windows at startup. This setting is only supported on macOS.
     */
    wasOpenedAsHidden: boolean;
    /**
     * if the app was opened as a login item that should restore the state from the
     * previous session. This indicates that the app should restore the windows that
     * were open the last time the app was closed. This setting is only supported on
     * macOS.
     */
    restoreState: boolean;
  }

  interface MenuItemConstructorOptions {
    /**
     * Will be called with when the menu item is clicked.
     */
    click: (menuItem: MenuItem, browserWindow: BrowserWindow, event: Event) => void;
    /**
     * Define the action of the menu item, when specified the property will be ignored.
     */
    role: string;
    /**
     * Can be , , , or .
     */
    type: string;
    label: string;
    sublabel: string;
    accelerator: Accelerator;
    icon: NativeImage;
    /**
     * If false, the menu item will be greyed out and unclickable.
     */
    enabled: boolean;
    /**
     * If false, the menu item will be entirely hidden.
     */
    visible: boolean;
    /**
     * Should only be specified for or type menu items.
     */
    checked: boolean;
    /**
     * Should be specified for type menu items. If is specified, the can be omitted. If
     * the value is not a then it will be automatically converted to one using .
     */
    submenu: Menu;
    /**
     * Unique within a single menu. If defined then it can be used as a reference to
     * this item by the position attribute.
     */
    id: string;
    /**
     * This field allows fine-grained definition of the specific location within a
     * given menu.
     */
    position: string;
  }

  interface MessageBoxOptions {
    /**
     * Can be , , , or . On Windows, "question" displays the same icon as "info",
     * unless you set an icon using the "icon" option.
     */
    type: string;
    /**
     * Array of texts for buttons. On Windows, an empty array will result in one button
     * labeled "OK".
     */
    buttons: string[];
    /**
     * Index of the button in the buttons array which will be selected by default when
     * the message box opens.
     */
    defaultId: number;
    /**
     * Title of the message box, some platforms will not show it.
     */
    title: string;
    /**
     * Content of the message box.
     */
    message: string;
    /**
     * Extra information of the message.
     */
    detail: string;
    icon: NativeImage;
    /**
     * The value will be returned when user cancels the dialog instead of clicking the
     * buttons of the dialog. By default it is the index of the buttons that have
     * "cancel" or "no" as label, or 0 if there is no such buttons. On macOS and
     * Windows the index of "Cancel" button will always be used as , not matter whether
     * it is already specified.
     */
    cancelId: number;
    /**
     * On Windows Electron will try to figure out which one of the are common buttons
     * (like "Cancel" or "Yes"), and show the others as command links in the dialog.
     * This can make the dialog appear in the style of modern Windows apps. If you
     * don't like this behavior, you can set to .
     */
    noLink: boolean;
  }

  interface OnBeforeRedirectDetails {
    id: string;
    url: string;
    method: string;
    resourceType: string;
    timestamp: number;
    redirectURL: string;
    statusCode: number;
    /**
     * The server IP address that the request was actually sent to.
     */
    ip?: string;
    fromCache: boolean;
    responseHeaders: ResponseHeaders;
  }

  interface OnBeforeRequestDetails {
    id: number;
    url: string;
    method: string;
    resourceType: string;
    timestamp: number;
    uploadData: UploadData[];
  }

  interface OnCompletedDetails {
    id: number;
    url: string;
    method: string;
    resourceType: string;
    timestamp: number;
    responseHeaders: ResponseHeaders;
    fromCache: boolean;
    statusCode: number;
    statusLine: string;
  }

  interface OnErrorOccurredDetails {
    id: number;
    url: string;
    method: string;
    resourceType: string;
    timestamp: number;
    fromCache: boolean;
    /**
     * The error description.
     */
    error: string;
  }

  interface OnResponseStartedDetails {
    id: number;
    url: string;
    method: string;
    resourceType: string;
    timestamp: number;
    responseHeaders: ResponseHeaders;
    /**
     * Indicates whether the response was fetched from disk cache.
     */
    fromCache: boolean;
    statusCode: number;
    statusLine: string;
  }

  interface OnSendHeadersDetails {
    id: number;
    url: string;
    method: string;
    resourceType: string;
    timestamp: number;
    requestHeaders: RequestHeaders;
  }

  interface OpenDevToolsOptions {
    /**
     * Opens the devtools with specified dock state, can be , , , . Defaults to last
     * used dock state. In mode it's possible to dock back. In mode it's not.
     */
    mode: string;
  }

  interface OpenDialogOptions {
    title: string;
    defaultPath: string;
    /**
     * Custom label for the confirmation button, when left empty the default label will
     * be used.
     */
    buttonLabel: string;
    filters: string[];
    /**
     * Contains which features the dialog should use, can contain , , , and .
     */
    properties: string[];
  }

  interface OpenExternalOptions {
    /**
     * to bring the opened application to the foreground. The default is .
     */
    activate: boolean;
  }

  interface Parameters {
    /**
     * Specify the screen type to emulate (default: )
     */
    screenPosition: string;
    /**
     * Set the emulated screen size (screenPosition == mobile)
     */
    screenSize: ScreenSize;
    /**
     * Position the view on the screen (screenPosition == mobile) (default: )
     */
    viewPosition: ViewPosition;
    /**
     * Set the device scale factor (if zero defaults to original device scale factor)
     * (default: )
     */
    deviceScaleFactor: number;
    /**
     * Set the emulated view size (empty means no override)
     */
    viewSize: ViewSize;
    /**
     * Whether emulated view should be scaled down if necessary to fit into available
     * space (default: )
     */
    fitToView: boolean;
    /**
     * Offset of the emulated view inside available space (not in fit to view mode)
     * (default: )
     */
    offset: Offset;
    /**
     * Scale of emulated view inside available space (not in fit to view mode)
     * (default: )
     */
    scale: number;
  }

  interface Point {
    x: number;
    y: number;
  }

  interface Position {
    x: number;
    y: number;
  }

  interface PrintOptions {
    /**
     * Don't ask user for print settings. Default is .
     */
    silent: boolean;
    /**
     * Also prints the background color and image of the web page. Default is .
     */
    printBackground: boolean;
  }

  interface PrintToPDFOptions {
    /**
     * Specifies the type of margins to use. Uses 0 for default margin, 1 for no
     * margin, and 2 for minimum margin.
     */
    marginsType: number;
    /**
     * Specify page size of the generated PDF. Can be , , , , , or an Object containing
     * and in microns.
     */
    pageSize: string;
    /**
     * Whether to print CSS backgrounds.
     */
    printBackground: boolean;
    /**
     * Whether to print selection only.
     */
    printSelectionOnly: boolean;
    /**
     * for landscape, for portrait.
     */
    landscape: boolean;
  }

  interface ProcessMemoryInfo {
    /**
     * The amount of memory currently pinned to actual physical RAM.
     */
    workingSetSize: number;
    /**
     * The maximum amount of memory that has ever been pinned to actual physical RAM.
     */
    peakWorkingSetSize: number;
    /**
     * The amount of memory not shared by other processes, such as JS heap or HTML
     * content.
     */
    privateBytes: number;
    /**
     * The amount of memory shared between processes, typically memory consumed by the
     * Electron code itself
     */
    sharedBytes: number;
  }

  interface ProgressBarOptions {
    /**
     * - Mode for the progress bar (, , , , or )
     */
    mode: string;
  }

  interface ReadBookmark {
    title: string;
    url: string;
  }

  interface Rect {
    x: number;
    y: number;
    width: number;
    height: number;
  }

  interface RedirectRequest {
    url: string;
    method: string;
    session?: Session;
    uploadData?: UploadData;
  }

  interface RegisterBufferProtocolRequest {
    url: string;
    referrer: string;
    method: string;
    uploadData: UploadData[];
  }

  interface RegisterFileProtocolRequest {
    url: string;
    referrer: string;
    method: string;
    uploadData: UploadData[];
  }

  interface RegisterHttpProtocolRequest {
    url: string;
    referrer: string;
    method: string;
    uploadData: UploadData[];
  }

  interface RegisterStringProtocolRequest {
    url: string;
    referrer: string;
    method: string;
    uploadData: UploadData[];
  }

  interface RegisterURLSchemeAsPrivilegedOptions {
    /**
     * Default true.
     */
    secure?: boolean;
    /**
     * Default true.
     */
    bypassCSP?: boolean;
    /**
     * Default true.
     */
    allowServiceWorkers?: boolean;
    /**
     * Default true.
     */
    supportFetchAPI?: boolean;
    /**
     * Default true.
     */
    corsEnabled?: boolean;
  }

  interface RelaunchOptions {
    args?: string[];
    execPath?: string;
  }

  interface Request {
    method: string;
    url: URL;
    referrer: URL;
  }

  interface Require {
    module: string;
  }

  interface ResizeOptions {
    width?: number;
    height?: number;
    /**
     * The desired quality of the resize image. Possible values are , or . The default
     * is . These values express a desired quality/speed tradeoff. They are translated
     * into an algorithm-specific method that depends on the capabilities (CPU, GPU) of
     * the underlying platform. It is possible for all three methods to be mapped to
     * the same algorithm on a given platform.
     */
    quality?: string;
  }

  interface ResourceUsage {
    images: MemoryUsageDetails;
    cssStyleSheets: MemoryUsageDetails;
    xslStyleSheets: MemoryUsageDetails;
    fonts: MemoryUsageDetails;
    other: MemoryUsageDetails;
  }

  interface Response {
    cancel?: boolean;
    /**
     * The original request is prevented from being sent or completed and is instead
     * redirected to the given URL.
     */
    redirectURL?: string;
  }

  interface Result {
    requestId: number;
    /**
     * Position of the active match.
     */
    activeMatchOrdinal: number;
    /**
     * Number of Matches.
     */
    matches: number;
    /**
     * Coordinates of first match region.
     */
    selectionArea: SelectionArea;
  }

  interface SaveDialogOptions {
    title: string;
    defaultPath: string;
    /**
     * Custom label for the confirmation button, when left empty the default label will
     * be used.
     */
    buttonLabel: string;
    filters: string[];
  }

  interface Settings {
    /**
     * to open the app at login, to remove the app as a login item. Defaults to .
     */
    openAtLogin: boolean;
    /**
     * to open the app as hidden. Defaults to . The user can edit this setting from the
     * System Preferences so should be checked when the app is opened to know the
     * current value. This setting is only supported on macOS.
     */
    openAsHidden: boolean;
  }

  interface Size {
    width: number;
    height: number;
  }

  interface SourcesOptions {
    /**
     * An array of Strings that lists the types of desktop sources to be captured,
     * available types are and .
     */
    types: string[];
    /**
     * The suggested size that the media source thumbnail should be scaled to, defaults
     * to .
     */
    thumbnailSize?: ThumbnailSize;
  }

  interface StartMonitoringOptions {
    categoryFilter: string;
    traceOptions: string;
  }

  interface StartRecordingOptions {
    categoryFilter: string;
    traceOptions: string;
  }

  interface SystemMemoryInfo {
    /**
     * The total amount of physical memory in Kilobytes available to the system.
     */
    total: number;
    /**
     * The total amount of memory not being used by applications or disk cache.
     */
    free: number;
    /**
     * The total amount of swap memory in Kilobytes available to the system.
     */
    swapTotal: number;
    /**
     * The free amount of swap memory in Kilobytes available to the system.
     */
    swapFree: number;
  }

  interface WorkAreaSize {
    height: number;
    width: number;
  }

  interface EditFlags {
    /**
     * Whether the renderer believes it can undo.
     */
    canUndo: boolean;
    /**
     * Whether the renderer believes it can redo.
     */
    canRedo: boolean;
    /**
     * Whether the renderer believes it can cut.
     */
    canCut: boolean;
    /**
     * Whether the renderer believes it can copy
     */
    canCopy: boolean;
    /**
     * Whether the renderer believes it can paste.
     */
    canPaste: boolean;
    /**
     * Whether the renderer believes it can delete.
     */
    canDelete: boolean;
    /**
     * Whether the renderer believes it can select all.
     */
    canSelectAll: boolean;
  }

  interface Extra {
  }

  interface MediaFlags {
    /**
     * Whether the media element has crashed.
     */
    inError: boolean;
    /**
     * Whether the media element is paused.
     */
    isPaused: boolean;
    /**
     * Whether the media element is muted.
     */
    isMuted: boolean;
    /**
     * Whether the media element has audio.
     */
    hasAudio: boolean;
    /**
     * Whether the media element is looping.
     */
    isLooping: boolean;
    /**
     * Whether the media element's controls are visible.
     */
    isControlsVisible: boolean;
    /**
     * Whether the media element's controls are toggleable.
     */
    canToggleControls: boolean;
    /**
     * Whether the media element can be rotated.
     */
    canRotate: boolean;
  }

  interface Offset {
    /**
     * Set the x axis offset from top left corner
     */
    x: number;
    /**
     * Set the y axis offset from top left corner
     */
    y: number;
  }

  interface RequestHeaders {
  }

  interface ResponseHeaders {
  }

  interface ScreenSize {
    /**
     * Set the emulated screen width
     */
    width: number;
    /**
     * Set the emulated screen height
     */
    height: number;
  }

  interface SelectionArea {
  }

  interface ThumbnailSize {
  }

  interface ViewPosition {
    /**
     * Set the x axis offset from top left corner
     */
    x: number;
    /**
     * Set the y axis offset from top left corner
     */
    y: number;
  }

  interface ViewSize {
    /**
     * Set the emulated view width
     */
    width: number;
    /**
     * Set the emulated view height
     */
    height: number;
  }

  interface WebPreferences {
    /**
     * Whether to enable DevTools. If it is set to , can not use to open DevTools.
     * Default is .
     */
    devTools: boolean;
    /**
     * Whether node integration is enabled. Default is .
     */
    nodeIntegration: boolean;
    /**
     * Specifies a script that will be loaded before other scripts run in the page.
     * This script will always have access to node APIs no matter whether node
     * integration is turned on or off. The value should be the absolute file path to
     * the script. When node integration is turned off, the preload script can
     * reintroduce Node global symbols back to the global scope. See example .
     */
    preload: string;
    /**
     * Sets the session used by the page. Instead of passing the Session object
     * directly, you can also choose to use the option instead, which accepts a
     * partition string. When both and are provided, will be preferred. Default is the
     * default session.
     */
    session: Session;
    /**
     * Sets the session used by the page according to the session's partition string.
     * If starts with , the page will use a persistent session available to all pages
     * in the app with the same . If there is no prefix, the page will use an in-memory
     * session. By assigning the same , multiple pages can share the same session.
     * Default is the default session.
     */
    partition: string;
    /**
     * The default zoom factor of the page, represents . Default is .
     */
    zoomFactor: number;
    /**
     * Enables JavaScript support. Default is .
     */
    javascript: boolean;
    /**
     * When , it will disable the same-origin policy (usually using testing websites by
     * people), and set and to if these two options are not set by user. Default is .
     */
    webSecurity: boolean;
    /**
     * Allow an https page to display content like images from http URLs. Default is .
     */
    allowDisplayingInsecureContent: boolean;
    /**
     * Allow an https page to run JavaScript, CSS or plugins from http URLs. Default is
     * .
     */
    allowRunningInsecureContent: boolean;
    /**
     * Enables image support. Default is .
     */
    images: boolean;
    /**
     * Make TextArea elements resizable. Default is .
     */
    textAreasAreResizable: boolean;
    /**
     * Enables WebGL support. Default is .
     */
    webgl: boolean;
    /**
     * Enables WebAudio support. Default is .
     */
    webaudio: boolean;
    /**
     * Whether plugins should be enabled. Default is .
     */
    plugins: boolean;
    /**
     * Enables Chromium's experimental features. Default is .
     */
    experimentalFeatures: boolean;
    /**
     * Enables Chromium's experimental canvas features. Default is .
     */
    experimentalCanvasFeatures: boolean;
    /**
     * Enables scroll bounce (rubber banding) effect on macOS. Default is .
     */
    scrollBounce: boolean;
    /**
     * A list of feature strings separated by , like to enable. The full list of
     * supported feature strings can be found in the file.
     */
    blinkFeatures: string;
    /**
     * A list of feature strings separated by , like to disable. The full list of
     * supported feature strings can be found in the file.
     */
    disableBlinkFeatures: string;
    /**
     * Sets the default font for the font-family.
     */
    defaultFontFamily: DefaultFontFamily;
    /**
     * Defaults to .
     */
    defaultFontSize: number;
    /**
     * Defaults to .
     */
    defaultMonospaceFontSize: number;
    /**
     * Defaults to .
     */
    minimumFontSize: number;
    /**
     * Defaults to .
     */
    defaultEncoding: string;
    /**
     * Whether to throttle animations and timers when the page becomes background.
     * Defaults to .
     */
    backgroundThrottling: boolean;
    /**
     * Whether to enable offscreen rendering for the browser window. Defaults to .
     */
    offscreen: boolean;
    /**
     * Whether to enable Chromium OS-level sandbox.
     */
    sandbox: boolean;
  }

  interface DefaultFontFamily {
    /**
     * Defaults to .
     */
    standard: string;
    /**
     * Defaults to .
     */
    serif: string;
    /**
     * Defaults to .
     */
    sansSerif: string;
    /**
     * Defaults to .
     */
    monospace: string;
  }

}

declare module 'electron' {
  const electron: Electron.AllElectron;
  export = electron;
}

interface NodeRequireFunction {
  (moduleName: 'electron'): Electron.AllElectron;
}

interface File {
  /**
  * The real path to the file on the users filesystem
  */
  path: string;
}

declare module 'original-fs' {
  import * as fs from 'fs';

  export = fs;
}
