// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_WEB_CONTENTS_H_
#define ATOM_BROWSER_API_ATOM_API_WEB_CONTENTS_H_

#include <string>
#include <vector>

#include "atom/browser/api/event_emitter.h"
#include "atom/browser/common_web_contents_delegate.h"
#include "content/public/browser/browser_plugin_guest_delegate.h"
#include "content/public/common/favicon_url.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/gpu_data_manager_observer.h"
#include "native_mate/handle.h"
#include "ui/gfx/image/image.h"

namespace brightray {
class InspectableWebContents;
}

namespace mate {
class Arguments;
class Dictionary;
}

namespace atom {

namespace api {

// A struct of parameters for SetSize(). The parameters are all declared as
// scoped pointers since they are all optional. Null pointers indicate that the
// parameter has not been provided, and the last used value should be used. Note
// that when |enable_auto_size| is true, providing |normal_size| is not
// meaningful. This is because the normal size of the guestview is overridden
// whenever autosizing occurs.
struct SetSizeParams {
  SetSizeParams() {}
  ~SetSizeParams() {}

  scoped_ptr<bool> enable_auto_size;
  scoped_ptr<gfx::Size> min_size;
  scoped_ptr<gfx::Size> max_size;
  scoped_ptr<gfx::Size> normal_size;
};

class WebContents : public mate::EventEmitter,
                    public content::BrowserPluginGuestDelegate,
                    public CommonWebContentsDelegate,
                    public content::WebContentsObserver,
                    public content::GpuDataManagerObserver {
 public:
  // For node.js callback function type: function(error, buffer)
  typedef base::Callback<void(v8::Local<v8::Value>, v8::Local<v8::Value>)>
      PrintToPDFCallback;

  // Create from an existing WebContents.
  static mate::Handle<WebContents> CreateFrom(
      v8::Isolate* isolate, brightray::InspectableWebContents* web_contents);
  static mate::Handle<WebContents> CreateFrom(
      v8::Isolate* isolate, content::WebContents* web_contents);

  // Create a new WebContents.
  static mate::Handle<WebContents> Create(
      v8::Isolate* isolate, const mate::Dictionary& options);

  void Destroy();
  bool IsAlive() const;
  int GetID() const;
  bool Equal(const WebContents* web_contents) const;
  void LoadURL(const GURL& url, const mate::Dictionary& options);
  base::string16 GetTitle() const;
  bool IsLoading() const;
  bool IsWaitingForResponse() const;
  void Stop();
  void ReloadIgnoringCache();
  void GoBack();
  void GoForward();
  void GoToOffset(int offset);
  bool IsCrashed() const;
  void SetUserAgent(const std::string& user_agent);
  void InsertCSS(const std::string& css);
  void ExecuteJavaScript(const base::string16& code);
  void OpenDevTools(mate::Arguments* args);
  void CloseDevTools();
  bool IsDevToolsOpened();
  void ToggleDevTools();
  void InspectElement(int x, int y);
  void InspectServiceWorker();
  v8::Local<v8::Value> Session(v8::Isolate* isolate);
  void HasServiceWorker(const base::Callback<void(bool)>&);
  void UnregisterServiceWorker(const base::Callback<void(bool)>&);
  void SetAudioMuted(bool muted);
  bool IsAudioMuted();
  void Print(mate::Arguments* args);

  // Print current page as PDF.
  void PrintToPDF(const base::DictionaryValue& setting,
                  const PrintToPDFCallback& callback);

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

  // Sending messages to browser.
  bool SendIPCMessage(const base::string16& channel,
                      const base::ListValue& args);

  // Used to toggle autosize mode for this GuestView, and set both the automatic
  // and normal sizes.
  void SetSize(const SetSizeParams& params);

  // Sets the transparency of the guest.
  void SetAllowTransparency(bool allow);

  bool IsGuest() const;

  // Returns whether this guest has an associated embedder.
  bool attached() const { return !!embedder_web_contents_; }

  // Returns the current InspectableWebContents object, nullptr will be returned
  // if current WebContents can not beinspected, e.g. it is the devtools.
  brightray::InspectableWebContents* inspectable_web_contents() const {
    return inspectable_web_contents_;
  }

 protected:
  explicit WebContents(brightray::InspectableWebContents* web_contents);
  explicit WebContents(content::WebContents* web_contents);
  explicit WebContents(const mate::Dictionary& options);
  ~WebContents();

  // mate::Wrappable:
  mate::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  // content::WebContentsDelegate:
  bool AddMessageToConsole(content::WebContents* source,
                           int32 level,
                           const base::string16& message,
                           int32 line_no,
                           const base::string16& source_id) override;
  bool ShouldCreateWebContents(
      content::WebContents* web_contents,
      int route_id,
      int main_frame_route_id,
      WindowContainerType window_container_type,
      const base::string16& frame_name,
      const GURL& target_url,
      const std::string& partition_id,
      content::SessionStorageNamespace* session_storage_namespace) override;
  void CloseContents(content::WebContents* source) override;
  content::WebContents* OpenURLFromTab(
      content::WebContents* source,
      const content::OpenURLParams& params) override;
  void HandleKeyboardEvent(
      content::WebContents* source,
      const content::NativeWebKeyboardEvent& event) override;
  void EnterFullscreenModeForTab(content::WebContents* source,
                                 const GURL& origin) override;
  void ExitFullscreenModeForTab(content::WebContents* source) override;

  // content::WebContentsObserver:
  void RenderViewDeleted(content::RenderViewHost*) override;
  void RenderProcessGone(base::TerminationStatus status) override;
  void DocumentLoadedInFrame(
      content::RenderFrameHost* render_frame_host) override;
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;
  void DidFailLoad(content::RenderFrameHost* render_frame_host,
                   const GURL& validated_url,
                   int error_code,
                   const base::string16& error_description) override;
  void DidFailProvisionalLoad(content::RenderFrameHost* render_frame_host,
                              const GURL& validated_url,
                              int error_code,
                              const base::string16& error_description) override;
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
  void RenderViewReady() override;
  void WebContentsDestroyed() override;
  void NavigationEntryCommitted(
      const content::LoadCommittedDetails& load_details) override;
  void TitleWasSet(content::NavigationEntry* entry, bool explicit_set) override;
  void DidUpdateFaviconURL(
      const std::vector<content::FaviconURL>& urls) override;
  void PluginCrashed(const base::FilePath& plugin_path,
                     base::ProcessId plugin_pid) override;

  // content::BrowserPluginGuestDelegate:
  void DidAttach(int guest_proxy_routing_id) final;
  content::WebContents* GetOwnerWebContents() const final;
  void GuestSizeChanged(const gfx::Size& new_size) final;
  void SetGuestHost(content::GuestHost* guest_host) final;
  void WillAttach(content::WebContents* embedder_web_contents,
                  int element_instance_id,
                  bool is_full_page_plugin) final;

  // content::GpuDataManagerObserver:
  void OnGpuProcessCrashed(base::TerminationStatus exit_code) override;

 private:
  // Called when received a message from renderer.
  void OnRendererMessage(const base::string16& channel,
                         const base::ListValue& args);

  // Called when received a synchronous message from renderer.
  void OnRendererMessageSync(const base::string16& channel,
                             const base::ListValue& args,
                             IPC::Message* message);

  // This method is invoked when the contents auto-resized to give the container
  // an opportunity to match it if it wishes.
  //
  // This gives the derived class an opportunity to inform its container element
  // or perform other actions.
  void GuestSizeChangedDueToAutoSize(const gfx::Size& old_size,
                                     const gfx::Size& new_size);

  // Returns the default size of the guestview.
  gfx::Size GetDefaultSize() const;


  v8::Global<v8::Value> session_;

  // Stores whether the contents of the guest can be transparent.
  bool guest_opaque_;

  // The WebContents that attaches this guest view.
  content::WebContents* embedder_web_contents_;

  // The size of the container element.
  gfx::Size element_size_;

  // The size of the guest content. Note: In autosize mode, the container
  // element may not match the size of the guest.
  gfx::Size guest_size_;

  // A pointer to the guest_host.
  content::GuestHost* guest_host_;

  // Indicates whether autosize mode is enabled or not.
  bool auto_size_enabled_;

  // The maximum size constraints of the container element in autosize mode.
  gfx::Size max_auto_size_;

  // The minimum size constraints of the container element in autosize mode.
  gfx::Size min_auto_size_;

  // The size that will be used when autosize mode is disabled.
  gfx::Size normal_size_;

  // Whether the guest view is inside a plugin document.
  bool is_full_page_plugin_;

  // Current InspectableWebContents object, can be nullptr for WebContents of
  // devtools. It is a weak reference.
  brightray::InspectableWebContents* inspectable_web_contents_;

  DISALLOW_COPY_AND_ASSIGN(WebContents);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_WEB_CONTENTS_H_
