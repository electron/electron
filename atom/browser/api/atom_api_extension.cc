// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_extension.h"

#include <string>
#include <vector>
#include "atom/browser/api/atom_api_web_contents.h"
#include "atom/browser/extensions/atom_notification_types.h"
#include "atom/browser/extensions/tab_helper.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "atom/common/native_mate_converters/v8_value_converter.h"
#include "atom/common/node_includes.h"
#include "base/files/file_path.h"
#include "components/prefs/pref_service.h"
#include "base/strings/string_util.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/notification_types.h"
#include "extensions/common/extension.h"
#include "extensions/common/file_util.h"
#include "extensions/common/manifest_handlers/background_info.h"
#include "extensions/common/one_shot_event.h"
#include "native_mate/converter.h"
#include "native_mate/dictionary.h"

namespace mate {

template<>
struct Converter<extensions::Manifest::Location> {
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     extensions::Manifest::Location* out) {
    std::string type;
    if (!ConvertFromV8(isolate, val, &type))
      return false;

    if (type == "internal")
      *out = extensions::Manifest::Location::INTERNAL;
    else if (type == "external_pref")
      *out = extensions::Manifest::Location::EXTERNAL_PREF;
    else if (type == "external_registry")
      *out = extensions::Manifest::Location::EXTERNAL_REGISTRY;
    else if (type == "unpacked")
      *out = extensions::Manifest::Location::UNPACKED;
    else if (type == "component")
      *out = extensions::Manifest::Location::COMPONENT;
    else if (type == "external_pref_download")
      *out = extensions::Manifest::Location::EXTERNAL_PREF_DOWNLOAD;
    else if (type == "external_policy_download")
      *out = extensions::Manifest::Location::EXTERNAL_POLICY_DOWNLOAD;
    else if (type == "command_line")
      *out = extensions::Manifest::Location::COMMAND_LINE;
    else if (type == "external_policy")
      *out = extensions::Manifest::Location::EXTERNAL_POLICY;
    else if (type == "external_component")
      *out = extensions::Manifest::Location::EXTERNAL_COMPONENT;
    else
      *out = extensions::Manifest::Location::INVALID_LOCATION;
    return true;
  }
};

}  // namespace mate

namespace {

scoped_refptr<extensions::Extension> LoadExtension(const base::FilePath& path,
    const base::DictionaryValue& manifest,
    const extensions::Manifest::Location& manifest_location,
    int flags,
    std::string* error) {
  scoped_refptr<extensions::Extension> extension(extensions::Extension::Create(
      path, manifest_location, manifest, flags, error));
  if (!extension.get())
    return NULL;

  std::vector<extensions::InstallWarning> warnings;
  if (!extensions::file_util::ValidateExtension(extension.get(),
                                                error,
                                                &warnings))
    return NULL;
  extension->AddInstallWarnings(warnings);

  return extension;
}

}  // namespace

namespace atom {

namespace api {

Extension::Extension() {
  registrar_.Add(this,
                 extensions::NOTIFICATION_EXTENSION_LOADED_DEPRECATED,
                 content::NotificationService::AllBrowserContextsAndSources());
}

Extension::~Extension() {}

void Extension::Observe(
    int type, const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  switch (type) {
    case extensions::NOTIFICATION_EXTENSION_LOADED_DEPRECATED: {
      extensions::Extension* extension =
          content::Details<extensions::Extension>(details).ptr();
      extensions_.Insert(extension);
    }
  }
}

// static
Extension* Extension::GetInstance() {
  return base::Singleton<Extension>::get();
}

// static
v8::Local<v8::Value> Extension::Load(
  v8::Isolate* isolate,
  const base::FilePath& path,
  const base::DictionaryValue& manifest,
  const extensions::Manifest::Location& manifest_location,
  int flags) {
  std::string error;
  scoped_refptr<extensions::Extension> extension;

  std::unique_ptr<base::DictionaryValue> manifest_copy =
      manifest.CreateDeepCopy();
  if (manifest_copy->empty()) {
    manifest_copy = extensions::file_util::LoadManifest(path, &error);
  }

  if (error.empty()) {
    extension = LoadExtension(path,
                              *manifest_copy,
                              manifest_location,
                              flags,
                              &error);
  }

  std::unique_ptr<base::DictionaryValue>
      install_info(new base::DictionaryValue);
  if (error.empty()) {
    Install(extension);
    install_info->SetString("name", extension->non_localized_name());
    install_info->SetString("id", extension->id());
    install_info->SetString("url", extension->url().spec());
    install_info->SetString("base_path", extension->path().value());
    install_info->SetString("version", extension->VersionString());
    install_info->SetString("description", extension->description());
    install_info->Set("manifest", std::move(manifest_copy));
  } else {
    install_info->SetString("error", error);
  }
  std::unique_ptr<atom::V8ValueConverter>
      converter(new atom::V8ValueConverter);
  return converter->ToV8Value(install_info.get(), isolate->GetCurrentContext());
}

// static
void Extension::Install(
    const scoped_refptr<const extensions::Extension>& extension) {
  if (!content::BrowserThread::CurrentlyOn(content::BrowserThread::UI))
    content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
        base::Bind(Install, extension));

