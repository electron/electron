// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/app_shim/app_shim_controller.h"

#import <Cocoa/Cocoa.h>
#include <mach/mach.h>

#include <utility>

#include "base/apple/bundle_locations.h"
#include "base/apple/foundation_util.h"
#include "base/apple/mach_logging.h"
#include "base/apple/scoped_mach_port.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/mac/scoped_mach_msg_destroy.h"
#include "base/memory/raw_ptr.h"
#include "base/strings/strcat.h"
#include "base/strings/sys_string_conversions.h"
#include "base/task/single_thread_task_runner.h"
#include "components/remote_cocoa/app_shim/application_bridge.h"
#import "components/remote_cocoa/app_shim/native_widget_mac_nswindow.h"
#include "components/remote_cocoa/app_shim/native_widget_ns_window_bridge.h"
#include "content/public/browser/remote_cocoa.h"
#include "mojo/public/cpp/platform/named_platform_channel.h"
#include "mojo/public/cpp/platform/platform_channel.h"
#import "shell/app_shim/app_shim_window.h"
#import "shell/browser/ui/cocoa/electron_menu_controller.h"
#include "shell/browser/ui/file_dialog.h"
#include "shell/browser/ui/message_box.h"
#include "shell/common/app_shim_constants.h"
#include "ui/accelerated_widget_mac/window_resize_helper_mac.h"
#include "ui/display/screen.h"
#include "ui/gfx/native_ui_types.h"
#include "url/gurl.h"

namespace {

// How long a shim waits for the host app to start listening.
constexpr base::TimeDelta kPollTimeout = base::Seconds(60);
constexpr base::TimeDelta kPollPeriod = base::Milliseconds(100);
// How long the host has to answer a quit request before the shim quits anyway,
// so that a host that is busy or hung cannot hold up a logout.
constexpr base::TimeDelta kQuitAnswerTimeout = base::Seconds(5);
// How long the host has to take note of a build mismatch before the shim
// quits without knowing that it has.
constexpr base::TimeDelta kBuildMismatchAcknowledgementTimeout =
    base::Seconds(2);

NSString* InfoPlistString(const char* key) {
  id value = [NSBundle.mainBundle
      objectForInfoDictionaryKey:base::SysUTF8ToNSString(key)];
  return base::apple::ObjCCast<NSString>(value);
}

NSString* AppName() {
  return base::apple::ObjCCast<NSString>([NSBundle.mainBundle
             objectForInfoDictionaryKey:@"CFBundleDisplayName"])
             ?: base::apple::ObjCCast<NSString>([NSBundle.mainBundle
                    objectForInfoDictionaryKey:@"CFBundleName"])
                    ?: NSProcessInfo.processInfo.processName;
}

}  // namespace

@interface ElectronAppShimDelegate : NSObject <NSApplicationDelegate>
- (instancetype)initWithController:(electron::AppShimController*)controller;
- (void)controllerDestroyed;
@end

@implementation ElectronAppShimDelegate {
  raw_ptr<electron::AppShimController> _controller;
}

- (instancetype)initWithController:(electron::AppShimController*)controller {
  if ((self = [super init])) {
    _controller = controller;
  }
  return self;
}

- (void)controllerDestroyed {
  _controller = nullptr;
}

// The "+" of a window's tab bar. AppKit shows it when something in the
// responder chain answers to this, and the window's own delegate is
// remote_cocoa's.
- (void)newWindowForTab:(id)sender {
  NSWindow* window = NSApp.keyWindow ?: NSApp.mainWindow;
  if (!_controller || ![window isKindOfClass:[NativeWidgetMacNSWindow class]]) {
    return;
  }
  _controller->OnNewWindowForTab(
      static_cast<NativeWidgetMacNSWindow*>(window).bridgedNativeWidgetId);
}

// The Quick Look panel asks the responder chain who fills it, which ends
// here. It is the key window's, if the host asked for a preview there.
- (ElectronAppShimWindow*)windowForPreviewPanel {
  ElectronAppShimWindow* window =
      [ElectronAppShimWindow existingForWindow:NSApp.mainWindow];
  return [window hasPreviewItem] ? window : nil;
}

