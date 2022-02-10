// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_WEB_CONTENTS_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_WEB_CONTENTS_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "chrome/browser/devtools/devtools_eye_dropper.h"
#include "chrome/browser/devtools/devtools_file_system_indexer.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_context.h"  // nogncheck
#include "content/common/cursors/webcursor.h"
#include "content/common/frame.mojom.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/keyboard_event_processing_result.h"
#include "content/public/browser/permission_type.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_contents_observer.h"
#include "electron/buildflags/buildflags.h"
#include "electron/shell/common/api/api.mojom.h"
#include "gin/handle.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "printing/buildflags/buildflags.h"
#include "shell/browser/api/frame_subscriber.h"
#include "shell/browser/api/save_page_handler.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/browser/extended_web_contents_observer.h"
#include "shell/browser/ui/inspectable_web_contents.h"
#include "shell/browser/ui/inspectable_web_contents_delegate.h"
#include "shell/browser/ui/inspectable_web_contents_view_delegate.h"
#include "shell/common/gin_helper/cleaned_up_at_exit.h"
#include "shell/common/gin_helper/constructible.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/pinnable.h"
#include "ui/base/models/image_model.h"
#include "ui/gfx/image/image.h"

#if BUILDFLAG(ENABLE_PRINTING)
#include "shell/browser/printing/print_preview_message_handler.h"
#include "shell/browser/printing/print_view_manager_electron.h"
#endif

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
#include "extensions/common/mojom/view_type.mojom.h"

namespace extensions {
class ScriptExecutor;
}
#endif

namespace blink {
struct DeviceEmulationParams;
}

namespace gin_helper {
class Dictionary;
}

namespace network {
class ResourceRequestBody;
}

namespace gin {
class Arguments;
}

class ExclusiveAccessManager;

