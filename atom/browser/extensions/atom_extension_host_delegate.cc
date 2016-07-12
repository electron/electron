// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/extensions/atom_extension_host_delegate.h"

#include "atom/browser/extensions/atom_extension_web_contents_observer.h"
#include "base/lazy_instance.h"
#include "extensions/browser/extension_host.h"
#include "extensions/browser/load_monitoring_extension_host_queue.h"
#include "extensions/browser/process_manager.h"
#include "extensions/browser/serial_extension_host_queue.h"

#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"

namespace extensions {

namespace {

// Singleton for GetExtensionHostQueue().
struct QueueWrapper {
  QueueWrapper() {
    queue.reset(new LoadMonitoringExtensionHostQueue(
        std::unique_ptr<ExtensionHostQueue>(new SerialExtensionHostQueue())));
  }
  std::unique_ptr<ExtensionHostQueue> queue;
};
base::LazyInstance<QueueWrapper> g_queue = LAZY_INSTANCE_INITIALIZER;

}  // namespace

AtomExtensionHostDelegate::AtomExtensionHostDelegate() {}

AtomExtensionHostDelegate::~AtomExtensionHostDelegate() {}

void AtomExtensionHostDelegate::OnExtensionHostCreated(
    content::WebContents* web_contents) {
  AtomExtensionWebContentsObserver::CreateForWebContents(web_contents);
}

void AtomExtensionHostDelegate::OnRenderViewCreatedForBackgroundPage(
    ExtensionHost* host) {
  extensions::ProcessManager::Get(
    host->browser_context())->IncrementLazyKeepaliveCount(host->extension());
}

content::JavaScriptDialogManager*
AtomExtensionHostDelegate::GetJavaScriptDialogManager() {
  NOTIMPLEMENTED();
  return nullptr;
}

void AtomExtensionHostDelegate::CreateTab(content::WebContents* web_contents,
                                            const std::string& extension_id,
                                            WindowOpenDisposition disposition,
                                            const gfx::Rect& initial_rect,
                                            bool user_gesture) {
  NOTIMPLEMENTED();
}

void AtomExtensionHostDelegate::ProcessMediaAccessRequest(
    content::WebContents* web_contents,
    const content::MediaStreamRequest& request,
    const content::MediaResponseCallback& callback,
    const Extension* extension) {
}

bool AtomExtensionHostDelegate::CheckMediaAccessPermission(
    content::WebContents* web_contents,
    const GURL& security_origin,
    content::MediaStreamType type,
    const Extension* extension) {
  return false;
}

ExtensionHostQueue* AtomExtensionHostDelegate::GetExtensionHostQueue() const {
  return g_queue.Get().queue.get();
}

}  // namespace extensions