- (BOOL)acceptsPreviewPanelControl:(QLPreviewPanel*)panel {
  return [self windowForPreviewPanel] != nil;
}

- (void)beginPreviewPanelControl:(QLPreviewPanel*)panel {
  ElectronAppShimWindow* window = [self windowForPreviewPanel];
  panel.dataSource = window;
  panel.delegate = window;
}

- (void)endPreviewPanelControl:(QLPreviewPanel*)panel {
  panel.dataSource = nil;
  panel.delegate = nil;
}

- (void)applicationDidBecomeActive:(NSNotification*)notification {
  if (_controller)
    _controller->OnDidBecomeActive();
}

- (BOOL)applicationShouldHandleReopen:(NSApplication*)sender
                    hasVisibleWindows:(BOOL)flag {
  if (_controller)
    _controller->OnReopen(flag);
  return YES;
}

- (void)application:(NSApplication*)sender
          openFiles:(NSArray<NSString*>*)filenames {
  std::vector<base::FilePath> files;
  for (NSString* filename in filenames)
    files.push_back(base::apple::NSStringToFilePath(filename));
  if (_controller)
    _controller->OnOpenFiles(std::move(files));
  [sender replyToOpenOrPrint:NSApplicationDelegateReplySuccess];
}

- (void)application:(NSApplication*)application
           openURLs:(NSArray<NSURL*>*)urls {
  std::vector<base::FilePath> files;
  std::vector<GURL> other;
  for (NSURL* url in urls) {
    if (url.fileURL) {
      files.push_back(base::apple::NSURLToFilePath(url));
    } else if (GURL gurl(base::SysNSStringToUTF8(url.absoluteString));
               gurl.is_valid()) {
      other.push_back(std::move(gurl));
    }
  }
  if (!_controller)
    return;
  if (!files.empty())
    _controller->OnOpenFiles(std::move(files));
  if (!other.empty())
    _controller->OnOpenUrls(std::move(other));
}

- (BOOL)applicationSupportsSecureRestorableState:(NSApplication*)app {
  return YES;
}

@end