namespace electron {

class ElectronBrowserContext;
class ElectronJavaScriptDialogManager;
class InspectableWebContents;
class WebContentsZoomController;
class WebViewGuestDelegate;
class FrameSubscriber;
class WebDialogHelper;
class NativeWindow;

#if BUILDFLAG(ENABLE_OSR)
class OffScreenRenderWidgetHostView;
class OffScreenWebContentsView;
#endif

namespace api {

using DevicePermissionMap = std::map<
    int,
    std::map<content::PermissionType,
             std::map<url::Origin, std::vector<std::unique_ptr<base::Value>>>>>;

// Wrapper around the content::WebContents.
class WebContents : public ExclusiveAccessContext,
                    public gin::Wrappable<WebContents>,
                    public gin_helper::EventEmitterMixin<WebContents>,
                    public gin_helper::Constructible<WebContents>,
                    public gin_helper::Pinnable<WebContents>,
                    public gin_helper::CleanedUpAtExit,
                    public content::WebContentsObserver,
                    public content::WebContentsDelegate,
                    public InspectableWebContentsDelegate,
                    public InspectableWebContentsViewDelegate {
 public:
  enum class Type {
    kBackgroundPage,  // An extension background page.
    kBrowserWindow,   // Used by BrowserWindow.
    kBrowserView,     // Used by BrowserView.
    kRemote,          // Thin wrap around an existing WebContents.
    kWebView,         // Used by <webview>.
    kOffScreen,       // Used for offscreen rendering
  };

  // Create a new WebContents and return the V8 wrapper of it.
  static gin::Handle<WebContents> New(v8::Isolate* isolate,
                                      const gin_helper::Dictionary& options);

  // Create a new V8 wrapper for an existing |web_content|.
  //
  // The lifetime of |web_contents| will be managed by this class.
  static gin::Handle<WebContents> CreateAndTake(
      v8::Isolate* isolate,
      std::unique_ptr<content::WebContents> web_contents,
      Type type);

  // Get the api::WebContents associated with |web_contents|. Returns nullptr
  // if there is no associated wrapper.
  static WebContents* From(content::WebContents* web_contents);
  static WebContents* FromID(int32_t id);

  // Get the V8 wrapper of the |web_contents|, or create one if not existed.
  //
  // The lifetime of |web_contents| is NOT managed by this class, and the type
  // of this wrapper is always REMOTE.
  static gin::Handle<WebContents> FromOrCreate(
      v8::Isolate* isolate,
      content::WebContents* web_contents);

  static gin::Handle<WebContents> CreateFromWebPreferences(
      v8::Isolate* isolate,
      const gin_helper::Dictionary& web_preferences);

  // gin::Wrappable
  static gin::WrapperInfo kWrapperInfo;
  static v8::Local<v8::ObjectTemplate> FillObjectTemplate(
      v8::Isolate*,
      v8::Local<v8::ObjectTemplate>);
  const char* GetTypeName() override;

  void Destroy();
  base::WeakPtr<WebContents> GetWeakPtr() { return weak_factory_.GetWeakPtr(); }

  bool GetBackgroundThrottling() const;
  void SetBackgroundThrottling(bool allowed);
  int GetProcessID() const;
  base::ProcessId GetOSProcessID() const;
  Type GetType() const;
  bool Equal(const WebContents* web_contents) const;
  void LoadURL(const GURL& url, const gin_helper::Dictionary& options);
  void Reload();
  void ReloadIgnoringCache();
  void DownloadURL(const GURL& url);
  GURL GetURL() const;
  std::u16string GetTitle() const;
  bool IsLoading() const;
  bool IsLoadingMainFrame() const;
  bool IsWaitingForResponse() const;
  void Stop();
  bool CanGoBack() const;
  void GoBack();
  bool CanGoForward() const;
  void GoForward();
  bool CanGoToOffset(int offset) const;
  void GoToOffset(int offset);
  bool CanGoToIndex(int index) const;
  void GoToIndex(int index);
  int GetActiveIndex() const;
  void ClearHistory();
  int GetHistoryLength() const;
  const std::string GetWebRTCIPHandlingPolicy() const;
  void SetWebRTCIPHandlingPolicy(const std::string& webrtc_ip_handling_policy);
  std::string GetMediaSourceID(content::WebContents* request_web_contents);
  bool IsCrashed() const;
  void ForcefullyCrashRenderer();
  void SetUserAgent(const std::string& user_agent);
  std::string GetUserAgent();
  void InsertCSS(const std::string& css);
  v8::Local<v8::Promise> SavePage(const base::FilePath& full_file_path,
                                  const content::SavePageType& save_type);
  void OpenDevTools(gin::Arguments* args);
  void CloseDevTools();
  bool IsDevToolsOpened();
  bool IsDevToolsFocused();
  void ToggleDevTools();
  void EnableDeviceEmulation(const blink::DeviceEmulationParams& params);
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
  v8::Local<v8::Value> GetNativeView(v8::Isolate* isolate) const;
  void IncrementCapturerCount(gin::Arguments* args);
  void DecrementCapturerCount(gin::Arguments* args);
  bool IsBeingCaptured();
  void HandleNewRenderFrame(content::RenderFrameHost* render_frame_host);

#if BUILDFLAG(ENABLE_PRINTING)
  void OnGetDefaultPrinter(base::Value print_settings,
                           printing::CompletionCallback print_callback,
                           std::u16string device_name,
                           bool silent,
                           // <error, default_printer_name>
                           std::pair<std::string, std::u16string> info);
  void Print(gin::Arguments* args);
  // Print current page as PDF.
  v8::Local<v8::Promise> PrintToPDF(base::DictionaryValue settings);
#endif

  void SetNextChildWebPreferences(const gin_helper::Dictionary);

  // DevTools workspace api.
  void AddWorkSpace(gin::Arguments* args, const base::FilePath& path);
  void RemoveWorkSpace(gin::Arguments* args, const base::FilePath& path);

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
  void Replace(const std::u16string& word);
  void ReplaceMisspelling(const std::u16string& word);
  uint32_t FindInPage(gin::Arguments* args);
  void StopFindInPage(content::StopFindAction action);
  void ShowDefinitionForSelection();
  void CopyImageAt(int x, int y);

  // Focus.
  void Focus();
  bool IsFocused() const;

  // Send WebInputEvent to the page.
  void SendInputEvent(v8::Isolate* isolate, v8::Local<v8::Value> input_event);

  // Subscribe to the frame updates.
  void BeginFrameSubscription(gin::Arguments* args);
  void EndFrameSubscription();

  // Dragging native items.
  void StartDrag(const gin_helper::Dictionary& item, gin::Arguments* args);

  // Captures the page with |rect|, |callback| would be called when capturing is
  // done.
  v8::Local<v8::Promise> CapturePage(gin::Arguments* args);

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
  void SetZoomFactor(gin_helper::ErrorThrower thrower, double factor);
  double GetZoomFactor() const;

  // Callback triggered on permission response.
  void OnEnterFullscreenModeForTab(
      content::RenderFrameHost* requesting_frame,
      const blink::mojom::FullscreenOptions& options,
      bool allowed);

  // Create window with the given disposition.
  void OnCreateWindow(const GURL& target_url,
                      const content::Referrer& referrer,
                      const std::string& frame_name,
                      WindowOpenDisposition disposition,
                      const std::string& features,
                      const scoped_refptr<network::ResourceRequestBody>& body);

  // Returns the preload script path of current WebContents.
  std::vector<base::FilePath> GetPreloadPaths() const;

  // Returns the web preferences of current WebContents.
  v8::Local<v8::Value> GetLastWebPreferences(v8::Isolate* isolate) const;

  // Returns the owner window.
  v8::Local<v8::Value> GetOwnerBrowserWindow(v8::Isolate* isolate) const;

  // Notifies the web page that there is user interaction.
  void NotifyUserActivation();

  v8::Local<v8::Promise> TakeHeapSnapshot(v8::Isolate* isolate,
                                          const base::FilePath& file_path);
  v8::Local<v8::Promise> GetProcessMemoryInfo(v8::Isolate* isolate);

  // Properties.
  int32_t ID() const { return id_; }
  v8::Local<v8::Value> Session(v8::Isolate* isolate);
  content::WebContents* HostWebContents() const;
  v8::Local<v8::Value> DevToolsWebContents(v8::Isolate* isolate);
  v8::Local<v8::Value> Debugger(v8::Isolate* isolate);
  content::RenderFrameHost* MainFrame();

  WebContentsZoomController* GetZoomController() { return zoom_controller_; }

  void AddObserver(ExtendedWebContentsObserver* obs) {
    observers_.AddObserver(obs);
  }
  void RemoveObserver(ExtendedWebContentsObserver* obs) {
    // Trying to remove from an empty collection leads to an access violation
    if (!observers_.empty())
      observers_.RemoveObserver(obs);
  }

  bool EmitNavigationEvent(const std::string& event,
                           content::NavigationHandle* navigation_handle);

  // this.emit(name, new Event(sender, message), args...);
  template <typename... Args>
  bool EmitWithSender(base::StringPiece name,
                      content::RenderFrameHost* sender,
                      electron::mojom::ElectronBrowser::InvokeCallback callback,
                      Args&&... args) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Object> wrapper;
    if (!GetWrapper(isolate).ToLocal(&wrapper))
      return false;
    v8::Local<v8::Object> event = gin_helper::internal::CreateNativeEvent(
        isolate, wrapper, sender, std::move(callback));
    return EmitCustomEvent(name, event, std::forward<Args>(args)...);
  }

