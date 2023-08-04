// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/api/tabs/tabs_window_api.h"

#include <memory>

#include "base/lazy_instance.h"
#include "extensions/browser/event_router.h"
#include "extensions/browser/extension_system.h"
#include "shell/browser/extensions/api/tabs/tabs_event_router.h"
#include "shell/common/extensions/api/tabs.h"

namespace extensions {

TabsWindowsAPI::TabsWindowsAPI(content::BrowserContext* context)
    : browser_context_(context) {
  EventRouter* event_router = EventRouter::Get(browser_context_);

  // Tabs API Events.
  event_router->RegisterObserver(this, api::tabs::OnZoomChange::kEventName);
}

TabsWindowsAPI::~TabsWindowsAPI() = default;

// static
TabsWindowsAPI* TabsWindowsAPI::Get(content::BrowserContext* context) {
  return BrowserContextKeyedAPIFactory<TabsWindowsAPI>::Get(context);
}

TabsEventRouter* TabsWindowsAPI::tabs_event_router() {
  if (!tabs_event_router_.get()) {
    tabs_event_router_ = std::make_unique<TabsEventRouter>(browser_context_);
  }
  return tabs_event_router_.get();
}

void TabsWindowsAPI::Shutdown() {
  EventRouter::Get(browser_context_)->UnregisterObserver(this);
}

static base::LazyInstance<BrowserContextKeyedAPIFactory<TabsWindowsAPI>>::
    DestructorAtExit g_tabs_windows_api_factory = LAZY_INSTANCE_INITIALIZER;

BrowserContextKeyedAPIFactory<TabsWindowsAPI>*
TabsWindowsAPI::GetFactoryInstance() {
  return g_tabs_windows_api_factory.Pointer();
}

void TabsWindowsAPI::OnListenerAdded(const EventListenerInfo& details) {
  // Initialize the event routers.
  tabs_event_router();
  EventRouter::Get(browser_context_)->UnregisterObserver(this);
}

}  // namespace extensions
