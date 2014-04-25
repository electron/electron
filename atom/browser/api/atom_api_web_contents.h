// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_WEB_CONTENTS_H_
#define ATOM_BROWSER_API_ATOM_API_WEB_CONTENTS_H_

#include "atom/browser/api/event_emitter.h"
#include "content/public/browser/web_contents_observer.h"
#include "native_mate/handle.h"

namespace atom {

namespace api {

class WebContents : public mate::EventEmitter,
                    public content::WebContentsObserver {
 public:
  static mate::Handle<WebContents> Create(v8::Isolate* isolate,
                                          content::WebContents* web_contents);

  bool IsAlive() const;
  void LoadURL(const GURL& url);
  GURL GetURL() const;
  string16 GetTitle() const;
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
  void ExecuteJavaScript(const string16& code);
  bool SendIPCMessage(const string16& channel, const base::ListValue& args);

 protected:
  explicit WebContents(content::WebContents* web_contents);

  // mate::Wrappable implementations:
  virtual mate::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) OVERRIDE;

  // content::WebContentsObserver implementations:
  virtual void RenderViewDeleted(content::RenderViewHost*) OVERRIDE;
  virtual void RenderProcessGone(base::TerminationStatus status) OVERRIDE;
  virtual void DidFinishLoad(
      int64 frame_id,
      const GURL& validated_url,
      bool is_main_frame,
      content::RenderViewHost* render_view_host) OVERRIDE;
  virtual void DidStartLoading(
      content::RenderViewHost* render_view_host) OVERRIDE;
  virtual void DidStopLoading(
      content::RenderViewHost* render_view_host) OVERRIDE;
  virtual bool OnMessageReceived(const IPC::Message& message) OVERRIDE;
  virtual void WebContentsDestroyed(content::WebContents*) OVERRIDE;

 private:
  // Called when received a message from renderer.
  void OnRendererMessage(const string16& channel, const base::ListValue& args);

  // Called when received a synchronous message from renderer.
  void OnRendererMessageSync(const string16& channel,
                             const base::ListValue& args,
                             IPC::Message* message);

  content::WebContents* web_contents_;  // Weak.

  DISALLOW_COPY_AND_ASSIGN(WebContents);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_WEB_CONTENTS_H_
