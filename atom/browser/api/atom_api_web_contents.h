// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_WEB_CONTENTS_H_
#define ATOM_BROWSER_API_ATOM_API_WEB_CONTENTS_H_

#include <string>

#include "atom/browser/api/event_emitter.h"
#include "content/public/browser/browser_plugin_guest_delegate.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_contents_observer.h"
#include "native_mate/handle.h"

namespace mate {
class Dictionary;
}

namespace atom {

namespace api {

class WebContents : public mate::EventEmitter,
                    public content::BrowserPluginGuestDelegate,
                    public content::WebContentsDelegate,
                    public content::WebContentsObserver {
 public:
  // Create from an existing WebContents.
  static mate::Handle<WebContents> CreateFrom(
      v8::Isolate* isolate, content::WebContents* web_contents);

  // Create a new WebContents.
  static mate::Handle<WebContents> Create(
      v8::Isolate* isolate, const mate::Dictionary& options);

  void Destroy();
  bool IsAlive() const;
  void LoadURL(const GURL& url);
  GURL GetURL() const;
  base::string16 GetTitle() const;
  bool IsLoading() const;
  bool IsWaitingForResponse() const;
  void Stop();
  void Reload();
  void ReloadIgnoringCache();
  bool CanGoBack() const;
  bool CanGoForward() const;
  bool CanGoToOffset(int offset) const;
  void GoBack();
  void GoForward();
  void GoToIndex(int index);
  void GoToOffset(int offset);
  int GetRoutingID() const;
  int GetProcessID() const;
  bool IsCrashed() const;
  void SetUserAgent(const std::string& user_agent);
  void ExecuteJavaScript(const base::string16& code);
  bool SendIPCMessage(const base::string16& channel,
                      const base::ListValue& args);

  // Toggles autosize mode for corresponding <webview>.
  void SetAutoSize(bool enabled,
                   const gfx::Size& min_size,
                   const gfx::Size& max_size);

  // Sets the transparency of the guest.
  void SetAllowTransparency(bool allow);

  // Returns whether this is a guest view.
  bool is_guest() const { return guest_instance_id_ != -1; }

  // Returns whether this guest has an associated embedder.
  bool attached() const { return !!embedder_web_contents_; }

  content::WebContents* web_contents() const {
    return content::WebContentsObserver::web_contents();
  }

 protected:
  explicit WebContents(content::WebContents* web_contents);
  explicit WebContents(const mate::Dictionary& options);
  ~WebContents();

  // mate::Wrappable:
  virtual mate::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  // content::WebContentsDelegate:
  virtual content::WebContents* OpenURLFromTab(
      content::WebContents* source,
      const content::OpenURLParams& params) override;

  // content::WebContentsObserver:
  virtual void RenderViewDeleted(content::RenderViewHost*) override;
  virtual void RenderProcessGone(base::TerminationStatus status) override;
  virtual void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                             const GURL& validated_url) override;
  virtual void DidStartLoading(
      content::RenderViewHost* render_view_host) override;
  virtual void DidStopLoading(
      content::RenderViewHost* render_view_host) override;
  virtual bool OnMessageReceived(const IPC::Message& message) override;
  virtual void RenderViewReady() override;
  virtual void WebContentsDestroyed() override;

  // content::BrowserPluginGuestDelegate:
  virtual void WillAttach(content::WebContents* embedder_web_contents,
                          const base::DictionaryValue& extra_params) override;
  virtual content::WebContents* CreateNewGuestWindow(
      const content::WebContents::CreateParams& create_params) override;
  virtual void DidAttach() override;
  virtual int GetGuestInstanceID() const override;
  virtual void ElementSizeChanged(const gfx::Size& old_size,
                                  const gfx::Size& new_size) override;
  virtual void GuestSizeChanged(const gfx::Size& old_size,
                                const gfx::Size& new_size) override;
  virtual void RequestPointerLockPermission(
      bool user_gesture,
      bool last_unlocked_by_target,
      const base::Callback<void(bool enabled)>& callback) override;
  virtual void RegisterDestructionCallback(
      const DestructionCallback& callback) override;

 private:
  // Called when received a message from renderer.
  void OnRendererMessage(const base::string16& channel,
                         const base::ListValue& args);

  // Called when received a synchronous message from renderer.
  void OnRendererMessageSync(const base::string16& channel,
                             const base::ListValue& args,
                             IPC::Message* message);

  void GuestSizeChangedDueToAutoSize(const gfx::Size& old_size,
                                     const gfx::Size& new_size);

  // Unique ID for a guest WebContents.
  int guest_instance_id_;

  DestructionCallback destruction_callback_;

  // Stores whether the contents of the guest can be transparent.
  bool guest_opaque_;

  // The extra parameters associated with this guest view passed
  // in from JavaScript. This will typically be the view instance ID,
  // the API to use, and view-specific parameters. These parameters
  // are passed along to new guests that are created from this guest.
  scoped_ptr<base::DictionaryValue> extra_params_;

  // Stores the WebContents that managed by this class.
  scoped_ptr<content::WebContents> storage_;

  // The WebContents that attaches this guest view.
  content::WebContents* embedder_web_contents_;

  // The size of the container element.
  gfx::Size element_size_;

  // The size of the guest content. Note: In autosize mode, the container
  // element may not match the size of the guest.
  gfx::Size guest_size_;

  // Indicates whether autosize mode is enabled or not.
  bool auto_size_enabled_;

  // The maximum size constraints of the container element in autosize mode.
  gfx::Size max_auto_size_;

  // The minimum size constraints of the container element in autosize mode.
  gfx::Size min_auto_size_;

  DISALLOW_COPY_AND_ASSIGN(WebContents);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_WEB_CONTENTS_H_