namespace electron {

namespace {

AppShimController* g_controller = nullptr;

}  // namespace

// static
AppShimController* AppShimController::Get() {
  return g_controller;
}

AppShimController::AppShimController(const base::FilePath& host_bundle_path)
    : host_bundle_path_(host_bundle_path),
      host_receiver_(host_.BindNewPipeAndPassReceiver()) {
  g_controller = this;
  screen_ = std::make_unique<display::ScopedNativeScreen>();
  delegate_ = [[ElectronAppShimDelegate alloc] initWithController:this];
  NSApp.delegate = delegate_;
  [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
  // The host decides which shims run; macOS must not bring this one back at
  // login by itself.
  [NSApp disableRelaunchOnLogin];
  SetUpMenu();

  poll_deadline_ = base::TimeTicks::Now() + kPollTimeout;
  PollForHost();
}

AppShimController::~AppShimController() {
  g_controller = nullptr;
  [delegate_ controllerDestroyed];
  NSApp.delegate = nil;
}

std::string AppShimController::ServerName() const {
  // Mirrors ElectronMainDelegate::SetUpBundleOverrides in the host.
  NSBundle* host = base::apple::OuterBundle();
  std::string base_bundle_id = base::SysNSStringToUTF8(host.bundleIdentifier);
  if (NSString* team_id = base::apple::ObjCCast<NSString>(
          [host objectForInfoDictionaryKey:@"ElectronTeamID"])) {
    base_bundle_id =
        base::StrCat({base::SysNSStringToUTF8(team_id), ".", base_bundle_id});
  }
  NSString* name = InfoPlistString(app_shim::kInfoPlistServerNameKey);
  return base::StrCat({base_bundle_id, ".",
                       name.length ? base::SysNSStringToUTF8(name)
                                   : app_shim::kDefaultServerName});
}

bool AppShimController::IsHostRunning() const {
  NSString* bundle_id = base::apple::OuterBundle().bundleIdentifier;
  if (!bundle_id)
    return false;
  NSString* want = base::apple::FilePathToNSString(host_bundle_path_)
                       .stringByResolvingSymlinksInPath;
  for (NSRunningApplication* app in [NSRunningApplication
           runningApplicationsWithBundleIdentifier:bundle_id]) {
    if (!app.terminated && [app.bundleURL.path.stringByResolvingSymlinksInPath
                               isEqualToString:want]) {
      return true;
    }
  }
  return false;
}

void AppShimController::LaunchHost() {
  launched_host_ = true;
  NSWorkspaceOpenConfiguration* configuration =
      [NSWorkspaceOpenConfiguration configuration];
  configuration.activates = NO;
  configuration.addsToRecentItems = NO;
  NSMutableArray<NSString*>* arguments = [NSMutableArray array];
  for (id argument in base::apple::ObjCCast<NSArray>([NSBundle.mainBundle
           objectForInfoDictionaryKey:
               base::SysUTF8ToNSString(
                   app_shim::kInfoPlistHostArgumentsKey)])) {
    if (NSString* string = base::apple::ObjCCast<NSString>(argument))
      [arguments addObject:string];
  }
  configuration.arguments = arguments;
  [NSWorkspace.sharedWorkspace
      openApplicationAtURL:base::apple::FilePathToNSURL(host_bundle_path_)
             configuration:configuration
         completionHandler:^(NSRunningApplication* app, NSError* error) {
           if (error) {
             LOG(ERROR) << "Could not start the host app: "
                        << base::SysNSStringToUTF8(error.localizedDescription);
           }
         }];
}

void AppShimController::PollForHost() {
  const std::string server_name = ServerName();
  if (mojo::PlatformChannelEndpoint server =
          mojo::NamedPlatformChannel::ConnectToServer(server_name);
      server.is_valid()) {
    // The host is listening. It is only worth connecting to when it runs the
    // build of Electron that this process loaded.
    if (!mojo::NamedPlatformChannel::ConnectToServer(
             app_shim::BuildMarkerServerName(server_name))
             .is_valid()) {
      OnBuildMismatch(std::move(server));
      return;
    }
    if (mojo::PlatformChannelEndpoint endpoint =
            ConnectToHost(std::move(server));
        endpoint.is_valid()) {
      OnConnected(std::move(endpoint));
      return;
    }
  }

  if (!launched_host_ && !IsHostRunning())
    LaunchHost();

  if (base::TimeTicks::Now() > poll_deadline_) {
    LOG(ERROR) << "The host app did not start listening for app shims.";
    Terminate();
    return;
  }
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&AppShimController::PollForHost,
                     weak_factory_.GetWeakPtr()),
      kPollPeriod);
}

// static
mojo::PlatformChannelEndpoint AppShimController::ConnectToHost(
    mojo::PlatformChannelEndpoint server_endpoint) {
  // One bootstrap server takes connections from every shim, so each shim makes
  // a channel of its own and sends the host one end of it in a raw Mach
  // message. Both sides then make an isolated Mojo connection over it.
  mojo::PlatformChannel channel;
  mach_msg_base_t message{};
  base::ScopedMachMsgDestroy scoped_message(&message.header);
  message.header.msgh_id = app_shim::kBootstrapMsgId;
  message.header.msgh_bits =
      MACH_MSGH_BITS(MACH_MSG_TYPE_MOVE_SEND, MACH_MSG_TYPE_MOVE_SEND);
  message.header.msgh_size = sizeof(message);
  message.header.msgh_local_port =
      channel.TakeLocalEndpoint().TakePlatformHandle().ReleaseMachSendRight();
  message.header.msgh_remote_port =
      server_endpoint.TakePlatformHandle().ReleaseMachSendRight();
  kern_return_t kr = mach_msg_send(&message.header);
  if (kr != KERN_SUCCESS) {
    MACH_LOG(ERROR, kr) << "mach_msg_send";
    return mojo::PlatformChannelEndpoint();
  }
  scoped_message.Disarm();
  return channel.TakeRemoteEndpoint();
}

