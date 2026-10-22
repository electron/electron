// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_app_shim_host.h"
#include "base/memory/raw_ptr.h"
#include "base/no_destructor.h"
#include "base/strings/utf_string_conversions.h"
#include "shell/browser/api/electron_api_menu.h"
#include "shell/browser/browser.h"
#include "shell/browser/native_window_mac_remote.h"
#include "shell/browser/ui/electron_menu_model.h"
#include "shell/browser/window_list.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/models/image_model.h"

#import <Foundation/Foundation.h>
#import <Security/Security.h>
#include <bsm/libbsm.h>

#include <optional>
#include <string>
#include <utility>

#include "base/apple/foundation_util.h"
#include "base/apple/scoped_cftyperef.h"
#include "base/functional/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/sys_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "components/remote_cocoa/browser/application_host.h"
#include "content/public/browser/browser_accessibility_state.h"
#include "content/public/browser/scoped_accessibility_mode.h"
#include "gin/arguments.h"
#include "gin/object_template_builder.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "shell/browser/app_shim/app_shim_listener.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/wrappable_pointer_tags.h"
#include "ui/accessibility/ax_mode.h"
#include "url/gurl.h"
#include "v8/include/cppgc/allocation.h"
#include "v8/include/cppgc/persistent.h"
#include "v8/include/v8-cppgc.h"

