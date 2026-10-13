// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_app_shim_host.h"

#import <Foundation/Foundation.h>
#import <Security/Security.h>
#include <bsm/libbsm.h>

#include <utility>

#include "base/apple/foundation_util.h"
#include "base/apple/scoped_cftyperef.h"
#include "base/functional/bind.h"
#include "base/strings/sys_string_conversions.h"
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
    }

    receiver_.set_disconnect_handler(
        base::BindOnce(&Connection::Disconnect, base::Unretained(this)));
    app_shim_.set_disconnect_handler(
        base::BindOnce(&Connection::Disconnect, base::Unretained(this)));

    mojo::PendingAssociatedReceiver<remote_cocoa::mojom::Application>
        application_receiver;
    application_host_ = std::make_unique<remote_cocoa::ApplicationHost>(
        &application_receiver, bundle_id_);
    app_shim_->CreateRemoteCocoaApplication(std::move(application_receiver));
  }

  bool connected() const { return connected_; }
  pid_t pid() const { return audit_token_to_pid(audit_token_); }
  const base::FilePath& bundle_path() const { return bundle_path_; }
  const std::string& bundle_id() const { return bundle_id_; }
  SecCodeRef code() const { return code_.get(); }
  remote_cocoa::ApplicationHost* application_host() const {
    return application_host_.get();
  }
  mojom::AppShim* app_shim() const {
    return connected_ ? app_shim_.get() : nullptr;
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

  bool TearDownConnection() {
    if (!connected_)
      return false;
    connected_ = false;
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
    accessibility_mode_ =
        content::BrowserAccessibilityState::GetInstance()
            ->CreateScopedModeForProcess(
                (mode == mojom::AppShimAccessibilityMode::kComplete
                     ? ui::kAXModeComplete
                     : ui::kAXModeBasic) |
                ui::AXMode::kFromPlatform);
  }
  void QuitRequested() override {
    bool quit = true;
    if (auto shim_host = wrapper())
      quit = shim_host->OnQuitRequested();
    if (quit && connected_)
      app_shim_->Quit();
  }

  cppgc::Persistent<api::AppShimHost> keep_alive_;
  const audit_token_t audit_token_;
  const base::apple::ScopedCFTypeRef<SecCodeRef> code_;
  const base::FilePath bundle_path_;
  std::string bundle_id_;
  bool connected_ = true;

  // Holds the isolated connection the pipes below run over.
  std::unique_ptr<AppShimHostBootstrap> bootstrap_;
  mojo::Receiver<mojom::AppShimHost> receiver_;
  mojo::Remote<mojom::AppShim> app_shim_;
  std::unique_ptr<remote_cocoa::ApplicationHost> application_host_;
  // Keeps accessibility on in this process while an assistive technology is
  // looking at the shim.
  std::unique_ptr<content::ScopedAccessibilityMode> accessibility_mode_;
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

remote_cocoa::ApplicationHost* AppShimHost::GetRemoteCocoaApplicationHost()
    const {
  return connection_->application_host();
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
  args->GetNext(&type);
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
  connection_->Disconnect();
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
      .SetMethod("quit", &AppShimHost::Quit);
}

const gin::WrapperInfo* AppShimHost::wrapper_info() const {
  return &kWrapperInfo;
}

const char* AppShimHost::GetHumanReadableName() const {
  return "Electron / AppShimHost";
}

}  // namespace electron::api