void AppShimController::OnBuildMismatch(
    mojo::PlatformChannelEndpoint server_endpoint) {
  LOG(ERROR) << "The host app is running another build of Electron than the "
                "one on disk; it has to be restarted before this app can "
                "connect to it.";

  // Telling the person is the host's to do, in its own words: it gets an
  // event for this message. It finds out who sent the message from the
  // sender's audit token, which needs this process to still be there, so the
  // host is given a moment to say that it has done so before this process
  // quits. It says so by letting go of the reply right.
  base::apple::ScopedMachReceiveRight reply;
  if (!base::apple::CreateMachPort(&reply, nullptr)) {
    Terminate();
    return;
  }
  mach_msg_base_t message{};
  base::ScopedMachMsgDestroy scoped_message(&message.header);
  message.header.msgh_id = app_shim::kBuildMismatchMsgId;
  message.header.msgh_bits =
      MACH_MSGH_BITS(MACH_MSG_TYPE_MOVE_SEND, MACH_MSG_TYPE_MAKE_SEND_ONCE);
  message.header.msgh_size = sizeof(message);
  message.header.msgh_remote_port =
      server_endpoint.TakePlatformHandle().ReleaseMachSendRight();
  message.header.msgh_local_port = reply.get();
  if (kern_return_t kr = mach_msg_send(&message.header); kr != KERN_SUCCESS) {
    MACH_LOG(ERROR, kr) << "mach_msg_send";
    Terminate();
    return;
  }
  scoped_message.Disarm();

  struct {
    mach_msg_header_t header;
    mach_msg_trailer_t trailer;
  } acknowledgement{};
  mach_msg(&acknowledgement.header, MACH_RCV_MSG | MACH_RCV_TIMEOUT, 0,
           sizeof(acknowledgement), reply.get(),
           kBuildMismatchAcknowledgementTimeout.InMilliseconds(),
           MACH_PORT_NULL);
  Terminate();
}

void AppShimController::OnConnected(mojo::PlatformChannelEndpoint endpoint) {
  mojo::ScopedMessagePipeHandle message_pipe =
      bootstrap_connection_.Connect(std::move(endpoint));
  CHECK(message_pipe.is_valid());
  host_bootstrap_.Bind(mojo::PendingRemote<mojom::AppShimHostBootstrap>(
      std::move(message_pipe), 0));
  host_bootstrap_.set_disconnect_handler(
      base::BindOnce(&AppShimController::Terminate, base::Unretained(this)));
  host_bootstrap_->OnShimConnected(
      std::move(host_receiver_),
      base::BindOnce(&AppShimController::OnShimConnectedResponse,
                     base::Unretained(this)));
}

void AppShimController::OnShimConnectedResponse(
    mojo::PendingReceiver<mojom::AppShim> app_shim_receiver) {
  if (!app_shim_receiver.is_valid()) {
    LOG(ERROR) << "The host app turned this app shim away.";
    Terminate();
    return;
  }
  // The pipe every remote_cocoa interface is associated with. It is read on
  // this runner so that a window resize can wait for the host's frames.
  shim_receiver_.Bind(std::move(app_shim_receiver),
                      ui::WindowResizeHelperMac::Get()->task_runner());
  shim_receiver_.set_disconnect_handler(
      base::BindOnce(&AppShimController::Terminate, base::Unretained(this)));
  host_bootstrap_.reset();
}

void AppShimController::Terminate() {
  terminating_ = true;
  waiting_for_quit_answer_ = false;
  quit_answer_timer_.Stop();
  [NSApp terminate:nil];
}

void AppShimController::OnQuitAnswerTimeout() {
  LOG(WARNING) << "The host app did not answer the quit request; quitting.";
  Terminate();
}

void AppShimController::OnReopen(bool has_visible_windows) {
  host_->ReopenApp(has_visible_windows);
}

void AppShimController::OnOpenFiles(std::vector<base::FilePath> files) {
  host_->FilesOpened(files);
}

void AppShimController::OnOpenUrls(std::vector<GURL> urls) {
  host_->UrlsOpened(urls);
}

