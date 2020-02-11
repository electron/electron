// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_RENDERER_EXTENSIONS_ELECTRON_EXTENSIONS_DISPATCHER_DELEGATE_H_
#define SHELL_RENDERER_EXTENSIONS_ELECTRON_EXTENSIONS_DISPATCHER_DELEGATE_H_

#include <set>
#include <string>

#include "base/macros.h"
#include "extensions/renderer/dispatcher_delegate.h"

class ElectronExtensionsDispatcherDelegate
    : public extensions::DispatcherDelegate {
 public:
  ElectronExtensionsDispatcherDelegate();
  ~ElectronExtensionsDispatcherDelegate() override;

 private:
  // extensions::DispatcherDelegate implementation.
  void RegisterNativeHandlers(
      extensions::Dispatcher* dispatcher,
      extensions::ModuleSystem* module_system,
      extensions::NativeExtensionBindingsSystem* bindings_system,
      extensions::ScriptContext* context) override;
  void PopulateSourceMap(
      extensions::ResourceBundleSourceMap* source_map) override;
  void RequireWebViewModules(extensions::ScriptContext* context) override;
  void OnActiveExtensionsUpdated(
      const std::set<std::string>& extensions_ids) override;
  void InitializeBindingsSystem(
      extensions::Dispatcher* dispatcher,
      extensions::NativeExtensionBindingsSystem* bindings_system) override;

  DISALLOW_COPY_AND_ASSIGN(ElectronExtensionsDispatcherDelegate);
};

#endif  // SHELL_RENDERER_EXTENSIONS_ELECTRON_EXTENSIONS_DISPATCHER_DELEGATE_H_
