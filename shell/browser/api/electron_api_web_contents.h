// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_ELECTRON_API_WEB_CONTENTS_H_
#define SHELL_BROWSER_API_ELECTRON_API_WEB_CONTENTS_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "content/common/cursors/webcursor.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/keyboard_event_processing_result.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/common/favicon_url.h"
#include "electron/buildflags/buildflags.h"
#include "electron/shell/common/api/api.mojom.h"
#include "gin/handle.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "printing/buildflags/buildflags.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "shell/browser/api/frame_subscriber.h"
#include "shell/browser/api/save_page_handler.h"
#include "shell/browser/common_web_contents_delegate.h"
#include "shell/common/gin_helper/trackable_object.h"
#include "ui/gfx/image/image.h"

#if BUILDFLAG(ENABLE_PRINTING)
#include "chrome/browser/printing/print_view_manager_basic.h"
#include "components/printing/common/print_messages.h"
#include "printing/backend/print_backend.h"
#include "shell/browser/printing/print_preview_message_handler.h"

#if defined(OS_WIN)
#include "printing/backend/win_helper.h"
#endif
#endif

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
#include "extensions/browser/script_executor.h"
#endif

namespace blink {
struct WebDeviceEmulationParams;
}

namespace gin_helper {
class Dictionary;
}

namespace network {
class ResourceRequestBody;
}

namespace electron {

class ElectronBrowserContext;
class ElectronJavaScriptDialogManager;
class InspectableWebContents;
class WebContentsZoomController;
class WebViewGuestDelegate;
class FrameSubscriber;

#if BUILDFLAG(ENABLE_OSR)
class OffScreenRenderWidgetHostView;
#endif

namespace api {

// Certain events are only in WebContentsDelegate, provide our own Observer to
// dispatch those events.
class ExtendedWebContentsObserver : public base::CheckedObserver {
 public:
  virtual void OnCloseContents() {}
  virtual void OnRendererResponsive() {}
  virtual void OnDraggableRegionsUpdated(
      const std::vector<mojom::DraggableRegionPtr>& regions) {}
  virtual void OnSetContentBounds(const gfx::Rect& rect) {}
  virtual void OnActivateContents() {}
  virtual void OnPageTitleUpdated(const base::string16& title,
                                  bool explicit_set) {}