bool AppShimController::ShouldTerminate() {
  if (terminating_ || !shim_receiver_.is_bound())
    return true;
  if (!waiting_for_quit_answer_) {
    waiting_for_quit_answer_ = true;
    host_->QuitRequested();
    quit_answer_timer_.Start(FROM_HERE, kQuitAnswerTimeout, this,
                             &AppShimController::OnQuitAnswerTimeout);
  }
  return false;
}

void AppShimController::EnableAccessibilitySupport(
    mojom::AppShimAccessibilityMode mode) {
  bool& asked = mode == mojom::AppShimAccessibilityMode::kComplete
                    ? asked_for_complete_accessibility_
                    : asked_for_partial_accessibility_;
  if (asked || asked_for_complete_accessibility_)
    return;
  asked = true;
  host_->EnableAccessibilitySupport(mode);
}

void AppShimController::CreateRemoteCocoaApplication(
    mojo::PendingAssociatedReceiver<remote_cocoa::mojom::Application>
        application) {
  remote_cocoa::ApplicationBridge::Get()->BindReceiver(std::move(application));
  remote_cocoa::ApplicationBridge::Get()->SetContentNSViewCreateCallbacks(
      base::BindRepeating(
          [](uint64_t view_id, mojo::ScopedInterfaceEndpointHandle host_handle,
             mojo::ScopedInterfaceEndpointHandle view_request_handle) {
            remote_cocoa::CreateRenderWidgetHostNSView(
                view_id, std::move(host_handle), std::move(view_request_handle),
                remote_cocoa::RenderWidgetHostViewMacDelegateCallback());
          }),
      base::BindRepeating(remote_cocoa::CreateWebContentsNSView));
}

void AppShimController::SetBadgeLabel(const std::string& label) {
  NSApp.dockTile.badgeLabel =
      label.empty() ? nil : base::SysUTF8ToNSString(label);
}

void AppShimController::SetUserAttention(mojom::AppShimAttentionType type) {
  switch (type) {
    case mojom::AppShimAttentionType::kCancel:
      [NSApp cancelUserAttentionRequest:attention_request_id_];
      break;
    case mojom::AppShimAttentionType::kInformational:
      attention_request_id_ =
          [NSApp requestUserAttention:NSInformationalRequest];
      break;
    case mojom::AppShimAttentionType::kCritical:
      attention_request_id_ = [NSApp requestUserAttention:NSCriticalRequest];
      break;
  }
}

void AppShimController::Quit() {
  Terminate();
}

namespace {

// The NSWindow that the host knows as `window_id`, if it is still there.
gfx::NativeWindow WindowForId(uint64_t window_id) {
  auto* bridge = remote_cocoa::NativeWidgetNSWindowBridge::GetFromId(window_id);
  return gfx::NativeWindow(bridge ? bridge->ns_window() : nil);
}

file_dialog::DialogSettings FromMojo(
    const mojom::AppShimFileDialogOptions& options) {
  file_dialog::DialogSettings settings;
  settings.title = options.title;
  settings.message = options.message;
  settings.button_label = options.button_label;
  settings.name_field_label = options.name_field_label;
  settings.default_path = options.default_path;
  for (const auto& filter : options.filters)
    settings.filters.emplace_back(filter->name, filter->extensions);
  settings.properties = options.properties;
  settings.shows_tag_field = options.shows_tag_field;
  return settings;
}

}  // namespace

void AppShimController::InitWindowChrome(uint64_t window_id,
                                         mojom::AppShimWindowChromePtr chrome) {
  [[ElectronAppShimWindow forWindowId:window_id] initChrome:*chrome];
}

void AppShimController::SetWindowButtonVisibility(uint64_t window_id,
                                                  bool visible) {
  [[ElectronAppShimWindow forWindowId:window_id] setButtonVisibility:visible];
}

void AppShimController::SetWindowButtonPosition(
    uint64_t window_id,
    const std::optional<gfx::Point>& position) {
  [[ElectronAppShimWindow forWindowId:window_id] setButtonPosition:position];
}

void AppShimController::SetWindowVibrancy(uint64_t window_id,
                                          const std::string& type,
                                          int32_t duration_ms,
                                          int32_t state) {
  [[ElectronAppShimWindow forWindowId:window_id]
      setVibrancy:type
         duration:duration_ms
            state:static_cast<NSVisualEffectState>(state)];
}

