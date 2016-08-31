// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_WEB_CONTENTS_H_
#define ATOM_BROWSER_API_ATOM_API_WEB_CONTENTS_H_

#include <string>
#include <vector>

#include "atom/browser/api/frame_subscriber.h"
#include "atom/browser/api/save_page_handler.h"
#include "atom/browser/api/trackable_object.h"
#include "atom/browser/common_web_contents_delegate.h"
#include "atom/common/options_switches.h"
#include "content/common/view_messages.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/common/favicon_url.h"
#include "content/common/cursors/webcursor.h"
#include "native_mate/handle.h"
#include "ui/gfx/image/image.h"

namespace autofill {
class AtomAutofillClient;
}

namespace blink {
struct WebDeviceEmulationParams;
}

namespace brightray {
class InspectableWebContents;
}

namespace mate {

template<>
struct Converter<WindowOpenDisposition> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   WindowOpenDisposition val) {
    std::string disposition = "other";
    switch (val) {
      case CURRENT_TAB: disposition = "default"; break;
      case NEW_FOREGROUND_TAB: disposition = "foreground-tab"; break;
      case NEW_BACKGROUND_TAB: disposition = "background-tab"; break;
      case NEW_POPUP: disposition = "new-popup"; break;
      case NEW_WINDOW: disposition = "new-window"; break;
      default: disposition = "other"; break;
    }
    return mate::ConvertToV8(isolate, disposition);
  }
};

class Arguments;
class Dictionary;
}  // namespace mate

