// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_MENU_ROLES_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_MENU_ROLES_H_

#include <optional>
#include <string>
#include <string_view>

#include "v8/include/v8-forward.h"
#include "v8/include/v8-local-handle.h"

namespace electron::api {
class BaseWindow;
class WebContents;
}  // namespace electron::api

// The built-in MenuItem roles.
namespace electron::api::menu_roles {

enum class Action {
  kNone,
  kAbout,
  kQuit,
  kClose,
  kMinimize,
  kToggleFullScreen,
  kUndo,
  kRedo,
  kCut,
  kCopy,
  kPaste,
  kPasteAndMatchStyle,
  kDelete,
  kSelectAll,
  kReload,
  kForceReload,
  kToggleDevTools,
  kResetZoom,
  kZoomIn,
  kZoomOut,
  kToggleSpellChecker,
};

struct Role {
  std::u16string Label() const;
  // Whether `checked` is computed rather than stored.
  bool computes_checked() const {
    return action == Action::kToggleSpellChecker;
  }

  const char* id;  // lower-case
  // nullptr when the label depends on the app name (see Label()).
  const char* label;
  const char* accelerator;  // may be empty
  Action action = Action::kNone;
  bool register_accelerator = true;
  // macOS handles all roles natively except for these.
  bool non_native_mac = false;
};

// |id| must be lower-case.
const Role* Find(std::string_view id);

// { [role]: { label, accelerator? } }, for tests.
v8::Local<v8::Value> Defaults(v8::Isolate* isolate);

// A template for appMenu, fileMenu, editMenu, viewMenu, windowMenu,
// shareMenu; empty otherwise.
v8::Local<v8::Value> DefaultSubmenu(v8::Isolate* isolate, const Role& role);

bool IsChecked(const Role& role);
// The enabled state for roles that follow the focused window's abilities.
std::optional<bool> IsEnabled(const Role& role);

// Returns false if the role has no built-in action to run here.
bool Execute(const Role& role,
             BaseWindow* focused_window,
             WebContents* focused_web_contents);

}  // namespace electron::api::menu_roles

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_MENU_ROLES_H_
