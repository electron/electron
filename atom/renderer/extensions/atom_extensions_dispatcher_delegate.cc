// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/extensions/atom_extensions_dispatcher_delegate.h"

#include <string>
#include <set>
#include "atom/common/javascript_bindings.h"
#include "atom/grit/atom_resources.h"  // NOLINT: This file is generated
#include "chrome/grit/renderer_resources.h"  // NOLINT: This file is generated
#include "chrome/renderer/extensions/tabs_custom_bindings.h"
#include "content/public/renderer/render_frame.h"
#include "extensions/renderer/resource_bundle_source_map.h"

namespace extensions {

AtomExtensionsDispatcherDelegate::AtomExtensionsDispatcherDelegate() {
}

AtomExtensionsDispatcherDelegate::~AtomExtensionsDispatcherDelegate() {
}

void AtomExtensionsDispatcherDelegate::InitOriginPermissions(
    const extensions::Extension* extension,
    bool is_extension_active) {
}

void AtomExtensionsDispatcherDelegate::RegisterNativeHandlers(
    extensions::Dispatcher* dispatcher,
    extensions::ModuleSystem* module_system,
    extensions::ScriptContext* context) {
  module_system->RegisterNativeHandler(
      "atom",
      std::unique_ptr<NativeHandler>(
          new atom::JavascriptBindings(
              context->GetRenderFrame()->GetRenderView(), context)));
  module_system->RegisterNativeHandler(
      "tabs",
      std::unique_ptr<NativeHandler>(
          new extensions::TabsCustomBindings(context)));
}

void AtomExtensionsDispatcherDelegate::PopulateSourceMap(
    extensions::ResourceBundleSourceMap* source_map) {
  source_map->RegisterSource("event_emitter", IDR_ATOM_EVENT_EMITTER_JS);
  source_map->RegisterSource("ipc_utils", IDR_ATOM_IPC_INTERNAL_JS);
  source_map->RegisterSource("ipc", IDR_ATOM_IPC_BINDINGS_JS);
  // TODO(bridiver) - add a permission for this
  // so only component extensions can use
  source_map->RegisterSource("webFrame",
                             IDR_ATOM_WEB_FRAME_BINDINGS_JS);
  source_map->RegisterSource("browserAction",
                             IDR_ATOM_BROWSER_ACTION_BINDINGS_JS);
  source_map->RegisterSource("privacy", IDR_ATOM_PRIVACY_BINDINGS_JS);
  source_map->RegisterSource("tabs",
                             IDR_ATOM_TABS_BINDINGS_JS);
  source_map->RegisterSource("contextMenus",
                             IDR_ATOM_CONTEXT_MENUS_BINDINGS_JS);
  source_map->RegisterSource("contentSettings",
                             IDR_ATOM_CONTENT_SETTINGS_BINDINGS_JS);
  source_map->RegisterSource("windows",
                             IDR_ATOM_WINDOWS_BINDINGS_JS);
}

void AtomExtensionsDispatcherDelegate::RequireAdditionalModules(
    extensions::ScriptContext* context,
    bool is_within_platform_app) {
}

void AtomExtensionsDispatcherDelegate::OnActiveExtensionsUpdated(
    const std::set<std::string>& extension_ids) {
}

void AtomExtensionsDispatcherDelegate::SetChannel(int channel) {
}

}  // namespace extensions
