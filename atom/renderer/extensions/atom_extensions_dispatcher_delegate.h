// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_EXTENSIONS_ATOM_EXTENSIONS_DISPATCHER_DELEGATE_H_
#define ATOM_RENDERER_EXTENSIONS_ATOM_EXTENSIONS_DISPATCHER_DELEGATE_H_

#include <set>
#include <string>
#include "base/macros.h"
#include "extensions/renderer/dispatcher_delegate.h"
#include "base/memory/scoped_ptr.h"

namespace content {
class RenderViewObserver;
}

namespace extensions {

class AtomExtensionsDispatcherDelegate
    : public extensions::DispatcherDelegate {
 public:
  AtomExtensionsDispatcherDelegate();
  ~AtomExtensionsDispatcherDelegate() override;

 private:
  // extensions::DispatcherDelegate implementation.
  void InitOriginPermissions(const extensions::Extension* extension,
                             bool is_extension_active) override;
  void RegisterNativeHandlers(extensions::Dispatcher* dispatcher,
                              extensions::ModuleSystem* module_system,
                              extensions::ScriptContext* context) override;
  void PopulateSourceMap(
      extensions::ResourceBundleSourceMap* source_map) override;
  void RequireAdditionalModules(extensions::ScriptContext* context,
                                bool is_within_platform_app) override;
  void OnActiveExtensionsUpdated(
      const std::set<std::string>& extensions_ids) override;
  void SetChannel(int channel) override;

  DISALLOW_COPY_AND_ASSIGN(AtomExtensionsDispatcherDelegate);
};

}  // namespace extensions

#endif  // ATOM_RENDERER_EXTENSIONS_ATOM_EXTENSIONS_DISPATCHER_DELEGATE_H_
