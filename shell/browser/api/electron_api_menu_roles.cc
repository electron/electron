// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_menu_roles.h"

#include <array>
#include <string>
#include <string_view>

#include "base/memory/stack_allocated.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "electron/buildflags/buildflags.h"
#include "gin/converter.h"
#include "gin/dictionary.h"
#include "shell/browser/api/electron_api_base_window.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/browser.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/native_window.h"
#include "v8/include/v8.h"

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
#include "components/spellcheck/browser/pref_names.h"
#endif

namespace electron::api::menu_roles {

namespace {

#if BUILDFLAG(IS_MAC)
constexpr bool kIsMac = true;
#else
constexpr bool kIsMac = false;
#endif
#if BUILDFLAG(IS_WIN)
constexpr bool kIsWin = true;
#else
constexpr bool kIsWin = false;
#endif
#if BUILDFLAG(IS_LINUX)
constexpr bool kIsLinux = true;
#else
constexpr bool kIsLinux = false;
#endif

constexpr Role kRoles[] = {
    {"about", nullptr, "",
     (kIsWin || kIsLinux) ? Action::kAbout : Action::kNone},
    {"close", kIsMac ? "Close Window" : "Close", "CommandOrControl+W",
     Action::kClose},
    {"copy", "Copy", "CommandOrControl+C", Action::kCopy, false},
    {"cut", "Cut", "CommandOrControl+X", Action::kCut, false},
    {"delete", "Delete", "", Action::kDelete},
    {"forcereload", "Force Reload", "Shift+CmdOrCtrl+R", Action::kForceReload,
     true, true},
    {"front", "Bring All to Front", ""},
    {"help", "Help", ""},
    {"hide", nullptr, "Command+H"},
    {"hideothers", "Hide Others", "Command+Alt+H"},
    {"minimize", "Minimize", "CommandOrControl+M", Action::kMinimize},
    {"paste", "Paste", "CommandOrControl+V", Action::kPaste, false},
    {"pasteandmatchstyle", "Paste and Match Style",
     kIsMac ? "Cmd+Option+Shift+V" : "Shift+CommandOrControl+V",
     Action::kPasteAndMatchStyle, false},
    {"quit", nullptr, kIsWin ? "" : "CommandOrControl+Q", Action::kQuit},
    {"redo", "Redo", kIsWin ? "Control+Y" : "Shift+CommandOrControl+Z",
     Action::kRedo},
    {"reload", "Reload", "CmdOrCtrl+R", Action::kReload, true, true},
    {"resetzoom", "Actual Size", "CommandOrControl+0", Action::kResetZoom, true,
     true},
    {"selectall", "Select All", "CommandOrControl+A", Action::kSelectAll},
    {"services", "Services", ""},
    {"recentdocuments", "Open Recent", ""},
    {"clearrecentdocuments", "Clear Menu", ""},
    {"showsubstitutions", "Show Substitutions", ""},
    {"togglesmartquotes", "Smart Quotes", ""},
    {"togglesmartdashes", "Smart Dashes", ""},
    {"toggletextreplacement", "Text Replacement", ""},
    {"startspeaking", "Start Speaking", ""},
    {"stopspeaking", "Stop Speaking", ""},
    {"toggledevtools", "Toggle Developer Tools",
     kIsMac ? "Alt+Command+I" : "Ctrl+Shift+I", Action::kToggleDevTools, true,
     true},
    {"togglefullscreen", "Toggle Full Screen",
     kIsMac ? "Control+Command+F" : "F11", Action::kToggleFullScreen},
    {"undo", "Undo", "CommandOrControl+Z", Action::kUndo},
    {"unhide", "Show All", ""},
    {"window", "Window", ""},
    {"zoom", "Zoom", ""},
    {"zoomin", "Zoom In", "CommandOrControl+Plus", Action::kZoomIn, true, true},
    {"zoomout", "Zoom Out", "CommandOrControl+-", Action::kZoomOut, true, true},
    {"togglespellchecker", "Check Spelling While Typing", "",
     Action::kToggleSpellChecker, true, true},
    {"appmenu", nullptr, ""},
    {"filemenu", "File", ""},
    {"editmenu", "Edit", ""},
    {"viewmenu", "View", ""},
    {"windowmenu", "Window", ""},
    {"sharemenu", "Share", ""},
};

// A submenu template as Menu.buildFromTemplate() takes it.
class TemplateBuilder {
  STACK_ALLOCATED();

 public:
  explicit TemplateBuilder(v8::Isolate* isolate)
      : isolate_(isolate), items_(isolate) {}

