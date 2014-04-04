// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_DEVTOOLS_WEB_CONTENTS_OBSERVER_H_
#define ATOM_BROWSER_DEVTOOLS_WEB_CONTENTS_OBSERVER_H_

#include "content/public/browser/web_contents_observer.h"

namespace base {
class ListValue;
}

namespace atom {

class NativeWindow;

class DevToolsWebContentsObserver : public content::WebContentsObserver {
 public:
  DevToolsWebContentsObserver(NativeWindow* native_window,
                              content::WebContents* web_contents);
  virtual ~DevToolsWebContentsObserver();

 protected:
  virtual bool OnMessageReceived(const IPC::Message& message) OVERRIDE;

  void OnRendererMessage(const string16& channel,
                         const base::ListValue& args);
  void OnRendererMessageSync(const string16& channel,
                             const base::ListValue& args,
                             IPC::Message* reply_msg);

 private:
  NativeWindow* inspected_window_;

  DISALLOW_COPY_AND_ASSIGN(DevToolsWebContentsObserver);
};

}  // namespace atom

#endif  // ATOM_BROWSER_DEVTOOLS_WEB_CONTENTS_OBSERVER_H_