  content::NotificationService::current()->Notify(
        extensions::NOTIFICATION_CRX_INSTALLER_DONE,
        content::Source<Extension>(GetInstance()),
        content::Details<const extensions::Extension>(extension.get()));
}

// static
void Extension::Disable(const std::string& extension_id) {
  if (!content::BrowserThread::CurrentlyOn(content::BrowserThread::UI))
    content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
        base::Bind(Disable, extension_id));

  const extensions::Extension* extension =
    GetInstance()->extensions_.GetByID(extension_id);

  if (!extension)
    return;

  content::NotificationService::current()->Notify(
        atom::NOTIFICATION_DISABLE_USER_EXTENSION_REQUEST,
        content::Source<Extension>(GetInstance()),
        content::Details<const extensions::Extension>(extension));
}

// static
void Extension::Enable(const std::string& extension_id) {
  if (!content::BrowserThread::CurrentlyOn(content::BrowserThread::UI))
    content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
        base::Bind(Enable, extension_id));

  const extensions::Extension* extension =
    GetInstance()->extensions_.GetByID(extension_id);

  if (!extension)
    return;

  content::NotificationService::current()->Notify(
        atom::NOTIFICATION_ENABLE_USER_EXTENSION_REQUEST,
        content::Source<Extension>(GetInstance()),
        content::Details<const extensions::Extension>(extension));
}

// static
bool Extension::IsBackgroundPageUrl(GURL url,
                    content::BrowserContext* browser_context) {
  if (url.scheme() != "chrome-extension")
    return false;

  if (extensions::ExtensionSystem::Get(browser_context)
      ->ready().is_signaled()) {
    const extensions::Extension* extension =
        extensions::ExtensionRegistry::Get(browser_context)->
            enabled_extensions().GetExtensionOrAppByURL(url);
    if (extension &&
        url == extensions::BackgroundInfo::GetBackgroundURL(extension))
      return true;
  }

  return false;
}

// static
bool Extension::IsBackgroundPageWebContents(
    content::WebContents* web_contents) {
  auto browser_context = web_contents->GetBrowserContext();
  auto url = web_contents->GetURL();

  return IsBackgroundPageUrl(url, browser_context);
}

// static
bool Extension::IsBackgroundPage(const WebContents* web_contents) {
  return IsBackgroundPageWebContents(web_contents->web_contents());
}

// static
v8::Local<v8::Value> Extension::TabValue(v8::Isolate* isolate,
                    WebContents* web_contents) {
  std::unique_ptr<base::DictionaryValue> value(
      extensions::TabHelper::CreateTabValue(web_contents->web_contents()));
  return mate::ConvertToV8(isolate, *value);
}

// static
bool Extension::HandleURLOverride(GURL* url,
        content::BrowserContext* browser_context) {
  return false;
}

bool Extension::HandleURLOverrideReverse(GURL* url,
          content::BrowserContext* browser_context) {
  return false;
}

}  // namespace api

}  // namespace atom

namespace {

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.SetMethod("load", &atom::api::Extension::Load);
  dict.SetMethod("disable", &atom::api::Extension::Disable);
  dict.SetMethod("enable", &atom::api::Extension::Enable);
  dict.SetMethod("tabValue", &atom::api::Extension::TabValue);
  dict.SetMethod("isBackgroundPage", &atom::api::Extension::IsBackgroundPage);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_extension, Initialize)