  TemplateBuilder& Role(std::string_view id) {
    gin::Dictionary item = gin::Dictionary::CreateEmpty(isolate_);
    item.Set("role", id);
    items_.push_back(gin::ConvertToV8(isolate_, item));
    return *this;
  }
  TemplateBuilder& Separator() {
    gin::Dictionary item = gin::Dictionary::CreateEmpty(isolate_);
    item.Set("type", std::string_view("separator"));
    items_.push_back(gin::ConvertToV8(isolate_, item));
    return *this;
  }
  TemplateBuilder& Submenu(std::string_view label,
                           v8::Local<v8::Value> submenu) {
    gin::Dictionary item = gin::Dictionary::CreateEmpty(isolate_);
    item.Set("label", label);
    item.Set("submenu", submenu);
    items_.push_back(gin::ConvertToV8(isolate_, item));
    return *this;
  }
  v8::Local<v8::Value> Build() {
    return v8::Array::New(isolate_, items_.data(), items_.size());
  }

 private:
  v8::Isolate* isolate_;
  v8::LocalVector<v8::Value> items_;
};

// The webContents whose open DevTools |web_contents| is, else |web_contents|.
WebContents* DevToolsOwnerOr(WebContents* web_contents) {
  for (WebContents* candidate : WebContents::GetWebContentsList()) {
    if (candidate->GetOpenDevToolsWebContents() ==
        web_contents->web_contents()) {
      return candidate;
    }
  }
  return web_contents;
}

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
// The focused webContents' session's prefs, else the default session's.
PrefService* SpellcheckPrefs(WebContents* web_contents) {
  content::BrowserContext* context =
      web_contents && web_contents->web_contents()
          ? web_contents->web_contents()->GetBrowserContext()
          : ElectronBrowserContext::GetDefaultBrowserContext();
  return static_cast<ElectronBrowserContext*>(context)->prefs();
}
#endif

NativeWindow* FocusedNativeWindow() {
  BaseWindow* window = BaseWindow::GetFocusedWindow();
  return window ? window->window() : nullptr;
}

}  // namespace

std::u16string Role::Label() const {
  if (label)
    return base::UTF8ToUTF16(label);
  const std::string name = Browser::Get()->GetName();
  const std::string_view role(id);
  if (role == "about")
    return kIsLinux ? u"About" : base::UTF8ToUTF16("About " + name);
  if (role == "hide")
    return base::UTF8ToUTF16("Hide " + name);
  if (role == "quit") {
    if (kIsMac)
      return base::UTF8ToUTF16("Quit " + name);
    return kIsWin ? u"Exit" : u"Quit";
  }
  if (role == "appmenu")
    return base::UTF8ToUTF16(name);
  return {};
}

const Role* Find(std::string_view id) {
  for (const Role& role : kRoles) {
    if (role.id == id)
      return &role;
  }
  return nullptr;
}

v8::Local<v8::Value> Defaults(v8::Isolate* isolate) {
  gin::Dictionary all = gin::Dictionary::CreateEmpty(isolate);
  for (const Role& role : kRoles) {
    gin::Dictionary entry = gin::Dictionary::CreateEmpty(isolate);
    entry.Set("label", role.Label());
    if (*role.accelerator)
      entry.Set("accelerator", std::string_view(role.accelerator));
    all.Set(std::string(role.id), entry);
  }
  return gin::ConvertToV8(isolate, all);
}

v8::Local<v8::Value> DefaultSubmenu(v8::Isolate* isolate, const Role& role) {
  const std::string_view id(role.id);
  if (id == "appmenu") {
    return TemplateBuilder(isolate)
        .Role("about")
        .Separator()
        .Role("services")
        .Separator()
        .Role("hide")
        .Role("hideothers")
        .Role("unhide")
        .Separator()
        .Role("quit")
        .Build();
  }
  if (id == "filemenu")
    return TemplateBuilder(isolate).Role(kIsMac ? "close" : "quit").Build();
  if (id == "editmenu") {
    TemplateBuilder edit(isolate);
    edit.Role("undo").Role("redo").Separator().Role("cut").Role("copy").Role(
        "paste");
    if (kIsMac) {
      edit.Role("pasteandmatchstyle").Role("delete").Role("selectall");
      edit.Separator();
      edit.Submenu("Substitutions", TemplateBuilder(isolate)
                                        .Role("showsubstitutions")
                                        .Separator()
                                        .Role("togglesmartquotes")
                                        .Role("togglesmartdashes")
                                        .Role("toggletextreplacement")
                                        .Build());
      edit.Submenu("Speech", TemplateBuilder(isolate)
                                 .Role("startspeaking")
                                 .Role("stopspeaking")
                                 .Build());
    } else {
      edit.Role("delete").Separator().Role("selectall");
    }
    return edit.Build();
  }
  if (id == "viewmenu") {
    return TemplateBuilder(isolate)
        .Role("reload")
        .Role("forcereload")
        .Role("toggledevtools")
        .Separator()
        .Role("resetzoom")
        .Role("zoomin")
        .Role("zoomout")
        .Separator()
        .Role("togglefullscreen")
        .Build();
  }
  if (id == "windowmenu") {
    TemplateBuilder window(isolate);
    window.Role("minimize").Role("zoom");
    if (kIsMac)
      window.Separator().Role("front");
    else
      window.Role("close");
    return window.Build();
  }
  if (id == "sharemenu")
    return v8::Array::New(isolate);
  return {};
}

bool IsChecked(const Role& role) {
#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  if (role.action == Action::kToggleSpellChecker) {
    return SpellcheckPrefs(WebContents::GetFocusedWebContents())
        ->GetBoolean(spellcheck::prefs::kSpellCheckEnable);
  }
#endif
  return false;
}