 protected:
  ~ExtendedWebContentsObserver() override {}
};

// Wrapper around the content::WebContents.
class WebContents : public gin_helper::TrackableObject<WebContents>,
                    public CommonWebContentsDelegate,
                    public content::WebContentsObserver,
                    public mojom::ElectronBrowser {
 public:
  enum class Type {
    BACKGROUND_PAGE,  // A DevTools extension background page.
    BROWSER_WINDOW,   // Used by BrowserWindow.
    BROWSER_VIEW,     // Used by BrowserView.
    REMOTE,           // Thin wrap around an existing WebContents.
    WEB_VIEW,         // Used by <webview>.
    OFF_SCREEN,       // Used for offscreen rendering
  };

  // Create a new WebContents and return the V8 wrapper of it.
  static gin::Handle<WebContents> Create(v8::Isolate* isolate,
                                         const gin_helper::Dictionary& options);

  // Create a new V8 wrapper for an existing |web_content|.
  //
  // The lifetime of |web_contents| will be managed by this class.
  static gin::Handle<WebContents> CreateAndTake(
      v8::Isolate* isolate,
      std::unique_ptr<content::WebContents> web_contents,
      Type type);

  // Get the V8 wrapper of |web_content|, return empty handle if not wrapped.
  static gin::Handle<WebContents> From(v8::Isolate* isolate,
                                       content::WebContents* web_content);

  // Get the V8 wrapper of the |web_contents|, or create one if not existed.
  //
  // The lifetime of |web_contents| is NOT managed by this class, and the type
  // of this wrapper is always REMOTE.
  static gin::Handle<WebContents> FromOrCreate(
      v8::Isolate* isolate,
      content::WebContents* web_contents);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  base::WeakPtr<WebContents> GetWeakPtr() { return weak_factory_.GetWeakPtr(); }

  // Destroy the managed content::WebContents instance.
  //
  // Note: The |async| should only be |true| when users are expecting to use the
  // webContents immediately after the call. Always pass |false| if you are not
  // sure.
  // See https://github.com/electron/electron/issues/8930.
  //
  // Note: When destroying a webContents member inside a destructor, the |async|
  // should always be |false|, otherwise the destroy task might be delayed after
  // normal shutdown procedure, resulting in an assertion.
  // The normal pattern for calling this method in destructor is:
  // api_web_contents_->DestroyWebContents(!Browser::Get()->is_shutting_down())
  // See https://github.com/electron/electron/issues/15133.
  void DestroyWebContents(bool async);

  void SetBackgroundThrottling(bool allowed);
  int GetProcessID() const;
  base::ProcessId GetOSProcessID() const;
  base::ProcessId GetOSProcessIdForFrame(const std::string& name,
                                         const std::string& document_url) const;
  Type GetType() const;
  bool Equal(const WebContents* web_contents) const;
  void LoadURL(const GURL& url, const gin_helper::Dictionary& options);
  void DownloadURL(const GURL& url);
  GURL GetURL() const;
  base::string16 GetTitle() const;
  bool IsLoading() const;
  bool IsLoadingMainFrame() const;
  bool IsWaitingForResponse() const;
  void Stop();
  void ReloadIgnoringCache();
  void GoBack();
  void GoForward();
  void GoToOffset(int offset);
  const std::string GetWebRTCIPHandlingPolicy() const;
  void SetWebRTCIPHandlingPolicy(const std::string& webrtc_ip_handling_policy);
  bool IsCrashed() const;
  void SetUserAgent(const std::string& user_agent, gin_helper::Arguments* args);
  std::string GetUserAgent();
  void InsertCSS(const std::string& css);
  v8::Local<v8::Promise> SavePage(const base::FilePath& full_file_path,
                                  const content::SavePageType& save_type);
  void OpenDevTools(gin_helper::Arguments* args);
  void CloseDevTools();
  bool IsDevToolsOpened();
  bool IsDevToolsFocused();
  void ToggleDevTools();
  void EnableDeviceEmulation(const blink::WebDeviceEmulationParams& params);
  void DisableDeviceEmulation();
  void InspectElement(int x, int y);
  void InspectSharedWorker();
  void InspectSharedWorkerById(const std::string& workerId);
  std::vector<scoped_refptr<content::DevToolsAgentHost>> GetAllSharedWorkers();
  void InspectServiceWorker();
  void SetIgnoreMenuShortcuts(bool ignore);
  void SetAudioMuted(bool muted);
  bool IsAudioMuted();
  bool IsCurrentlyAudible();
  void SetEmbedder(const WebContents* embedder);
  void SetDevToolsWebContents(const WebContents* devtools);
  v8::Local<v8::Value> GetNativeView() const;
  void IncrementCapturerCount(gin_helper::Arguments* args);
  void DecrementCapturerCount(gin_helper::Arguments* args);
  bool IsBeingCaptured();

#if BUILDFLAG(ENABLE_PRINTING)
  void OnGetDefaultPrinter(base::Value print_settings,
                           printing::CompletionCallback print_callback,
                           base::string16 device_name,
                           bool silent,
                           base::string16 default_printer);
  v8::Local<v8::Promise> Print(base::Value settings);
  std::vector<printing::PrinterBasicInfo> GetPrinterList();
  // Print current page as PDF.
  v8::Local<v8::Promise> PrintToPDF(base::DictionaryValue settings);
#endif

  // DevTools workspace api.
  void AddWorkSpace(gin_helper::Arguments* args, const base::FilePath& path);
  void RemoveWorkSpace(gin_helper::Arguments* args, const base::FilePath& path);

  // Editing commands.
  void Undo();
  void Redo();
  void Cut();
  void Copy();
  void Paste();
  void PasteAndMatchStyle();
  void Delete();
  void SelectAll();
  void Unselect();
  void Replace(const base::string16& word);
  void ReplaceMisspelling(const base::string16& word);
  uint32_t FindInPage(gin_helper::Arguments* args);
  void StopFindInPage(content::StopFindAction action);
  void ShowDefinitionForSelection();
  void CopyImageAt(int x, int y);

  // Focus.
  void Focus();
  bool IsFocused() const;
  void TabTraverse(bool reverse);

  // Send messages to browser.
  bool SendIPCMessage(bool internal,
                      bool send_to_all,
                      const std::string& channel,
                      v8::Local<v8::Value> args);

  bool SendIPCMessageWithSender(bool internal,
                                bool send_to_all,
                                const std::string& channel,
                                blink::CloneableMessage args,
                                int32_t sender_id = 0);

  bool SendIPCMessageToFrame(bool internal,
                             bool send_to_all,
                             int32_t frame_id,
                             const std::string& channel,
                             v8::Local<v8::Value> args);

  // Send WebInputEvent to the page.
  void SendInputEvent(v8::Isolate* isolate, v8::Local<v8::Value> input_event);

  // Subscribe to the frame updates.
  void BeginFrameSubscription(gin_helper::Arguments* args);
  void EndFrameSubscription();

  // Dragging native items.
  void StartDrag(const gin_helper::Dictionary& item,
                 gin_helper::Arguments* args);

  // Captures the page with |rect|, |callback| would be called when capturing is
  // done.
  v8::Local<v8::Promise> CapturePage(gin_helper::Arguments* args);

  // Methods for creating <webview>.
  bool IsGuest() const;
  void AttachToIframe(content::WebContents* embedder_web_contents,
                      int embedder_frame_id);
  void DetachFromOuterFrame();

  // Methods for offscreen rendering
  bool IsOffScreen() const;
#if BUILDFLAG(ENABLE_OSR)
  void OnPaint(const gfx::Rect& dirty_rect, const SkBitmap& bitmap);
  void StartPainting();
  void StopPainting();
  bool IsPainting() const;
  void SetFrameRate(int frame_rate);
  int GetFrameRate() const;
#endif
  void Invalidate();
  gfx::Size GetSizeForNewRenderView(content::WebContents*) override;

  // Methods for zoom handling.
  void SetZoomLevel(double level);
  double GetZoomLevel() const;
  void SetZoomFactor(double factor);
  double GetZoomFactor() const;

  // Callback triggered on permission response.
  void OnEnterFullscreenModeForTab(
      content::WebContents* source,
      const GURL& origin,
      const blink::mojom::FullscreenOptions& options,
      bool allowed);

  // Create window with the given disposition.
  void OnCreateWindow(const GURL& target_url,
                      const content::Referrer& referrer,
                      const std::string& frame_name,
                      WindowOpenDisposition disposition,
                      const std::vector<std::string>& features,
                      const scoped_refptr<network::ResourceRequestBody>& body);

  // Returns the preload script path of current WebContents.
  std::vector<base::FilePath::StringType> GetPreloadPaths() const;

  // Returns the web preferences of current WebContents.
  v8::Local<v8::Value> GetWebPreferences(v8::Isolate* isolate) const;
  v8::Local<v8::Value> GetLastWebPreferences(v8::Isolate* isolate) const;

  // Returns the owner window.
  v8::Local<v8::Value> GetOwnerBrowserWindow() const;

  // Grants the child process the capability to access URLs with the origin of
  // the specified URL.
  void GrantOriginAccess(const GURL& url);

  v8::Local<v8::Promise> TakeHeapSnapshot(const base::FilePath& file_path);

  // Properties.
  int32_t ID() const;
  v8::Local<v8::Value> Session(v8::Isolate* isolate);
  content::WebContents* HostWebContents() const;
  v8::Local<v8::Value> DevToolsWebContents(v8::Isolate* isolate);
  v8::Local<v8::Value> Debugger(v8::Isolate* isolate);

  WebContentsZoomController* GetZoomController() { return zoom_controller_; }

  void AddObserver(ExtendedWebContentsObserver* obs) {
    observers_.AddObserver(obs);
  }
  void RemoveObserver(ExtendedWebContentsObserver* obs) {
    // Trying to remove from an empty collection leads to an access violation
    if (observers_.might_have_observers())
      observers_.RemoveObserver(obs);
  }

  bool EmitNavigationEvent(const std::string& event,
                           content::NavigationHandle* navigation_handle);

  WebContents* embedder() { return embedder_; }

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  extensions::ScriptExecutor* script_executor() {
    return script_executor_.get();
  }
#endif

 protected:
  // Does not manage lifetime of |web_contents|.
  WebContents(v8::Isolate* isolate, content::WebContents* web_contents);
  // Takes over ownership of |web_contents|.
  WebContents(v8::Isolate* isolate,
              std::unique_ptr<content::WebContents> web_contents,
              Type type);
  // Creates a new content::WebContents.
  WebContents(v8::Isolate* isolate, const gin_helper::Dictionary& options);
  ~WebContents() override;

  void InitWithSessionAndOptions(
      v8::Isolate* isolate,
      std::unique_ptr<content::WebContents> web_contents,
      gin::Handle<class Session> session,
      const gin_helper::Dictionary& options);

  // content::WebContentsDelegate:
  bool DidAddMessageToConsole(content::WebContents* source,
                              blink::mojom::ConsoleMessageLevel level,
                              const base::string16& message,
                              int32_t line_no,
                              const base::string16& source_id) override;
  void WebContentsCreated(content::WebContents* source_contents,
                          int opener_render_process_id,
                          int opener_render_frame_id,
                          const std::string& frame_name,
                          const GURL& target_url,
                          content::WebContents* new_contents) override;
  void AddNewContents(content::WebContents* source,
                      std::unique_ptr<content::WebContents> new_contents,
                      WindowOpenDisposition disposition,
                      const gfx::Rect& initial_rect,
                      bool user_gesture,
                      bool* was_blocked) override;
  content::WebContents* OpenURLFromTab(
      content::WebContents* source,
      const content::OpenURLParams& params) override;
  void BeforeUnloadFired(content::WebContents* tab,
                         bool proceed,
                         bool* proceed_to_fire_unload) override;
  void SetContentsBounds(content::WebContents* source,
                         const gfx::Rect& pos) override;
  void CloseContents(content::WebContents* source) override;
  void ActivateContents(content::WebContents* contents) override;
  void UpdateTargetURL(content::WebContents* source, const GURL& url) override;
  bool HandleKeyboardEvent(
      content::WebContents* source,
      const content::NativeWebKeyboardEvent& event) override;
  content::KeyboardEventProcessingResult PreHandleKeyboardEvent(
      content::WebContents* source,
      const content::NativeWebKeyboardEvent& event) override;
  void ContentsZoomChange(bool zoom_in) override;
  void EnterFullscreenModeForTab(
      content::WebContents* source,
      const GURL& origin,
      const blink::mojom::FullscreenOptions& options) override;
  void ExitFullscreenModeForTab(content::WebContents* source) override;
  void RendererUnresponsive(
      content::WebContents* source,
      content::RenderWidgetHost* render_widget_host,
      base::RepeatingClosure hang_monitor_restarter) override;
  void RendererResponsive(
      content::WebContents* source,
      content::RenderWidgetHost* render_widget_host) override;
  bool HandleContextMenu(content::RenderFrameHost* render_frame_host,
                         const content::ContextMenuParams& params) override;
  bool OnGoToEntryOffset(int offset) override;
  void FindReply(content::WebContents* web_contents,
                 int request_id,
                 int number_of_matches,
                 const gfx::Rect& selection_rect,
                 int active_match_ordinal,
                 bool final_update) override;
  bool CheckMediaAccessPermission(content::RenderFrameHost* render_frame_host,
                                  const GURL& security_origin,
                                  blink::mojom::MediaStreamType type) override;
  void RequestMediaAccessPermission(
      content::WebContents* web_contents,
      const content::MediaStreamRequest& request,
      content::MediaResponseCallback callback) override;
  void RequestToLockMouse(content::WebContents* web_contents,
                          bool user_gesture,
                          bool last_unlocked_by_target) override;
  std::unique_ptr<content::BluetoothChooser> RunBluetoothChooser(
      content::RenderFrameHost* frame,
      const content::BluetoothChooser::EventHandler& handler) override;
  content::JavaScriptDialogManager* GetJavaScriptDialogManager(
      content::WebContents* source) override;
  void OnAudioStateChanged(bool audible) override;

  // content::WebContentsObserver:
  void BeforeUnloadFired(bool proceed,
                         const base::TimeTicks& proceed_time) override;
  void RenderViewCreated(content::RenderViewHost* render_view_host) override;
  void RenderFrameCreated(content::RenderFrameHost* render_frame_host) override;
  void RenderViewHostChanged(content::RenderViewHost* old_host,
                             content::RenderViewHost* new_host) override;
  void RenderViewDeleted(content::RenderViewHost*) override;
  void RenderProcessGone(base::TerminationStatus status) override;
  void RenderFrameDeleted(content::RenderFrameHost* render_frame_host) override;
  void DOMContentLoaded(content::RenderFrameHost* render_frame_host) override;
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;
  void DidFailLoad(content::RenderFrameHost* render_frame_host,
                   const GURL& validated_url,
                   int error_code) override;
  void DidStartLoading() override;
  void DidStopLoading() override;
  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidRedirectNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  bool OnMessageReceived(const IPC::Message& message) override;
  void WebContentsDestroyed() override;
  void NavigationEntryCommitted(
      const content::LoadCommittedDetails& load_details) override;
  void TitleWasSet(content::NavigationEntry* entry) override;
  void DidUpdateFaviconURL(
      const std::vector<content::FaviconURL>& urls) override;
  void PluginCrashed(const base::FilePath& plugin_path,
                     base::ProcessId plugin_pid) override;
  void MediaStartedPlaying(const MediaPlayerInfo& video_type,
                           const content::MediaPlayerId& id) override;
  void MediaStoppedPlaying(
      const MediaPlayerInfo& video_type,
      const content::MediaPlayerId& id,
      content::WebContentsObserver::MediaStoppedReason reason) override;
  void DidChangeThemeColor() override;
  void OnInterfaceRequestFromFrame(
      content::RenderFrameHost* render_frame_host,
      const std::string& interface_name,
      mojo::ScopedMessagePipeHandle* interface_pipe) override;
  void DidAcquireFullscreen(content::RenderFrameHost* rfh) override;

  // InspectableWebContentsDelegate:
  void DevToolsReloadPage() override;

  // InspectableWebContentsViewDelegate:
  void DevToolsFocused() override;
  void DevToolsOpened() override;
  void DevToolsClosed() override;

 private:
  ElectronBrowserContext* GetBrowserContext() const;

  // Binds the given request for the ElectronBrowser API. When the
  // RenderFrameHost is destroyed, all related bindings will be removed.
  void BindElectronBrowser(mojom::ElectronBrowserRequest request,
                           content::RenderFrameHost* render_frame_host);
  void OnElectronBrowserConnectionError();

  uint32_t GetNextRequestId() { return ++request_id_; }

#if BUILDFLAG(ENABLE_OSR)
  OffScreenWebContentsView* GetOffScreenWebContentsView() const override;
  OffScreenRenderWidgetHostView* GetOffScreenRenderWidgetHostView() const;
#endif

  // mojom::ElectronBrowser
  void Message(bool internal,
               const std::string& channel,
               blink::CloneableMessage arguments) override;
  void Invoke(bool internal,
              const std::string& channel,
              blink::CloneableMessage arguments,
              InvokeCallback callback) override;
  void MessageSync(bool internal,
                   const std::string& channel,
                   blink::CloneableMessage arguments,
                   MessageSyncCallback callback) override;
  void MessageTo(bool internal,
                 bool send_to_all,
                 int32_t web_contents_id,
                 const std::string& channel,
                 blink::CloneableMessage arguments) override;
  void MessageHost(const std::string& channel,
                   blink::CloneableMessage arguments) override;
#if BUILDFLAG(ENABLE_REMOTE_MODULE)
  void DereferenceRemoteJSObject(const std::string& context_id,
                                 int object_id,
                                 int ref_count) override;
#endif
  void UpdateDraggableRegions(
      std::vector<mojom::DraggableRegionPtr> regions) override;
  void SetTemporaryZoomLevel(double level) override;
  void DoGetZoomLevel(DoGetZoomLevelCallback callback) override;

  // Called when we receive a CursorChange message from chromium.
  void OnCursorChange(const content::WebCursor& cursor);

  // Called when received a synchronous message from renderer to
  // get the zoom level.
  void OnGetZoomLevel(content::RenderFrameHost* frame_host,
                      IPC::Message* reply_msg);

  void InitZoomController(content::WebContents* web_contents,
                          const gin_helper::Dictionary& options);

  v8::Global<v8::Value> session_;
  v8::Global<v8::Value> devtools_web_contents_;
  v8::Global<v8::Value> debugger_;

  std::unique_ptr<ElectronJavaScriptDialogManager> dialog_manager_;
  std::unique_ptr<WebViewGuestDelegate> guest_delegate_;
  std::unique_ptr<FrameSubscriber> frame_subscriber_;

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  std::unique_ptr<extensions::ScriptExecutor> script_executor_;
#endif

  // The host webcontents that may contain this webcontents.
  WebContents* embedder_ = nullptr;

  // The zoom controller for this webContents.
  WebContentsZoomController* zoom_controller_ = nullptr;

  // The type of current WebContents.
  Type type_ = Type::BROWSER_WINDOW;

  // Request id used for findInPage request.
  uint32_t request_id_ = 0;

  // Whether background throttling is disabled.
  bool background_throttling_ = true;

  // Whether to enable devtools.
  bool enable_devtools_ = true;

  // Observers of this WebContents.
  base::ObserverList<ExtendedWebContentsObserver> observers_;

  // The ID of the process of the currently committed RenderViewHost.
  // -1 means no speculative RVH has been committed yet.
  int currently_committed_process_id_ = -1;

  service_manager::BinderRegistryWithArgs<content::RenderFrameHost*> registry_;
  mojo::BindingSet<mojom::ElectronBrowser, content::RenderFrameHost*> bindings_;
  std::map<content::RenderFrameHost*, std::vector<mojo::BindingId>>
      frame_to_bindings_map_;

  base::WeakPtrFactory<WebContents> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(WebContents);
};

}  // namespace api

}  // namespace electron

#endif  // SHELL_BROWSER_API_ELECTRON_API_WEB_CONTENTS_H_