namespace electron::api {

namespace {

// `rename` is the app's name and the shim's. The labels that the "about",
// "hide" and "quit" roles get by default name the app the menu was built in,
// and in a shim's menu bar they are about the shim.
std::vector<mojom::AppShimMenuItemPtr> DescribeMenu(
    ElectronMenuModel* model,
    const std::pair<std::u16string, std::u16string>& rename) {
  std::vector<mojom::AppShimMenuItemPtr> items;
  const size_t count = model->GetItemCount();
  items.reserve(count);
  for (size_t i = 0; i < count; ++i) {
    auto item = mojom::AppShimMenuItem::New();
    item->type = static_cast<int32_t>(model->GetTypeAt(i));
    item->command_id = model->GetCommandIdAt(i);
    item->label = model->GetLabelAt(i);
    item->secondary_label = model->GetSecondaryLabelAt(i);
    item->tool_tip = model->GetToolTipAt(i);
    item->accessibility_label = model->GetAccessibilityLabelAt(i);
    item->role = model->GetRoleAt(i);
    if (!rename.first.empty() && !rename.second.empty() &&
        (item->role == u"about" || item->role == u"hide" ||
         item->role == u"quit") &&
        item->label.ends_with(rename.first)) {
      item->label.replace(item->label.size() - rename.first.size(),
                          rename.first.size(), rename.second);
    }
    item->custom_type = model->GetCustomTypeAt(i);
    if (ui::ImageModel icon = model->GetIconAt(i); icon.IsImage())
      item->icon = icon.GetImage().AsImageSkia();
    ui::Accelerator accelerator;
    if (model->GetAcceleratorAtWithParams(i, true, &accelerator)) {
      item->has_accelerator = true;
      item->accelerator_key_code = accelerator.key_code();
      item->accelerator_modifiers = accelerator.modifiers();
      item->accelerator_shifted_char = accelerator.shifted_char.value_or(0);
    }
    item->register_accelerator = model->ShouldRegisterAcceleratorAt(i);
    item->works_when_hidden = model->WorksWhenHiddenAt(i);
    item->checked = model->IsItemCheckedAt(i);
    item->enabled = model->IsEnabledAt(i);
    item->visible = model->IsVisibleAt(i);
    item->group_id = model->GetGroupIdAt(i);
    if (ElectronMenuModel* submenu = model->GetSubmenuModelAt(i))
      item->submenu = DescribeMenu(submenu, rename);
    items.push_back(std::move(item));
  }
  return items;
}

// The running code of the process an audit token names. Unlike a pid, a token
// cannot come to mean another process.
base::apple::ScopedCFTypeRef<SecCodeRef> CodeForAuditToken(
    const audit_token_t& token) {
  NSDictionary* attributes = @{
    (__bridge NSString*)
    kSecGuestAttributeAudit : [NSData dataWithBytes:&token length:sizeof(token)]
  };
  base::apple::ScopedCFTypeRef<SecCodeRef> code;
  if (SecCodeCopyGuestWithAttributes(
          nullptr, (__bridge CFDictionaryRef)attributes, kSecCSDefaultFlags,
          code.InitializeInto()) != errSecSuccess) {
    code.reset();
  }
  return code;
}

// The bundle the code runs from, or the executable's path for bare code.
base::FilePath PathOfCode(SecCodeRef code) {
  base::apple::ScopedCFTypeRef<SecStaticCodeRef> static_code;
  base::apple::ScopedCFTypeRef<CFURLRef> url;
  if (!code ||
      SecCodeCopyStaticCode(code, kSecCSDefaultFlags,
                            static_code.InitializeInto()) != errSecSuccess ||
      SecCodeCopyPath(static_code.get(), kSecCSDefaultFlags,
                      url.InitializeInto()) != errSecSuccess) {
    return {};
  }
  return base::apple::NSURLToFilePath((__bridge NSURL*)url.get());
}

}  // namespace

// The native half of an AppShimHost: the Mojo connection to the shim process
// and the remote_cocoa application its windows are made through. It keeps the
// wrapper alive while the shim is connected, so a shim nobody holds on to in
// JavaScript still gets its events and keeps its windows.
class AppShimHost::Connection final : public NativePeer<AppShimHost>,
                                      public mojom::AppShimHost {
 public:
  Connection(api::AppShimHost* wrapper,
             std::unique_ptr<AppShimHostBootstrap> bootstrap,
             mojo::PendingReceiver<mojom::AppShimHost> host_receiver,
             mojo::PendingRemote<mojom::AppShim> app_shim)
      : NativePeer<api::AppShimHost>(wrapper),
        keep_alive_(wrapper),
        audit_token_(bootstrap->audit_token()),
        code_(CodeForAuditToken(audit_token_)),
        bundle_path_(PathOfCode(code_.get())),
        bootstrap_(std::move(bootstrap)),
        receiver_(this, std::move(host_receiver)),
        app_shim_(std::move(app_shim)) {
    StartObservingShutdown();

    if (NSBundle* bundle = [NSBundle
            bundleWithPath:base::apple::FilePathToNSString(bundle_path_)]) {
      bundle_id_ = base::SysNSStringToUTF8(bundle.bundleIdentifier);
      NSString* name =
          [bundle objectForInfoDictionaryKey:@"CFBundleDisplayName"]
              ?: [bundle objectForInfoDictionaryKey:@"CFBundleName"];
      name_ = base::SysNSStringToUTF16(name);
    }

    receiver_.set_disconnect_handler(
        base::BindOnce(&Connection::Disconnect, base::Unretained(this)));
    app_shim_.set_disconnect_handler(
        base::BindOnce(&Connection::Disconnect, base::Unretained(this)));

    mojo::PendingAssociatedReceiver<remote_cocoa::mojom::Application>
        application_receiver;
    // Activation goes to the shim's process. Not to its bundle identifier,
    // which is the shim's own claim and may be another app's.
    application_host_ = std::make_unique<remote_cocoa::ApplicationHost>(
        &application_receiver, std::string());
    application_host_->set_pid(pid());
    app_shim_->CreateRemoteCocoaApplication(std::move(application_receiver));
  }

  // The menu bar the shim shows while it is the active app. Without one it
  // shows a short menu that it builds for itself.
  void SetMenu(Menu* menu) {
    menu_ = menu;
    SendMenu();
  }
  Menu* menu() const { return menu_.Get(); }
  void SendMenu() {
    if (!connected())
      return;
    Menu* const menu = this->menu();
    app_shim_->SetApplicationMenu(
        menu ? std::make_optional(DescribeMenu(
                   menu->model(),
                   {base::UTF8ToUTF16(Browser::Get()->GetName()), name_}))
             : std::nullopt);
  }

  bool connected() const { return connected_ && !quit_requested_; }
  pid_t pid() const { return audit_token_to_pid(audit_token_); }
  const base::FilePath& bundle_path() const { return bundle_path_; }
  const std::string& bundle_id() const { return bundle_id_; }
  SecCodeRef code() const { return code_.get(); }
  remote_cocoa::ApplicationHost* application_host() const {
    return quit_requested_ ? nullptr : application_host_.get();
  }
  mojom::AppShim* app_shim() const {
    return connected_ ? app_shim_.get() : nullptr;
  }

  void DidCreateWindow() {
    hosts_window_ = true;
    ApplyAccessibilityMode();
  }

  // Ends the connection from JavaScript. The caller may be a listener of one
  // of this shim's windows (its `show`, say), and closing that window under
  // it would free what is notifying, so the windows close in a task of their
  // own. Until then the shim reads as disconnected and takes no new window.
  void Quit() {
    if (!connected_ || quit_requested_)
      return;
    quit_requested_ = true;
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(&Connection::Disconnect, weak_factory_.GetWeakPtr()));
  }