namespace atom {

struct SetSizeParams;
class AtomBrowserContext;
class WebViewGuestDelegate;

namespace api {

class WebContents : public mate::TrackableObject<WebContents>,
                    public CommonWebContentsDelegate,
                    public content::WebContentsObserver {
 public:
  enum Type {
    BACKGROUND_PAGE,  // A DevTools extension background page.
    BROWSER_WINDOW,  // Used by BrowserWindow.
    REMOTE,  // Thin wrap around an existing WebContents.
    WEB_VIEW,  // Used by <webview>.
    OFF_SCREEN,  // Used for offscreen rendering
  };

  // For node.js callback function type: function(error, buffer)
  using PrintToPDFCallback =
      base::Callback<void(v8::Local<v8::Value>, v8::Local<v8::Value>)>;

  // Create from an existing WebContents.
  static mate::Handle<WebContents> CreateFrom(
      v8::Isolate* isolate, content::WebContents* web_contents);

  // Create a new WebContents.
  static mate::Handle<WebContents> Create(
      v8::Isolate* isolate, const mate::Dictionary& options);

  // Create a new WebContents with CreateParams
  static mate::Handle<WebContents> CreateWithParams(
      v8::Isolate* isolate,
      const mate::Dictionary& options,
      const content::WebContents::CreateParams& params);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  int GetID() const;
  Type GetType() const;
  bool Equal(const WebContents* web_contents) const;
  void LoadURL(const GURL& url, const mate::Dictionary& options);
  void Reload(bool ignore_cache);
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
  bool CanGoToOffset(int offset) const;
  bool CanGoBack() const;
  bool CanGoForward() const;
  void GoToIndex(int index);
  const GURL& GetURLAtIndex(int index) const;
  const base::string16 GetTitleAtIndex(int index) const;
  int GetCurrentEntryIndex() const;
  int GetLastCommittedEntryIndex() const;
  int GetEntryCount() const;
  const std::string& GetWebRTCIPHandlingPolicy() const;
  void SetWebRTCIPHandlingPolicy(const std::string webrtc_ip_handling_policy);
  void ShowRepostFormWarningDialog(content::WebContents* source) override;
  bool IsCrashed() const;
  void SetUserAgent(const std::string& user_agent, mate::Arguments* args);
  std::string GetUserAgent();
  void InsertCSS(const std::string& css);
  bool SavePage(const base::FilePath& full_file_path,
                const content::SavePageType& save_type,
                const SavePageHandler::SavePageCallback& callback);
  void OpenDevTools(mate::Arguments* args);
  void CloseDevTools();
  bool IsDevToolsOpened();
  bool IsDevToolsFocused();
  void ToggleDevTools();
  void EnableDeviceEmulation(const blink::WebDeviceEmulationParams& params);
  void DisableDeviceEmulation();
  void InspectElement(int x, int y);
  void InspectServiceWorker();
  void HasServiceWorker(const base::Callback<void(bool)>&);
  void UnregisterServiceWorker(const base::Callback<void(bool)>&);
  void SetAudioMuted(bool muted);
  bool IsAudioMuted();
  void Print(mate::Arguments* args);
  int GetContentWindowId();
  void ResumeLoadingCreatedWebContents();

  // Print current page as PDF.
  void PrintToPDF(const base::DictionaryValue& setting,
                  const PrintToPDFCallback& callback);

  // DevTools workspace api.
  void AddWorkSpace(mate::Arguments* args, const base::FilePath& path);
  void RemoveWorkSpace(mate::Arguments* args, const base::FilePath& path);

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
  uint32_t FindInPage(mate::Arguments* args);
  void StopFindInPage(content::StopFindAction action);
  void ShowDefinitionForSelection();
  void CopyImageAt(int x, int y);
  mate::Handle<WebContents> Clone(const mate::Dictionary& options);

  // Focus.
  void Focus();
  bool IsFocused() const;
  void TabTraverse(bool reverse);
  void SetActive(bool active);

  // Zoom
  void SetZoomLevel(double zoom);
  void ZoomIn();
  void ZoomOut();
  void ZoomReset();
  int GetZoomPercent();

#if defined(ENABLE_EXTENSIONS)
  bool ExecuteScriptInTab(const std::string code_string,
      const std::string extension_id,
      const mate::Dictionary& options);
#endif

  // Send messages to browser.
  bool SendIPCMessage(bool all_frames,
                      const base::string16& channel,
                      const base::ListValue& args);

  // Send WebInputEvent to the page.
  void SendInputEvent(v8::Isolate* isolate, v8::Local<v8::Value> input_event);

  // Subscribe to the frame updates.
  void BeginFrameSubscription(mate::Arguments* args);
  void EndFrameSubscription();

  // Dragging native items.
  void StartDrag(const mate::Dictionary& item, mate::Arguments* args);

  // Captures the page with |rect|, |callback| would be called when capturing is
  // done.
  void CapturePage(mate::Arguments* args);

  // Methods for creating <webview>.
  void SetSize(const SetSizeParams& params);
  bool IsGuest() const;

  // Methods for offscreen rendering
  bool IsOffScreen() const;
  void OnPaint(const gfx::Rect& dirty_rect, const SkBitmap& bitmap);
  void StartPainting();
  void StopPainting();
  bool IsPainting() const;
  void SetFrameRate(int frame_rate);
  int GetFrameRate() const;

  // Callback triggered on permission response.
  void OnEnterFullscreenModeForTab(content::WebContents* source,
                                   const GURL& origin,
                                   bool allowed);

  // Create window with the given disposition.
  void OnCreateWindow(const GURL& target_url,
                      const std::string& frame_name,
                      WindowOpenDisposition disposition);

  void AutofillSelect(const std::string& value, int frontend_id, int index);
  void AutofillPopupHidden();

  // Returns the web preferences of current WebContents.
  v8::Local<v8::Value> GetWebPreferences(v8::Isolate* isolate);

  // Returns the owner window.
  v8::Local<v8::Value> GetOwnerBrowserWindow();

  // Properties.
  int32_t ID() const;
  v8::Local<v8::Value> Session(v8::Isolate* isolate);
  content::WebContents* HostWebContents();
  v8::Local<v8::Value> DevToolsWebContents(v8::Isolate* isolate);
  v8::Local<v8::Value> Debugger(v8::Isolate* isolate);

 protected:
  WebContents(v8::Isolate* isolate, content::WebContents* web_contents);
  WebContents(v8::Isolate* isolate, const mate::Dictionary& options,
              const content::WebContents::CreateParams* create_params = NULL);
  ~WebContents();

  // content::WebContentsDelegate:
  bool AddMessageToConsole(content::WebContents* source,
                           int32_t level,
                           const base::string16& message,
                           int32_t line_no,
                           const base::string16& source_id) override;
  bool ShouldCreateWebContents(
      content::WebContents* web_contents,
      int32_t route_id,
      int32_t main_frame_route_id,
      int32_t main_frame_widget_route_id,
      WindowContainerType window_container_type,
      const std::string& frame_name,
      const GURL& target_url,
      const std::string& partition_id,
      content::SessionStorageNamespace* session_storage_namespace) override;
  void WebContentsCreated(content::WebContents* source_contents,
                          int opener_render_frame_id,
                          const std::string& frame_name,
                          const GURL& target_url,
                          content::WebContents* new_contents) override;
  void AddNewContents(content::WebContents* source,
                      content::WebContents* new_contents,
                      WindowOpenDisposition disposition,
                      const gfx::Rect& initial_rect,
                      bool user_gesture,
                      bool* was_blocked) override;
  bool ShouldResumeRequestsForCreatedWindow() override;
  bool IsAttached();
  content::WebContents* OpenURLFromTab(
      content::WebContents* source,
      const content::OpenURLParams& params) override;
  void BeforeUnloadFired(content::WebContents* tab,
                         bool proceed,
                         bool* proceed_to_fire_unload) override;
  void MoveContents(content::WebContents* source,
                    const gfx::Rect& pos) override;
  void CloseContents(content::WebContents* source) override;
  void ActivateContents(content::WebContents* contents) override;
  void UpdateTargetURL(content::WebContents* source, const GURL& url) override;
  void LoadProgressChanged(content::WebContents* source,
                                   double progress) override;
  bool IsPopupOrPanel(const content::WebContents* source) const override;
  void HandleKeyboardEvent(
      content::WebContents* source,
      const content::NativeWebKeyboardEvent& event) override;
  void EnterFullscreenModeForTab(content::WebContents* source,
                                 const GURL& origin) override;
  void ExitFullscreenModeForTab(content::WebContents* source) override;
  void RendererUnresponsive(content::WebContents* source) override;
  void RendererResponsive(content::WebContents* source) override;
  bool HandleContextMenu(const content::ContextMenuParams& params) override;
  bool OnGoToEntryOffset(int offset) override;
  void FindReply(content::WebContents* web_contents,
                 int request_id,
                 int number_of_matches,
                 const gfx::Rect& selection_rect,
                 int active_match_ordinal,
                 bool final_update) override;
  bool CheckMediaAccessPermission(
      content::WebContents* web_contents,
      const GURL& security_origin,
      content::MediaStreamType type) override;
  void RequestMediaAccessPermission(
      content::WebContents* web_contents,
      const content::MediaStreamRequest& request,
      const content::MediaResponseCallback& callback) override;
  void RequestToLockMouse(
      content::WebContents* web_contents,
      bool user_gesture,
      bool last_unlocked_by_target) override;
  std::unique_ptr<content::BluetoothChooser> RunBluetoothChooser(
      content::RenderFrameHost* frame,
      const content::BluetoothChooser::EventHandler& handler) override;

  // content::WebContentsObserver:
  void BeforeUnloadFired(const base::TimeTicks& proceed_time) override;
  void RenderViewDeleted(content::RenderViewHost*) override;
  void RenderProcessGone(base::TerminationStatus status) override;
  void DocumentLoadedInFrame(
      content::RenderFrameHost* render_frame_host) override;
  void DidStartProvisionalLoadForFrame(
      content::RenderFrameHost* render_frame_host,
      const GURL& validated_url,
      bool is_error_page,
      bool is_iframe_srcdoc) override;
  void DidCommitProvisionalLoadForFrame(
      content::RenderFrameHost* render_frame_host,
      const GURL& url,
      ui::PageTransition transition_type) override;
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;
  void DidFailLoad(content::RenderFrameHost* render_frame_host,
                   const GURL& validated_url,
                   int error_code,
                   const base::string16& error_description,
                   bool was_ignored_by_handler) override;
  void DidFailProvisionalLoad(content::RenderFrameHost* render_frame_host,
                              const GURL& validated_url,
                              int error_code,
                              const base::string16& error_description,
                              bool was_ignored_by_handler) override;
  void DidStartLoading() override;
  void DidStopLoading() override;
  void DidGetResourceResponseStart(
      const content::ResourceRequestDetails& details) override;
  void DidGetRedirectForResourceRequest(
      content::RenderFrameHost* render_frame_host,
      const content::ResourceRedirectDetails& details) override;
  void DidNavigateMainFrame(
      const content::LoadCommittedDetails& details,
      const content::FrameNavigateParams& params) override;
  bool OnMessageReceived(const IPC::Message& message) override;
  void WebContentsDestroyed() override;
  void NavigationEntryCommitted(
      const content::LoadCommittedDetails& load_details) override;
  void TitleWasSet(content::NavigationEntry* entry, bool explicit_set) override;
  void DidUpdateFaviconURL(
      const std::vector<content::FaviconURL>& urls) override;
  void PluginCrashed(const base::FilePath& plugin_path,
                     base::ProcessId plugin_pid) override;
  void MediaStartedPlaying(const MediaPlayerId& id) override;
  void MediaStoppedPlaying(const MediaPlayerId& id) override;
  void DidChangeThemeColor(SkColor theme_color) override;

  // brightray::InspectableWebContentsDelegate:
  void DevToolsReloadPage() override;

  // brightray::InspectableWebContentsViewDelegate:
  void DevToolsFocused() override;
  void DevToolsOpened() override;
  void DevToolsClosed() override;

 private:
  AtomBrowserContext* GetBrowserContext() const;

  uint32_t GetNextRequestId() {
    return ++request_id_;
  }

  // Called when we receive a CursorChange message from chromium.
  void OnCursorChange(const content::WebCursor& cursor);

  // Called when received a message from renderer.
  void OnRendererMessage(const base::string16& channel,
                         const base::ListValue& args);

  // Called when received a synchronous message from renderer.
  void OnRendererMessageSync(const base::string16& channel,
                             const base::ListValue& args,
                             IPC::Message* message);

  v8::Global<v8::Value> session_;
  v8::Global<v8::Value> devtools_web_contents_;
  v8::Global<v8::Value> debugger_;

  std::unique_ptr<WebViewGuestDelegate> guest_delegate_;

  // The host webcontents that may contain this webcontents.
  WebContents* embedder_;

  // The type of current WebContents.
  Type type_;

  // Request id used for findInPage request.
  uint32_t request_id_;

  // When a new tab is created asynchronously, stores the LoadURLParams
  // needed to continue loading the page once the tab is ready.
  std::unique_ptr<content::NavigationController::LoadURLParams>
    delayed_load_url_params_;

  // Whether background throttling is disabled.
  bool background_throttling_;

  // When a new tab is created asynchronously, stores the OpenURLParams needed
  // to continue loading the page once the tab is ready.
  std::unique_ptr<content::OpenURLParams> delayed_open_url_params_;

  DISALLOW_COPY_AND_ASSIGN(WebContents);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_WEB_CONTENTS_H_
