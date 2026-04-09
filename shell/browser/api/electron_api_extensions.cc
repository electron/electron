// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_extensions.h"

#include "chrome/browser/browser_process.h"
#include "extensions/browser/extension_registry.h"
#include "gin/data_object_builder.h"
#include "gin/object_template_builder.h"
#include "shell/browser/api/electron_api_extensions.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/extensions/electron_extension_system.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/extension_converter.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/handle.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/node_util.h"

namespace electron::api {

gin::DeprecatedWrapperInfo Extensions::kWrapperInfo = {gin::kEmbedderNativeGin};

Extensions::Extensions(v8::Isolate* isolate,
                       ElectronBrowserContext* browser_context)
    : browser_context_(browser_context) {
  extensions::ExtensionRegistry::Get(browser_context)->AddObserver(this);
}

Extensions::~Extensions() {
  extensions::ExtensionRegistry::Get(browser_context())->RemoveObserver(this);
}

// static
gin_helper::Handle<Extensions> Extensions::Create(
    v8::Isolate* isolate,
    ElectronBrowserContext* browser_context) {
  return gin_helper::CreateHandle(isolate,
                                  new Extensions(isolate, browser_context));
}

v8::Local<v8::Promise> Extensions::LoadExtension(
    v8::Isolate* isolate,
    const base::FilePath& extension_path,
    gin::Arguments* args) {
  gin_helper::Promise<const extensions::Extension*> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  if (!extension_path.IsAbsolute()) {
    promise.RejectWithErrorMessage(
        "The path to the extension in 'loadExtension' must be absolute");
    return handle;
  }

  if (browser_context()->IsOffTheRecord()) {
    promise.RejectWithErrorMessage(
        "Extensions cannot be loaded in a temporary session");
    return handle;
  }

  int load_flags = extensions::Extension::FOLLOW_SYMLINKS_ANYWHERE;
  gin_helper::Dictionary options;
  if (args->GetNext(&options)) {
    bool allowFileAccess = false;
    options.Get("allowFileAccess", &allowFileAccess);
    if (allowFileAccess)
      load_flags |= extensions::Extension::ALLOW_FILE_ACCESS;
  }

  auto* extension_system = static_cast<extensions::ElectronExtensionSystem*>(
      extensions::ExtensionSystem::Get(browser_context()));
  extension_system->LoadExtension(
      extension_path, load_flags,
      base::BindOnce(
          [](gin_helper::Promise<const extensions::Extension*> promise,
             const extensions::Extension* extension,
             const std::string& error_msg) {
            if (extension) {
              if (!error_msg.empty())
                util::EmitWarning(promise.isolate(), error_msg,
                                  "ExtensionLoadWarning");
              promise.Resolve(extension);
            } else {
              promise.RejectWithErrorMessage(error_msg);
            }
          },
          std::move(promise)));

  return handle;
}

void Extensions::RemoveExtension(const std::string& extension_id) {
  auto* extension_system = static_cast<extensions::ElectronExtensionSystem*>(
      extensions::ExtensionSystem::Get(browser_context()));
  extension_system->RemoveExtension(extension_id);
}

v8::Local<v8::Value> Extensions::GetExtension(v8::Isolate* isolate,
                                              const std::string& extension_id) {
  auto* registry = extensions::ExtensionRegistry::Get(browser_context());
  const extensions::Extension* extension =
      registry->GetInstalledExtension(extension_id);
  if (extension) {
    return gin::ConvertToV8(isolate, extension);
  } else {
    return v8::Null(isolate);
  }
}

v8::Local<v8::Value> Extensions::GetAllExtensions(v8::Isolate* isolate) {
  auto* registry = extensions::ExtensionRegistry::Get(browser_context());
  const extensions::ExtensionSet extensions =
      registry->GenerateInstalledExtensionsSet();
  std::vector<const extensions::Extension*> extensions_vector;
  for (const auto& extension : extensions) {
    if (extension->location() !=
        extensions::mojom::ManifestLocation::kComponent)
      extensions_vector.emplace_back(extension.get());
  }
  return gin::ConvertToV8(isolate, extensions_vector);
}

void Extensions::OnExtensionLoaded(content::BrowserContext* browser_context,
                                   const extensions::Extension* extension) {
  Emit("extension-loaded", extension);
}

void Extensions::OnExtensionUnloaded(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension,
    extensions::UnloadedExtensionReason reason) {
  Emit("extension-unloaded", extension);
}

void Extensions::OnExtensionReady(content::BrowserContext* browser_context,
                                  const extensions::Extension* extension) {
  Emit("extension-ready", extension);
}

// static
gin::ObjectTemplateBuilder Extensions::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin_helper::EventEmitterMixin<Extensions>::GetObjectTemplateBuilder(
             isolate)
      .SetMethod("loadExtension", &Extensions::LoadExtension)
      .SetMethod("removeExtension", &Extensions::RemoveExtension)
      .SetMethod("getExtension", &Extensions::GetExtension)
      .SetMethod("getAllExtensions", &Extensions::GetAllExtensions);
}

const char* Extensions::GetTypeName() {
  return "Extensions";
}

}  // namespace electron::api
