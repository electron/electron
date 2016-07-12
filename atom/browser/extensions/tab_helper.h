// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_EXTENSIONS_TAB_HELPER_H_
#define ATOM_BROWSER_EXTENSIONS_TAB_HELPER_H_

#include <string>
#include "base/macros.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "extensions/browser/extension_function_dispatcher.h"
#include "extensions/browser/script_execution_observer.h"
#include "extensions/browser/script_executor.h"

namespace base {
class DictionaryValue;
}

namespace content {
class BrowserContext;
class RenderFrameHost;
class RenderViewHost;
}

namespace mate {
class Dictionary;
}

namespace keys {
extern const char kIdKey[];
extern const char kTabIdKey[];
extern const char kIncognitoKey[];
extern const char kWindowIdKey[];
}

namespace extensions {

class Extension;

// This class keeps the extension API's windowID up-to-date with the current
// window of the tab.
class TabHelper : public content::WebContentsObserver,
                  public content::WebContentsUserData<TabHelper> {
 public:
  ~TabHelper() override;

  // Identifier of the tab.
  void SetTabId(content::RenderFrameHost* render_frame_host);
  const int32_t& session_id() const { return session_id_; }

  // Identifier of the window the tab is in.
  void SetWindowId(const int32_t& id);
  const int32_t& window_id() const { return window_id_; }

  // Set this tab as the active tab in its window
  void SetActive(bool active);

  bool ExecuteScriptInTab(
    const std::string extension_id,
    const std::string code_string,
    const mate::Dictionary& options);

  ScriptExecutor* script_executor() {
    return script_executor_.get();
  }

  // If the specified WebContents has a TabHelper (probably because it
  // was used as the contents of a tab), returns a tab id. This value is
  // immutable for a given tab. It will be unique across Chrome within the
  // current session, but may be re-used across sessions. Returns -1
  // for a NULL WebContents or if the WebContents has no TabHelper.
  static int32_t IdForTab(const content::WebContents* tab);

  static content::WebContents* GetTabById(int tab_id,
                         content::BrowserContext* browser_context);

  static int32_t IdForWindowContainingTab(
      const content::WebContents* tab);

  static base::DictionaryValue* CreateTabValue(
      content::WebContents* web_contents);

 private:
  explicit TabHelper(content::WebContents* contents);
  friend class content::WebContentsUserData<TabHelper>;

  void ExecuteScript(
      std::string extension_id,
      const mate::Dictionary& options,
      bool success,
      const std::string& code_string);

  // content::WebContentsObserver overrides.
  void RenderViewCreated(content::RenderViewHost* render_view_host) override;
  void RenderFrameCreated(content::RenderFrameHost* host) override;
  void WebContentsDestroyed() override;
  void DidCloneToNewWebContents(
      content::WebContents* old_web_contents,
      content::WebContents* new_web_contents) override;

  // Our content script observers. Declare at top so that it will outlive all
  // other members, since they might add themselves as observers.
  base::ObserverList<ScriptExecutionObserver> script_execution_observers_;

  // Unique identifier of the tab for session restore. This id is only unique
  // within the current session, and is not guaranteed to be unique across
  // sessions.
  int32_t session_id_ = -1;

  // Unique identifier of the window the tab is in.
  int32_t window_id_ = -1;

  std::unique_ptr<ScriptExecutor> script_executor_;

  DISALLOW_COPY_AND_ASSIGN(TabHelper);
};

}  // namespace extensions

#endif  // ATOM_BROWSER_EXTENSIONS_TAB_HELPER_H_