std::optional<bool> IsEnabled(const Role& role) {
  switch (role.action) {
    case Action::kMinimize:
      if (NativeWindow* window = FocusedNativeWindow())
        return window->IsMinimizable();
      break;
    case Action::kToggleFullScreen:
      if (NativeWindow* window = FocusedNativeWindow())
        return window->IsFullScreenable();
      break;
    case Action::kClose:
      if (NativeWindow* window = FocusedNativeWindow())
        return window->IsClosable();
      break;
    default:
      break;
  }
  return std::nullopt;
}

bool Execute(const Role& role,
             BaseWindow* focused_window,
             WebContents* focused_web_contents) {
  if (role.action == Action::kNone || (kIsMac && !role.non_native_mac))
    return false;

  switch (role.action) {
    case Action::kAbout:
      Browser::Get()->ShowAboutPanel();
      return true;
    case Action::kQuit:
      Browser::Get()->Quit();
      return true;
    default:
      break;
  }

  if (role.action == Action::kClose || role.action == Action::kMinimize ||
      role.action == Action::kToggleFullScreen) {
    if (!focused_window || !focused_window->window())
      return false;
    NativeWindow* window = focused_window->window();
    switch (role.action) {
      case Action::kClose:
        window->Close();
        break;
      case Action::kMinimize:
        if (window->IsMinimizable())
          window->Minimize();
        break;
      case Action::kToggleFullScreen:
        window->SetFullScreen(!window->IsFullscreen());
        break;
      default:
        break;
    }
    return true;
  }

  if (!focused_web_contents || !focused_web_contents->web_contents())
    return false;
  WebContents* wc = focused_web_contents;
  switch (role.action) {
    case Action::kUndo:
      wc->Undo();
      break;
    case Action::kRedo:
      wc->Redo();
      break;
    case Action::kCut:
      wc->Cut();
      break;
    case Action::kCopy:
      wc->Copy();
      break;
    case Action::kPaste:
      wc->Paste();
      break;
    case Action::kPasteAndMatchStyle:
      wc->PasteAndMatchStyle();
      break;
    case Action::kDelete:
      wc->Delete();
      break;
    case Action::kSelectAll:
      wc->SelectAll();
      break;
    case Action::kReload:
      DevToolsOwnerOr(wc)->Reload();
      break;
    case Action::kForceReload:
      DevToolsOwnerOr(wc)->ReloadIgnoringCache();
      break;
    case Action::kToggleDevTools:
      DevToolsOwnerOr(wc)->ToggleDevTools();
      break;
    case Action::kResetZoom:
      wc->SetZoomLevel(0);
      break;
    case Action::kZoomIn:
      wc->SetZoomLevel(wc->GetZoomLevel() + 0.5);
      break;
    case Action::kZoomOut:
      wc->SetZoomLevel(wc->GetZoomLevel() - 0.5);
      break;
    case Action::kToggleSpellChecker: {
#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
      PrefService* prefs = SpellcheckPrefs(wc);
      prefs->SetBoolean(
          spellcheck::prefs::kSpellCheckEnable,
          !prefs->GetBoolean(spellcheck::prefs::kSpellCheckEnable));
#endif
      break;
    }
    default:
      return false;
  }
  return true;
}

}  // namespace electron::api::menu_roles
