// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_WEB_CONTENTS_H_
#define ATOM_BROWSER_API_ATOM_API_WEB_CONTENTS_H_

#include "atom/browser/api/event_emitter.h"
#include "content/public/browser/browser_plugin_guest_delegate.h"
#include "content/public/browser/web_contents_observer.h"
#include "native_mate/handle.h"

namespace mate {
class Dictionary;
}

namespace atom {

namespace api {

class WebContents : public mate::EventEmitter,
                    public content::BrowserPluginGuestDelegate,
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
  void ExecuteJavaScript(const base::string16& code);
  bool SendIPCMessage(const base::string16& channel,
                      const base::ListValue& args);

 protected:
  explicit WebContents(content::WebContents* web_contents);
  explicit WebContents(const mate::Dictionary& options);
  ~WebContents();

  // mate::Wrappable:
  virtual mate::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) OVERRIDE;

  // content::WebContentsObserver:
  virtual void RenderViewDeleted(content::RenderViewHost*) OVERRIDE;
  virtual void RenderProcessGone(base::TerminationStatus status) OVERRIDE;
  virtual void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                             const GURL& validated_url) OVERRIDE;
  virtual void DidStartLoading(
      content::RenderViewHost* render_view_host) OVERRIDE;
  virtual void DidStopLoading(
      content::RenderViewHost* render_view_host) OVERRIDE;
  virtual bool OnMessageReceived(const IPC::Message& message) OVERRIDE;
  virtual void WebContentsDestroyed() OVERRIDE;

  // content::BrowserPluginGuestDelegate:
  virtual void WillAttach(content::WebContents* embedder_web_contents,
                          const base::DictionaryValue& extra_params) final;
  virtual void DidAttach() final;
  virtual int GetGuestInstanceID() const final;
  virtual void ElementSizeChanged(const gfx::Size& old_size,
                                  const gfx::Size& new_size) final;
  virtual void GuestSizeChanged(const gfx::Size& old_size,
                                const gfx::Size& new_size) final;
  virtual void RequestPointerLockPermission(
      bool user_gesture,
      bool last_unlocked_by_target,
      const base::Callback<void(bool)>& callback) final;
  virtual void RegisterDestructionCallback(
      const DestructionCallback& callback) final;

 private:
  // Called when received a message from renderer.
  void OnRendererMessage(const base::string16& channel,
                         const base::ListValue& args);

  // Called when received a synchronous message from renderer.
  void OnRendererMessageSync(const base::string16& channel,
                             const base::ListValue& args,
                             IPC::Message* message);

  // Unique ID for a guest WebContents.
  int guest_instance_id_;

  DestructionCallback destruction_callback_;

  // Stores the WebContents that managed by this class.
  scoped_ptr<content::WebContents> storage_;

  DISALLOW_COPY_AND_ASSIGN(WebContents);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_WEB_CONTENTS_H_
