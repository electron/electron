// Copyright (c) 2026 Anthropic PBC
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_TAB_REGISTRY_H_
#define ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_TAB_REGISTRY_H_

#include <optional>
#include <set>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/supports_user_data.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "content/public/browser/web_contents_user_data.h"
#include "electron/shell/common/extensions/api/tabs.h"
#include "extensions/common/mojom/view_type.mojom.h"

namespace content {
class BrowserContext;
class WebContents;
}  // namespace content

namespace extensions {

class ElectronExtensionTabRegistry;

// Which WebContents of a session are tabs to the session's extensions.
enum class ElectronTabMode {
  // Every WebContents, unless it is registered as a view that is not a tab.
  kAllWebContents,
  // Only the WebContents registered as tabs.
  kRegistered,
};

// What the app registered about one WebContents through Extensions.addView().
// Present on every registered view. A registered view is not necessarily a
// tab: popups and side panels are registered too, so ask is_tab(), or better
// extensions::IsTab(), never "does this WebContents have tab data?".
class ElectronExtensionTabData
    : public content::WebContentsUserData<ElectronExtensionTabData> {
 public:
  ~ElectronExtensionTabData() override;

  ElectronExtensionTabData(const ElectronExtensionTabData&) = delete;
  ElectronExtensionTabData& operator=(const ElectronExtensionTabData&) = delete;

  mojom::ViewType view_type() const { return view_type_; }
  bool is_tab() const { return view_type_ == mojom::ViewType::kTabContents; }
  int window_id() const { return window_id_; }
  // -1 for a view that is not a tab.
  int index() const { return index_; }
  bool active() const { return active_; }
  // The other fields the app supplied, custom ones included.
  const base::DictValue& details() const { return details_; }

  // The registered fields as a chrome.tabs.Tab. Only for tabs. Fields that
  // Electron derives from the WebContents are left for the caller to write.
  electron_api::tabs::Tab CreateTab() const;

 private:
  friend class content::WebContentsUserData<ElectronExtensionTabData>;
  friend class ElectronExtensionTabRegistry;

  ElectronExtensionTabData(content::WebContents* web_contents,
                           base::WeakPtr<ElectronExtensionTabRegistry> registry,
                           mojom::ViewType view_type);

  base::WeakPtr<ElectronExtensionTabRegistry> registry_;
  const mojom::ViewType view_type_;
  int window_id_ = -1;
  int index_ = -1;
  bool active_ = false;
  base::DictValue details_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

// The views an app registered for one BrowserContext, and what "current
// window" means for callers that are not in a tab. The app pushes this state
// in from the main process, so the lookups made while handling network
// requests and messages never call into JavaScript.
class ElectronExtensionTabRegistry : public base::SupportsUserData::Data {
 public:
  // Never returns null.
  static ElectronExtensionTabRegistry* Get(content::BrowserContext* context);

  explicit ElectronExtensionTabRegistry(content::BrowserContext* context);
  ~ElectronExtensionTabRegistry() override;

  ElectronExtensionTabRegistry(const ElectronExtensionTabRegistry&) = delete;
  ElectronExtensionTabRegistry& operator=(const ElectronExtensionTabRegistry&) =
      delete;

  ElectronTabMode mode() const { return mode_; }
  void set_mode(ElectronTabMode mode) { mode_ = mode; }

  std::optional<int> focused_window_id() const { return focused_window_id_; }
  void set_focused_window_id(std::optional<int> window_id) {
    focused_window_id_ = window_id;
  }

  // Registers |web_contents|, which must belong to this registry's
  // BrowserContext. |details| is the object passed to Extensions.addView().
  // On failure, returns the error to throw and changes nothing.
  base::expected<void, std::string> AddView(content::WebContents* web_contents,
                                            const base::DictValue& details);

  // Merges |details| into a registered view. A key set to null is removed.
  // On failure, returns the error to throw and changes nothing.
  base::expected<void, std::string> UpdateView(
      content::WebContents* web_contents,
      const base::DictValue& details);

  // Does nothing if |web_contents| is not registered.
  void RemoveView(content::WebContents* web_contents);

  // The active registered tab of |window_id|, or null.
  content::WebContents* GetActiveTab(int window_id) const;

 private:
  friend class ElectronExtensionTabData;

  struct ParsedDetails;

  static base::expected<ParsedDetails, std::string> ParseDetails(
      mojom::ViewType view_type,
      const ElectronExtensionTabData* current,
      const base::DictValue& details);

  void Apply(ElectronExtensionTabData* data, ParsedDetails parsed);
  void OnViewDestroyed(content::WebContents* web_contents);

  const raw_ptr<content::BrowserContext> context_;
  ElectronTabMode mode_ = ElectronTabMode::kAllWebContents;
  std::optional<int> focused_window_id_;
  std::set<raw_ptr<content::WebContents>> views_;

  base::WeakPtrFactory<ElectronExtensionTabRegistry> weak_factory_{this};
};

}  // namespace extensions

#endif  // ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_TAB_REGISTRY_H_