  // Ends the connection, which ends the shim process and closes its windows.
  void Disconnect() {
    if (!TearDownConnection())
      return;
    if (auto shim_host = wrapper())
      shim_host->OnDisconnected();
    // May free the wrapper, and this with it, at the next garbage collection.
    keep_alive_.Clear();
  }

 private:
  ~Connection() override = default;

  void ApplyAccessibilityMode() {
    if (!hosts_window_ || !wanted_accessibility_mode_ || !connected_)
      return;
    accessibility_mode_ =
        content::BrowserAccessibilityState::GetInstance()
            ->CreateScopedModeForProcess(
                (*wanted_accessibility_mode_ ==
                         mojom::AppShimAccessibilityMode::kComplete
                     ? ui::kAXModeComplete
                     : ui::kAXModeBasic) |
                ui::AXMode::kFromPlatform);
  }

  bool TearDownConnection() {
    if (!connected_)
      return false;
    connected_ = false;
    menu_.Clear();
    weak_factory_.InvalidateWeakPtrs();
    // Closes every window the shim hosts, synchronously: their closed events
    // run JavaScript before this returns.
    application_host_.reset();
    accessibility_mode_.reset();
    receiver_.reset();
    app_shim_.reset();
    bootstrap_.reset();
    return true;
  }

  // NativePeerBase:
  void TearDownNative() override {
    TearDownConnection();
    keep_alive_.Clear();
  }

  // mojom::AppShimHost:
  void ReopenApp(bool has_visible_windows) override {
    if (auto shim_host = wrapper())
      shim_host->OnReopen(has_visible_windows);
  }
  void FilesOpened(const std::vector<base::FilePath>& files) override {
    if (auto shim_host = wrapper())
      shim_host->OnFilesOpened(files);
  }
  void UrlsOpened(const std::vector<GURL>& urls) override {
    if (auto shim_host = wrapper())
      shim_host->OnUrlsOpened(urls);
  }
  void EnableAccessibilitySupport(
      mojom::AppShimAccessibilityMode mode) override {
    // Any process of the same user can connect and ask for this, and it
    // costs every page in the app. It waits until the app has given the
    // shim a window, which is the app's way of saying it accepts the shim.
    if (!wanted_accessibility_mode_ ||
        mode == mojom::AppShimAccessibilityMode::kComplete) {
      wanted_accessibility_mode_ = mode;
    }
    ApplyAccessibilityMode();
  }
  void MenuItemActivated(const std::vector<uint32_t>& path,
                         int32_t command_id,
                         int32_t event_flags) override {
    // The shim's menu may be older than this one: the item is only chosen if
    // what is at `path` now is still the item the shim meant.
    Menu* const menu = this->menu();
    ElectronMenuModel* model = menu ? menu->model() : nullptr;
    for (size_t i = 0; model && i + 1 < path.size(); ++i) {
      model = path[i] < model->GetItemCount()
                  ? model->GetSubmenuModelAt(path[i])
                  : nullptr;
    }
    if (model && !path.empty() && path.back() < model->GetItemCount() &&
        model->GetCommandIdAt(path.back()) == command_id) {
      model->ActivatedAt(path.back(), event_flags);
    }
    // A click may have changed a check mark, or anything else.
    SendMenu();
  }
  void MenuWillShow() override {
    if (Menu* const menu = this->menu())
      menu->model()->MenuWillShow();
    SendMenu();
  }
  void MenuWillClose() override {
    if (Menu* const menu = this->menu())
      menu->model()->MenuWillClose();
  }
  void DidBecomeActive() override { SendMenu(); }
  void NewWindowForTab(uint64_t window_id) override {
    for (NativeWindow* window : WindowList::GetWindows()) {
      if (!window->IsInAppShim())
        continue;
      auto* remote = static_cast<NativeWindowMacRemote*>(window);
      if (remote->GetAppShim() == app_shim() &&
          remote->GetRemoteWindowId() == window_id) {
        remote->NotifyNewWindowForTab();
        return;
      }
    }
  }
  void QuitRequested() override {
    bool quit = true;
    if (auto shim_host = wrapper())
      quit = shim_host->OnQuitRequested();
    if (!connected_)
      return;
    if (quit)
      app_shim_->Quit();
    else
      app_shim_->CancelQuit();
  }

