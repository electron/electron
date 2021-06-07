// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_converters/extension_converter.h"

#include "extensions/common/extension.h"
#include "gin/dictionary.h"
#include "shell/browser/api/electron_api_session.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_converters/value_converter.h"

namespace gin {

// static
v8::Local<v8::Value> Converter<const extensions::Extension*>::ToV8(
    v8::Isolate* isolate,
    const extensions::Extension* extension) {
  auto dict = gin::Dictionary::CreateEmpty(isolate);
  dict.Set("id", extension->id());
  dict.Set("name", extension->name());
  dict.Set("path", extension->path());
  dict.Set("url", extension->url());
  dict.Set("version", extension->VersionString());
  dict.Set("manifest", *(extension->manifest()->value()));

  return gin::ConvertToV8(isolate, dict);
}

bool Converter<electron::api::ExtensionTabDetails>::FromV8(
    v8::Isolate* isolate,
    v8::Local<v8::Value> val,
    electron::api::ExtensionTabDetails* out) {
  gin_helper::Dictionary options;
  if (!ConvertFromV8(isolate, val, &options))
    return false;

  *out = electron::api::ExtensionTabDetails();

  options.Get("windowId", &out->window_id);
  options.Get("active", &out->active);
  options.Get("highlighted", &out->highlighted);
  options.Get("pinned", &out->pinned);
  options.Get("groupId", &out->group_id);
  options.Get("index", &out->index);
  options.Get("discarded", &out->discarded);
  options.Get("autoDiscardable", &out->auto_discardable);
  options.Get("openerTabId", &out->opener_tab_id);
  options.Get("mutedReason", &out->muted_reason);
  options.Get("mutedExtensionId", &out->muted_extension_id);

  return true;
}

}  // namespace gin
