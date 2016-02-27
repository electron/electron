// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_extension.h"

#include <string>
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "atom/common/node_includes.h"
#include "base/files/file_path.h"
#include "base/prefs/pref_service.h"
#include "base/strings/string_util.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "content/public/common/url_constants.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/notification_types.h"
#include "extensions/common/file_util.h"
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


namespace atom {

namespace api {

Extension::Extension() {}
Extension::~Extension() {}

// static
Extension* Extension::GetInstance() {
  return base::Singleton<Extension>::get();
}

// static
std::string Extension::Load(
    v8::Isolate* isolate,
    const base::FilePath& path,
    const extensions::Manifest::Location& manifest_location,
    int flags) {

  std::string error;
  scoped_refptr<extensions::Extension> extension =
    extensions::file_util::LoadExtension(path,
                                      manifest_location,
                                      flags,
                                      &error);

  if (error.empty()) {
    Install(extension);
  }

  return error;
}

// static
void Extension::Install(
    const scoped_refptr<const extensions::Extension>& extension) {
  if (!content::BrowserThread::CurrentlyOn(content::BrowserThread::UI))
    content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
        base::Bind(Install, extension));

  GetInstance()->extensions_.Insert(extension);
  content::NotificationService::current()->Notify(
        extensions::NOTIFICATION_CRX_INSTALLER_DONE,
        content::Source<content::BrowserContext>(NULL),
        content::Details<const extensions::Extension>(extension.get()));
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
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_extension, Initialize)
