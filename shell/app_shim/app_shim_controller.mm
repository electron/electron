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
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/mac/scoped_mach_msg_destroy.h"
#include "base/memory/raw_ptr.h"
#include "base/strings/strcat.h"
#include "base/strings/sys_string_conversions.h"
#include "base/task/single_thread_task_runner.h"
#include "components/remote_cocoa/app_shim/application_bridge.h"
#include "content/public/browser/remote_cocoa.h"
#include "mojo/public/cpp/platform/named_platform_channel.h"
#include "mojo/public/cpp/platform/platform_channel.h"
#include "shell/common/app_shim_constants.h"
#include "ui/display/screen.h"
#include "url/gurl.h"

namespace {

// How long a shim waits for the host app to start listening.
constexpr base::TimeDelta kPollTimeout = base::Seconds(60);
constexpr base::TimeDelta kPollPeriod = base::Milliseconds(100);

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

- (NSApplicationTerminateReply)applicationShouldTerminate:
    (NSApplication*)sender {
  return !_controller || _controller->ShouldTerminate() ? NSTerminateNow
                                                        : NSTerminateCancel;
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
  if (mojo::PlatformChannelEndpoint endpoint = ConnectToHost(ServerName());
      endpoint.is_valid()) {
    OnConnected(std::move(endpoint));
    return;
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
    const std::string& server_name) {
  // One bootstrap server takes connections from every shim, so each shim makes
  // a channel of its own and sends the host one end of it in a raw Mach
  // message. Both sides then make an isolated Mojo connection over it.
  mojo::PlatformChannelEndpoint server_endpoint =
      mojo::NamedPlatformChannel::ConnectToServer(server_name);
  if (!server_endpoint.is_valid())
    return mojo::PlatformChannelEndpoint();

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
  shim_receiver_.Bind(std::move(app_shim_receiver));
  shim_receiver_.set_disconnect_handler(
      base::BindOnce(&AppShimController::Terminate, base::Unretained(this)));
  host_bootstrap_.reset();
}

void AppShimController::Terminate() {
  terminating_ = true;
  [NSApp terminate:nil];
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
  host_->QuitRequested();
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
