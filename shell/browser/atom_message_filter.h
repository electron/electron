// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_ATOM_MESSAGE_FILTER_H_
#define SHELL_BROWSER_ATOM_MESSAGE_FILTER_H_

#include "content/public/browser/browser_message_filter.h"
#include "content/public/browser/web_contents.h"

namespace electron {

// This filter class routes IPC messages to WebContents that it does not receive
// otherwise, but we want to handle in api::WebContents. By default,
// WebContentsImpl notifies its observers when it receives messages from the
// RenderFrameHost, but we listen to the WidgetHostMsg_SetCursor message to
// detect when the cursor changes, so we need to forward that message to the
// WebContentsImpl. Related issue: #18213.
class AtomMessageFilter : public content::BrowserMessageFilter {
 public:
  explicit AtomMessageFilter(content::WebContents* web_contents);

  // content::BrowserMessageFilter implementation.
  bool OnMessageReceived(const IPC::Message& message) override;
  void OnChannelClosing() override;
  void OnDestruct() const override;

 private:
  friend class content::BrowserThread;
  friend class base::DeleteHelper<AtomMessageFilter>;

  ~AtomMessageFilter() override;

  void ForwardMessage(const IPC::Message& message);
  void ForwardMessageOnUIThread(const IPC::Message& message,
                                content::WebContentsImpl* web_contents_impl);

  bool Valid();

  content::WebContents* web_contents_;
  mutable base::Lock mutex_;
  mutable bool valid_;

  DISALLOW_COPY_AND_ASSIGN(AtomMessageFilter);
};

}  // namespace electron

#endif  // SHELL_BROWSER_ATOM_MESSAGE_FILTER_H_
