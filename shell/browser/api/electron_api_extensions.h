// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_EXTENSIONS_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_EXTENSIONS_H_

#include "base/memory/raw_ptr.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_registry_observer.h"
#include "gin/wrappable.h"
#include "shell/browser/event_emitter_mixin.h"

namespace gin {
template <typename T>
class Handle;
}  // namespace gin

namespace electron {

class ElectronBrowserContext;

namespace api {

class Extensions final : public gin::DeprecatedWrappable<Extensions>,
                         public gin_helper::EventEmitterMixin<Extensions>,
                         private extensions::ExtensionRegistryObserver {
 public:
  static gin::Handle<Extensions> Create(
      v8::Isolate* isolate,
      ElectronBrowserContext* browser_context);

  // gin::Wrappable
  static gin::DeprecatedWrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

  v8::Local<v8::Promise> LoadExtension(v8::Isolate* isolate,
                                       const base::FilePath& extension_path,
                                       gin::Arguments* args);
  void RemoveExtension(const std::string& extension_id);
  v8::Local<v8::Value> GetExtension(v8::Isolate* isolate,
                                    const std::string& extension_id);
  v8::Local<v8::Value> GetAllExtensions(v8::Isolate* isolate);

  // extensions::ExtensionRegistryObserver:
  void OnExtensionLoaded(content::BrowserContext* browser_context,
                         const extensions::Extension* extension) override;
  void OnExtensionReady(content::BrowserContext* browser_context,
                        const extensions::Extension* extension) override;
  void OnExtensionUnloaded(content::BrowserContext* browser_context,
                           const extensions::Extension* extension,
                           extensions::UnloadedExtensionReason reason) override;

  // disable copy
  Extensions(const Extensions&) = delete;
  Extensions& operator=(const Extensions&) = delete;

 protected:
  explicit Extensions(v8::Isolate* isolate,
                      ElectronBrowserContext* browser_context);
  ~Extensions() override;

 private:
  content::BrowserContext* browser_context() const {
    return browser_context_.get();
  }

  raw_ptr<content::BrowserContext> browser_context_;
};

}  // namespace api

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_EXTENSIONS_H_