  WebContents* embedder() { return embedder_; }

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  extensions::ScriptExecutor* script_executor() {
    return script_executor_.get();
  }
#endif

  // Set the window as owner window.
  void SetOwnerWindow(NativeWindow* owner_window);
  void SetOwnerWindow(content::WebContents* web_contents,
                      NativeWindow* owner_window);

  // Returns the WebContents managed by this delegate.
  content::WebContents* GetWebContents() const;

  // Returns the WebContents of devtools.
  content::WebContents* GetDevToolsWebContents() const;

  InspectableWebContents* inspectable_web_contents() const {
    return inspectable_web_contents_.get();
  }

  NativeWindow* owner_window() const { return owner_window_.get(); }

  bool is_html_fullscreen() const { return html_fullscreen_; }

  void set_fullscreen_frame(content::RenderFrameHost* rfh) {
    fullscreen_frame_ = rfh;
  }

  // mojom::ElectronBrowser
  void Message(bool internal,
               const std::string& channel,
               blink::CloneableMessage arguments,
               content::RenderFrameHost* render_frame_host);
  void Invoke(bool internal,
              const std::string& channel,
              blink::CloneableMessage arguments,
              electron::mojom::ElectronBrowser::InvokeCallback callback,
              content::RenderFrameHost* render_frame_host);
  void OnFirstNonEmptyLayout(content::RenderFrameHost* render_frame_host);
  void ReceivePostMessage(const std::string& channel,
                          blink::TransferableMessage message,
                          content::RenderFrameHost* render_frame_host);
  void MessageSync(
      bool internal,
      const std::string& channel,
      blink::CloneableMessage arguments,
      electron::mojom::ElectronBrowser::MessageSyncCallback callback,
      content::RenderFrameHost* render_frame_host);
  void MessageTo(int32_t web_contents_id,
                 const std::string& channel,
                 blink::CloneableMessage arguments);
  void MessageHost(const std::string& channel,
                   blink::CloneableMessage arguments,
                   content::RenderFrameHost* render_frame_host);
  void UpdateDraggableRegions(std::vector<mojom::DraggableRegionPtr> regions);
  void SetTemporaryZoomLevel(double level);
  void DoGetZoomLevel(
      electron::mojom::ElectronBrowser::DoGetZoomLevelCallback callback);
  void SetImageAnimationPolicy(const std::string& new_policy);