  cppgc::Persistent<api::AppShimHost> keep_alive_;
  const audit_token_t audit_token_;
  const base::apple::ScopedCFTypeRef<SecCodeRef> code_;
  const base::FilePath bundle_path_;
  std::string bundle_id_;
  // What the shim calls itself, for its menu bar; its own claim.
  std::u16string name_;
  bool connected_ = true;
  bool quit_requested_ = false;

  // Holds the isolated connection the pipes below run over.
  std::unique_ptr<AppShimHostBootstrap> bootstrap_;
  mojo::Receiver<mojom::AppShimHost> receiver_;
  mojo::Remote<mojom::AppShim> app_shim_;
  std::unique_ptr<remote_cocoa::ApplicationHost> application_host_;
  // Keeps accessibility on in this process while an assistive technology is
  // looking at the shim.
  // The shim's menu bar, if it has been given one.
  cppgc::Persistent<Menu> menu_;

  bool hosts_window_ = false;
  std::optional<mojom::AppShimAccessibilityMode> wanted_accessibility_mode_;
  std::unique_ptr<content::ScopedAccessibilityMode> accessibility_mode_;

  base::WeakPtrFactory<Connection> weak_factory_{this};
};

gin::WrapperInfo AppShimHost::kWrapperInfo =
    electron::MakeWrapperInfo(electron::kElectronAppShimHost);

// static
AppShimHost* AppShimHost::Create(
    v8::Isolate* isolate,
    std::unique_ptr<AppShimHostBootstrap> bootstrap,
    mojo::PendingReceiver<mojom::AppShimHost> host_receiver,
    mojo::PendingRemote<mojom::AppShim> app_shim) {
  return cppgc::MakeGarbageCollected<AppShimHost>(
      isolate->GetCppHeap()->GetAllocationHandle(), std::move(bootstrap),
      std::move(host_receiver), std::move(app_shim));
}

AppShimHost::AppShimHost(
    std::unique_ptr<AppShimHostBootstrap> bootstrap,
    mojo::PendingReceiver<mojom::AppShimHost> host_receiver,
    mojo::PendingRemote<mojom::AppShim> app_shim)
    : connection_(
          NativePeer<AppShimHost>::Create<Connection>(this,
                                                      std::move(bootstrap),
                                                      std::move(host_receiver),
                                                      std::move(app_shim))) {}

AppShimHost::~AppShimHost() = default;

// static
base::FilePath AppShimHost::BundlePathForAuditToken(
    const audit_token_t& token) {
  return PathOfCode(CodeForAuditToken(token).get());
}

remote_cocoa::ApplicationHost* AppShimHost::GetRemoteCocoaApplicationHost()
    const {
  return connection_->application_host();
}

mojom::AppShim* AppShimHost::GetAppShim() const {
  return connection_->connected() ? connection_->app_shim() : nullptr;
}

void AppShimHost::DidCreateWindow() {
  connection_->DidCreateWindow();
}

void AppShimHost::OnReopen(bool has_visible_windows) {
  Emit("activate", has_visible_windows);
}

void AppShimHost::OnFilesOpened(const std::vector<base::FilePath>& files) {
  for (const base::FilePath& file : files)
    Emit("open-file", file);
}

