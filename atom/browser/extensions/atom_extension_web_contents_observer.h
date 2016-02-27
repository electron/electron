// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_EXTENSIONS_ATOM_EXTENSION_WEB_CONTENTS_OBSERVER_H_
#define ATOM_BROWSER_EXTENSIONS_ATOM_EXTENSION_WEB_CONTENTS_OBSERVER_H_

#include <stdint.h>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "content/public/browser/web_contents_user_data.h"
#include "extensions/browser/extension_web_contents_observer.h"
#include "extensions/common/stack_frame.h"

namespace content {
class RenderFrameHost;
}

namespace extensions {

class AtomExtensionWebContentsObserver
    : public ExtensionWebContentsObserver,
      public content::WebContentsUserData<AtomExtensionWebContentsObserver> {
 private:
  friend class content::WebContentsUserData<AtomExtensionWebContentsObserver>;

  explicit AtomExtensionWebContentsObserver(
      content::WebContents* web_contents);
  ~AtomExtensionWebContentsObserver() override;

  bool OnMessageReceived(const IPC::Message& message,
                         content::RenderFrameHost* render_frame_host) override;

  // Adds a message to the extensions ErrorConsole.
  void OnDetailedConsoleMessageAdded(
      content::RenderFrameHost* render_frame_host,
      const base::string16& message,
      const base::string16& source,
      const StackTrace& stack_trace,
      int32_t severity_level);

  DISALLOW_COPY_AND_ASSIGN(AtomExtensionWebContentsObserver);
};

}  // namespace extensions

#endif  // ATOM_BROWSER_EXTENSIONS_ATOM_EXTENSION_WEB_CONTENTS_OBSERVER_H_