  // Grants |origin| access to |device|.
  // To be used in place of ObjectPermissionContextBase::GrantObjectPermission.
  void GrantDevicePermission(const url::Origin& origin,
                             const base::Value* device,
                             content::PermissionType permissionType,
                             content::RenderFrameHost* render_frame_host);

  // Returns the list of devices that |origin| has been granted permission to
  // access. To be used in place of
  // ObjectPermissionContextBase::GetGrantedObjects.
  std::vector<base::Value> GetGrantedDevices(
      const url::Origin& origin,
      content::PermissionType permissionType,
      content::RenderFrameHost* render_frame_host);

  // disable copy
  WebContents(const WebContents&) = delete;
  WebContents& operator=(const WebContents&) = delete;

 private:
  // Does not manage lifetime of |web_contents|.
  WebContents(v8::Isolate* isolate, content::WebContents* web_contents);
  // Takes over ownership of |web_contents|.
  WebContents(v8::Isolate* isolate,
              std::unique_ptr<content::WebContents> web_contents,
              Type type);
  // Creates a new content::WebContents.
  WebContents(v8::Isolate* isolate, const gin_helper::Dictionary& options);
  ~WebContents() override;

  // Delete this if garbage collection has not started.
  void DeleteThisIfAlive();

  // Creates a InspectableWebContents object and takes ownership of
  // |web_contents|.
  void InitWithWebContents(std::unique_ptr<content::WebContents> web_contents,
                           ElectronBrowserContext* browser_context,
                           bool is_guest);

  void InitWithSessionAndOptions(
      v8::Isolate* isolate,
      std::unique_ptr<content::WebContents> web_contents,
      gin::Handle<class Session> session,
      const gin_helper::Dictionary& options);

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  void InitWithExtensionView(v8::Isolate* isolate,
                             content::WebContents* web_contents,
                             extensions::mojom::ViewType view_type);
#endif