void AppShimHost::OnUrlsOpened(const std::vector<GURL>& urls) {
  for (const GURL& url : urls)
    Emit("open-url", url.spec());
}

bool AppShimHost::OnQuitRequested() {
  return !Emit("quit-requested");
}

void AppShimHost::OnDisconnected() {
  Emit("disconnected");
}

int AppShimHost::GetPid() const {
  return connection_->pid();
}

base::FilePath AppShimHost::GetBundlePath() const {
  return connection_->bundle_path();
}

std::string AppShimHost::GetBundleId() const {
  return connection_->bundle_id();
}

bool AppShimHost::IsConnected() const {
  return connection_->connected();
}

bool AppShimHost::SatisfiesCodeRequirement(gin_helper::ErrorThrower thrower,
                                           const std::string& requirement) {
  base::apple::ScopedCFTypeRef<SecRequirementRef> requirement_ref;
  if (SecRequirementCreateWithString(
          base::SysUTF8ToCFStringRef(requirement).get(), kSecCSDefaultFlags,
          requirement_ref.InitializeInto()) != errSecSuccess) {
    thrower.ThrowError("Invalid code requirement");
    return false;
  }
  return connection_->code() &&
         SecCodeCheckValidity(connection_->code(), kSecCSDefaultFlags,
                              requirement_ref.get()) == errSecSuccess;
}

void AppShimHost::SetBadge(const std::string& label) {
  if (mojom::AppShim* app_shim = connection_->app_shim())
    app_shim->SetBadgeLabel(label);
}

void AppShimHost::Bounce(gin::Arguments* args) {
  std::string type = "informational";
  if (args->Length() > 0 && !args->GetNext(&type)) {
    args->ThrowTypeError("Invalid bounce type");
    return;
  }
  mojom::AppShimAttentionType attention_type;
  if (type == "critical") {
    attention_type = mojom::AppShimAttentionType::kCritical;
  } else if (type == "informational") {
    attention_type = mojom::AppShimAttentionType::kInformational;
  } else {
    args->ThrowTypeError("Invalid bounce type");
    return;
  }
  if (mojom::AppShim* app_shim = connection_->app_shim())
    app_shim->SetUserAttention(attention_type);
}

void AppShimHost::CancelBounce() {
  if (mojom::AppShim* app_shim = connection_->app_shim())
    app_shim->SetUserAttention(mojom::AppShimAttentionType::kCancel);
}

void AppShimHost::Quit() {
  connection_->Quit();
}

void AppShimHost::SetApplicationMenu(gin::Arguments* args) {
  v8::Local<v8::Value> value;
  Menu* menu = nullptr;
  if (!args->GetNext(&value) ||
      (!value->IsNull() &&
       (!gin::ConvertFromV8(args->isolate(), value, &menu) || !menu))) {
    args->ThrowTypeError("Must pass a Menu or null");
    return;
  }
  connection_->SetMenu(menu);
}

gin::ObjectTemplateBuilder AppShimHost::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin_helper::EventEmitterMixin<AppShimHost>::GetObjectTemplateBuilder(
             isolate)
      .SetProperty("pid", &AppShimHost::GetPid)
      .SetProperty("bundlePath", &AppShimHost::GetBundlePath)
      .SetProperty("bundleId", &AppShimHost::GetBundleId)
      .SetMethod("isConnected", &AppShimHost::IsConnected)
      .SetMethod("satisfiesCodeRequirement",
                 &AppShimHost::SatisfiesCodeRequirement)
      .SetMethod("setBadge", &AppShimHost::SetBadge)
      .SetMethod("bounce", &AppShimHost::Bounce)
      .SetMethod("cancelBounce", &AppShimHost::CancelBounce)
      .SetMethod("setApplicationMenu", &AppShimHost::SetApplicationMenu)
      .SetMethod("quit", &AppShimHost::Quit);
}

const gin::WrapperInfo* AppShimHost::wrapper_info() const {
  return &kWrapperInfo;
}

const char* AppShimHost::GetHumanReadableName() const {
  return "Electron / AppShimHost";
}

}  // namespace electron::api