void AppShimController::PerformWindowTabAction(uint64_t window_id,
                                               mojom::AppShimTabAction action) {
  [[ElectronAppShimWindow forWindowId:window_id] performTabAction:action];
}

void AppShimController::AddTabbedWindow(uint64_t window_id,
                                        uint64_t tabbed_window_id) {
  [[ElectronAppShimWindow forWindowId:window_id]
      addTabbedWindow:[ElectronAppShimWindow forWindowId:tabbed_window_id]];
}

void AppShimController::PreviewFile(uint64_t window_id,
                                    const base::FilePath& path,
                                    const std::string& display_name) {
  [[ElectronAppShimWindow forWindowId:window_id]
      previewFile:base::apple::FilePathToNSURL(path)
            title:base::SysUTF8ToNSString(display_name)];
}

void AppShimController::CloseFilePreview(uint64_t window_id) {
  [[ElectronAppShimWindow forWindowId:window_id] closeFilePreview];
}

void AppShimController::OnNewWindowForTab(uint64_t window_id) {
  if (shim_receiver_.is_bound())
    host_->NewWindowForTab(window_id);
}

void AppShimController::ShowMessageBox(
    uint64_t window_id,
    mojom::AppShimMessageBoxOptionsPtr options,
    ShowMessageBoxCallback callback) {
  const gfx::NativeWindow window = WindowForId(window_id);
  if (!window) {
    std::move(callback).Run(options->cancel_id, false);
    return;
  }
  MessageBoxSettings settings;
  settings.type = static_cast<MessageBoxType>(options->type);
  settings.buttons = options->buttons;
  settings.default_id = options->default_id;
  settings.cancel_id = options->cancel_id;
  settings.message = options->message;
  settings.detail = options->detail;
  settings.checkbox_label = options->checkbox_label;
  settings.checkbox_checked = options->checkbox_checked;
  settings.icon = options->icon;
  settings.text_width = options->text_width;
  if (options->has_id)
    settings.id = options->id;
  ShowMessageBoxForNSWindow(settings, window, std::move(callback));
}

void AppShimController::CloseMessageBox(int32_t id) {
  electron::CloseMessageBox(id);
}

void AppShimController::ShowOpenDialog(
    uint64_t window_id,
    mojom::AppShimFileDialogOptionsPtr options,
    ShowOpenDialogCallback callback) {
  const gfx::NativeWindow window = WindowForId(window_id);
  if (!window) {
    std::move(callback).Run(true, {});
    return;
  }
  file_dialog::ShowOpenDialogForNSWindow(FromMojo(*options), window,
                                         std::move(callback));
}

void AppShimController::ShowSaveDialog(
    uint64_t window_id,
    mojom::AppShimFileDialogOptionsPtr options,
    ShowSaveDialogCallback callback) {
  const gfx::NativeWindow window = WindowForId(window_id);
  if (!window) {
    std::move(callback).Run(true, base::FilePath());
    return;
  }
  file_dialog::ShowSaveDialogForNSWindow(FromMojo(*options), window,
                                         std::move(callback));
}

void AppShimController::SetApplicationMenu(
    std::optional<std::vector<mojom::AppShimMenuItemPtr>> items) {
  if (!items) {
    if (menu_) {
      menu_controller_ = nil;
      menu_.reset();
      SetUpMenu();
    }
    return;
  }
  // The usual case is the same menu with another state, which the menu on
  // screen takes from the model the next time it looks.
  if (menu_ && menu_->Update(&*items))
    return;

  // The controller refers to the model, so it goes first.
  menu_controller_ = nil;
  menu_ =
      std::make_unique<AppShimMenu>(std::move(*items), std::vector<uint32_t>(),
                                    static_cast<AppShimMenu::Client*>(this));
  menu_controller_ =
      [[ElectronMenuController alloc] initWithModel:menu_->model()
                              useDefaultAccelerator:YES];
  [NSApp setMainMenu:[menu_controller_ menu]];
}