  // content::WebContentsDelegate:
  bool DidAddMessageToConsole(content::WebContents* source,
                              blink::mojom::ConsoleMessageLevel level,
                              const std::u16string& message,
                              int32_t line_no,
                              const std::u16string& source_id) override;
  bool IsWebContentsCreationOverridden(
      content::SiteInstance* source_site_instance,
      content::mojom::WindowContainerType window_container_type,
      const GURL& opener_url,
      const content::mojom::CreateNewWindowParams& params) override;
  content::WebContents* CreateCustomWebContents(
      content::RenderFrameHost* opener,
      content::SiteInstance* source_site_instance,
      bool is_new_browsing_instance,
      const GURL& opener_url,
      const std::string& frame_name,
      const GURL& target_url,
      const content::StoragePartitionId& partition_id,
      content::SessionStorageNamespace* session_storage_namespace) override;
  void WebContentsCreatedWithFullParams(
      content::WebContents* source_contents,
      int opener_render_process_id,
      int opener_render_frame_id,
      const content::mojom::CreateNewWindowParams& params,
      content::WebContents* new_contents) override;
  void AddNewContents(content::WebContents* source,
                      std::unique_ptr<content::WebContents> new_contents,
                      const GURL& target_url,
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
  bool PlatformHandleKeyboardEvent(
      content::WebContents* source,
      const content::NativeWebKeyboardEvent& event);
  content::KeyboardEventProcessingResult PreHandleKeyboardEvent(
      content::WebContents* source,
      const content::NativeWebKeyboardEvent& event) override;
  void ContentsZoomChange(bool zoom_in) override;
  void EnterFullscreenModeForTab(
      content::RenderFrameHost* requesting_frame,
      const blink::mojom::FullscreenOptions& options) override;
  void ExitFullscreenModeForTab(content::WebContents* source) override;
  void RendererUnresponsive(
      content::WebContents* source,
      content::RenderWidgetHost* render_widget_host,
      base::RepeatingClosure hang_monitor_restarter) override;
  void RendererResponsive(
      content::WebContents* source,
      content::RenderWidgetHost* render_widget_host) override;
  bool HandleContextMenu(content::RenderFrameHost& render_frame_host,
                         const content::ContextMenuParams& params) override;
  bool OnGoToEntryOffset(int offset) override;
  void FindReply(content::WebContents* web_contents,
                 int request_id,
                 int number_of_matches,
                 const gfx::Rect& selection_rect,
                 int active_match_ordinal,
                 bool final_update) override;
  void RequestKeyboardLock(content::WebContents* web_contents,
                           bool esc_key_locked) override;
  void CancelKeyboardLockRequest(content::WebContents* web_contents) override;
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
  content::JavaScriptDialogManager* GetJavaScriptDialogManager(
      content::WebContents* source) override;
  void OnAudioStateChanged(bool audible) override;
  void UpdatePreferredSize(content::WebContents* web_contents,
                           const gfx::Size& pref_size) override;

  // content::WebContentsObserver:
  void BeforeUnloadFired(bool proceed,
                         const base::TimeTicks& proceed_time) override;
  void OnBackgroundColorChanged() override;
  void RenderFrameCreated(content::RenderFrameHost* render_frame_host) override;
  void RenderFrameDeleted(content::RenderFrameHost* render_frame_host) override;
  void RenderFrameHostChanged(content::RenderFrameHost* old_host,
                              content::RenderFrameHost* new_host) override;
  void FrameDeleted(int frame_tree_node_id) override;
  void RenderViewDeleted(content::RenderViewHost*) override;
  void PrimaryMainFrameRenderProcessGone(
      base::TerminationStatus status) override;
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
  void ReadyToCommitNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void WebContentsDestroyed() override;
  void NavigationEntryCommitted(
      const content::LoadCommittedDetails& load_details) override;
  void TitleWasSet(content::NavigationEntry* entry) override;
  void DidUpdateFaviconURL(
      content::RenderFrameHost* render_frame_host,
      const std::vector<blink::mojom::FaviconURLPtr>& urls) override;
  void PluginCrashed(const base::FilePath& plugin_path,
                     base::ProcessId plugin_pid) override;
  void MediaStartedPlaying(const MediaPlayerInfo& video_type,
                           const content::MediaPlayerId& id) override;
  void MediaStoppedPlaying(
      const MediaPlayerInfo& video_type,
      const content::MediaPlayerId& id,
      content::WebContentsObserver::MediaStoppedReason reason) override;
  void DidChangeThemeColor() override;
  void OnCursorChanged(const content::WebCursor& cursor) override;
  void DidAcquireFullscreen(content::RenderFrameHost* rfh) override;
  void OnWebContentsFocused(
      content::RenderWidgetHost* render_widget_host) override;
  void OnWebContentsLostFocus(
      content::RenderWidgetHost* render_widget_host) override;

  // InspectableWebContentsDelegate:
  void DevToolsReloadPage() override;

  // InspectableWebContentsViewDelegate:
  void DevToolsFocused() override;
  void DevToolsOpened() override;
  void DevToolsClosed() override;
  void DevToolsResized() override;

  ElectronBrowserContext* GetBrowserContext() const;

  void OnElectronBrowserConnectionError();

#if BUILDFLAG(ENABLE_OSR)
  OffScreenWebContentsView* GetOffScreenWebContentsView() const;
  OffScreenRenderWidgetHostView* GetOffScreenRenderWidgetHostView() const;
#endif

  // Called when received a synchronous message from renderer to
  // get the zoom level.
  void OnGetZoomLevel(content::RenderFrameHost* frame_host,
                      IPC::Message* reply_msg);

  void InitZoomController(content::WebContents* web_contents,
                          const gin_helper::Dictionary& options);

  // content::WebContentsDelegate:
  bool CanOverscrollContent() override;
  std::unique_ptr<content::EyeDropper> OpenEyeDropper(
      content::RenderFrameHost* frame,
      content::EyeDropperListener* listener) override;
  void RunFileChooser(content::RenderFrameHost* render_frame_host,
                      scoped_refptr<content::FileSelectListener> listener,
                      const blink::mojom::FileChooserParams& params) override;
  void EnumerateDirectory(content::WebContents* web_contents,
                          scoped_refptr<content::FileSelectListener> listener,
                          const base::FilePath& path) override;

  // ExclusiveAccessContext:
  Profile* GetProfile() override;
  bool IsFullscreen() const override;
  void EnterFullscreen(const GURL& url,
                       ExclusiveAccessBubbleType bubble_type,
                       const int64_t display_id) override;
  void ExitFullscreen() override;
  void UpdateExclusiveAccessExitBubbleContent(
      const GURL& url,
      ExclusiveAccessBubbleType bubble_type,
      ExclusiveAccessBubbleHideCallback bubble_first_hide_callback,
      bool force_update) override;
  void OnExclusiveAccessUserInput() override;
  content::WebContents* GetActiveWebContents() override;
  bool CanUserExitFullscreen() const override;
  bool IsExclusiveAccessBubbleDisplayed() const override;

  bool IsFullscreenForTabOrPending(const content::WebContents* source) override;
  bool TakeFocus(content::WebContents* source, bool reverse) override;
  content::PictureInPictureResult EnterPictureInPicture(
      content::WebContents* web_contents,
      const viz::SurfaceId&,
      const gfx::Size& natural_size) override;
  void ExitPictureInPicture() override;

  // InspectableWebContentsDelegate:
  void DevToolsSaveToFile(const std::string& url,
                          const std::string& content,
                          bool save_as) override;
  void DevToolsAppendToFile(const std::string& url,
                            const std::string& content) override;
  void DevToolsRequestFileSystems() override;
  void DevToolsAddFileSystem(const std::string& type,
                             const base::FilePath& file_system_path) override;
  void DevToolsRemoveFileSystem(
      const base::FilePath& file_system_path) override;
  void DevToolsIndexPath(int request_id,
                         const std::string& file_system_path,
                         const std::string& excluded_folders_message) override;
  void DevToolsStopIndexing(int request_id) override;
  void DevToolsSearchInPath(int request_id,
                            const std::string& file_system_path,
                            const std::string& query) override;
  void DevToolsSetEyeDropperActive(bool active) override;

  // InspectableWebContentsViewDelegate:
#if defined(TOOLKIT_VIEWS) && !BUILDFLAG(IS_MAC)
  ui::ImageModel GetDevToolsWindowIcon() override;
#endif
#if BUILDFLAG(IS_LINUX)
  void GetDevToolsWindowWMClass(std::string* name,
                                std::string* class_name) override;
#endif

  void ColorPickedInEyeDropper(int r, int g, int b, int a);

  // DevTools index event callbacks.
  void OnDevToolsIndexingWorkCalculated(int request_id,
                                        const std::string& file_system_path,
                                        int total_work);
  void OnDevToolsIndexingWorked(int request_id,
                                const std::string& file_system_path,
                                int worked);
  void OnDevToolsIndexingDone(int request_id,
                              const std::string& file_system_path);
  void OnDevToolsSearchCompleted(int request_id,
                                 const std::string& file_system_path,
                                 const std::vector<std::string>& file_paths);

  // Set fullscreen mode triggered by html api.
  void SetHtmlApiFullscreen(bool enter_fullscreen);
  // Update the html fullscreen flag in both browser and renderer.
  void UpdateHtmlApiFullscreen(bool fullscreen);

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

  // Whether the guest view has been attached.
  bool attached_ = false;

  // The zoom controller for this webContents.
  WebContentsZoomController* zoom_controller_ = nullptr;

  // The type of current WebContents.
  Type type_ = Type::kBrowserWindow;

  int32_t id_;

  // Request id used for findInPage request.
  uint32_t find_in_page_request_id_ = 0;

  // Whether background throttling is disabled.
  bool background_throttling_ = true;

  // Whether to enable devtools.
  bool enable_devtools_ = true;

  // Observers of this WebContents.
  base::ObserverList<ExtendedWebContentsObserver> observers_;

  v8::Global<v8::Value> pending_child_web_preferences_;

  // The window that this WebContents belongs to.
  base::WeakPtr<NativeWindow> owner_window_;

  bool offscreen_ = false;

  // Whether window is fullscreened by HTML5 api.
  bool html_fullscreen_ = false;

  // Whether window is fullscreened by window api.
  bool native_fullscreen_ = false;

  scoped_refptr<DevToolsFileSystemIndexer> devtools_file_system_indexer_;

  std::unique_ptr<ExclusiveAccessManager> exclusive_access_manager_;

  std::unique_ptr<DevToolsEyeDropper> eye_dropper_;

  ElectronBrowserContext* browser_context_;

  // The stored InspectableWebContents object.
  // Notice that inspectable_web_contents_ must be placed after
  // dialog_manager_, so we can make sure inspectable_web_contents_ is
  // destroyed before dialog_manager_, otherwise a crash would happen.
  std::unique_ptr<InspectableWebContents> inspectable_web_contents_;

  // Maps url to file path, used by the file requests sent from devtools.
  typedef std::map<std::string, base::FilePath> PathsMap;
  PathsMap saved_files_;

  // Map id to index job, used for file system indexing requests from devtools.
  typedef std::
      map<int, scoped_refptr<DevToolsFileSystemIndexer::FileSystemIndexingJob>>
          DevToolsIndexingJobsMap;
  DevToolsIndexingJobsMap devtools_indexing_jobs_;

  scoped_refptr<base::SequencedTaskRunner> file_task_runner_;

#if BUILDFLAG(ENABLE_PRINTING)
  scoped_refptr<base::TaskRunner> print_task_runner_;
#endif

  // Stores the frame thats currently in fullscreen, nullptr if there is none.
  content::RenderFrameHost* fullscreen_frame_ = nullptr;

  // In-memory cache that holds objects that have been granted permissions.
  DevicePermissionMap granted_devices_;

  base::WeakPtrFactory<WebContents> weak_factory_{this};
};

}  // namespace api

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_WEB_CONTENTS_H_