void AppShimController::OnMenuItemActivated(const std::vector<uint32_t>& path,
                                            int command_id,
                                            int event_flags) {
  if (shim_receiver_.is_bound())
    host_->MenuItemActivated(path, command_id, event_flags);
}

void AppShimController::OnMenuWillShow() {
  if (shim_receiver_.is_bound())
    host_->MenuWillShow();
}

void AppShimController::OnMenuWillClose() {
  if (shim_receiver_.is_bound())
    host_->MenuWillClose();
}

void AppShimController::OnDidBecomeActive() {
  if (shim_receiver_.is_bound())
    host_->DidBecomeActive();
}

void AppShimController::CancelQuit() {
  waiting_for_quit_answer_ = false;
  quit_answer_timer_.Stop();
}

void AppShimController::SetUpMenu() {
  NSString* name = AppName();
  NSMenu* main_menu = [[NSMenu alloc] initWithTitle:@""];

  NSMenuItem* app_item = [main_menu addItemWithTitle:name
                                              action:nil
                                       keyEquivalent:@""];
  NSMenu* app_menu = [[NSMenu alloc] initWithTitle:name];
  [app_menu addItemWithTitle:[NSString stringWithFormat:@"Hide %@", name]
                      action:@selector(hide:)
               keyEquivalent:@"h"];
  NSMenuItem* hide_others =
      [app_menu addItemWithTitle:@"Hide Others"
                          action:@selector(hideOtherApplications:)
                   keyEquivalent:@"h"];
  hide_others.keyEquivalentModifierMask =
      NSEventModifierFlagCommand | NSEventModifierFlagOption;
  [app_menu addItemWithTitle:@"Show All"
                      action:@selector(unhideAllApplications:)
               keyEquivalent:@""];
  [app_menu addItem:[NSMenuItem separatorItem]];
  [app_menu addItemWithTitle:[NSString stringWithFormat:@"Quit %@", name]
                      action:@selector(terminate:)
               keyEquivalent:@"q"];
  app_item.submenu = app_menu;

  NSMenuItem* edit_item = [main_menu addItemWithTitle:@"Edit"
                                               action:nil
                                        keyEquivalent:@""];
  NSMenu* edit_menu = [[NSMenu alloc] initWithTitle:@"Edit"];
  [edit_menu addItemWithTitle:@"Undo"
                       action:@selector(undo:)
                keyEquivalent:@"z"];
  [edit_menu addItemWithTitle:@"Redo"
                       action:@selector(redo:)
                keyEquivalent:@"Z"];
  [edit_menu addItem:[NSMenuItem separatorItem]];
  [edit_menu addItemWithTitle:@"Cut" action:@selector(cut:) keyEquivalent:@"x"];
  [edit_menu addItemWithTitle:@"Copy"
                       action:@selector(copy:)
                keyEquivalent:@"c"];
  [edit_menu addItemWithTitle:@"Paste"
                       action:@selector(paste:)
                keyEquivalent:@"v"];
  [edit_menu addItemWithTitle:@"Select All"
                       action:@selector(selectAll:)
                keyEquivalent:@"a"];
  edit_item.submenu = edit_menu;

  NSMenuItem* window_item = [main_menu addItemWithTitle:@"Window"
                                                 action:nil
                                          keyEquivalent:@""];
  NSMenu* window_menu = [[NSMenu alloc] initWithTitle:@"Window"];
  [window_menu addItemWithTitle:@"Minimize"
                         action:@selector(performMiniaturize:)
                  keyEquivalent:@"m"];
  [window_menu addItemWithTitle:@"Zoom"
                         action:@selector(performZoom:)
                  keyEquivalent:@""];
  [window_menu addItemWithTitle:@"Close"
                         action:@selector(performClose:)
                  keyEquivalent:@"w"];
  [window_menu addItem:[NSMenuItem separatorItem]];
  [window_menu addItemWithTitle:@"Bring All to Front"
                         action:@selector(arrangeInFront:)
                  keyEquivalent:@""];
  window_item.submenu = window_menu;
  NSApp.windowsMenu = window_menu;

  NSApp.mainMenu = main_menu;
}

}  // namespace electron
