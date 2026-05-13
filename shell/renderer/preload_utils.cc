// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/preload_utils.h"

#include <string>
#include <string_view>

#include "base/containers/span.h"
#include "base/no_destructor.h"
#include "base/process/process.h"
#include "base/strings/strcat.h"
#include "chrome/common/chrome_version.h"
#include "electron/buildflags/buildflags.h"
#include "electron/electron_version.h"
#include "mojo/public/cpp/base/big_buffer.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"
#include "third_party/electron_node/src/node_metadata.h"
#include "third_party/icu/source/common/unicode/uvernum.h"
#include "third_party/icu/source/i18n/unicode/timezone.h"
#include "third_party/icu/source/i18n/unicode/ulocdata.h"
#include "v8/include/v8-context.h"
#include "v8/include/v8-exception.h"

namespace electron::preload_utils {

namespace {

constexpr std::string_view kBindingCacheKey = "native-binding-cache";

v8::Local<v8::Object> GetBindingCache(v8::Isolate* isolate) {
  auto context = isolate->GetCurrentContext();
  gin_helper::Dictionary global(isolate, context->Global());
  v8::Local<v8::Value> cache;

  if (!global.GetHidden(kBindingCacheKey, &cache)) {
    cache = v8::Object::New(isolate);
    global.SetHidden(kBindingCacheKey, cache);
  }

  return cache->ToObject(context).ToLocalChecked();
}

}  // namespace

// adapted from node.cc
v8::Local<v8::Value> GetBinding(v8::Isolate* isolate,
                                v8::Local<v8::String> key) {
  v8::Local<v8::Object> exports;
  const std::string binding_key = gin::V8ToString(isolate, key);
  gin_helper::Dictionary cache(isolate, GetBindingCache(isolate));

  if (cache.Get(binding_key, &exports)) {
    return exports;
  }

  auto* const mod = node::binding::get_linked_module(binding_key.c_str());
  if (!mod) {
    gin_helper::ErrorThrower{isolate}.ThrowError(
        base::StrCat({"No such binding: ", binding_key}));
    return exports;
  }

  exports = v8::Object::New(isolate);
  DCHECK_EQ(mod->nm_register_func, nullptr);
  DCHECK_NE(mod->nm_context_register_func, nullptr);
  mod->nm_context_register_func(exports, v8::Null(isolate),
                                isolate->GetCurrentContext(), mod->nm_priv);
  cache.Set(binding_key, exports);
  return exports;
}

v8::Local<v8::Value> CreatePreloadScript(v8::Isolate* isolate,
                                         v8::Local<v8::String> source) {
  auto context = isolate->GetCurrentContext();
  auto maybe_script = v8::Script::Compile(context, source);
  v8::Local<v8::Script> script;
  if (!maybe_script.ToLocal(&script))
    return {};
  return script->Run(context).ToLocalChecked();
}

double Uptime() {
  return (base::Time::Now() - base::Process::Current().CreationTime())
      .InSecondsF();
}

namespace {

// Builds process.versions from per-process metadata — no node::Environment
// needed. cldr/tz are runtime ICU state; the sandboxed renderer doesn't run
// Node bootstrap (which would call InitializeIntlVersions()), so compute them
// from the renderer's own ICU.
v8::Local<v8::Value> BuildVersions(v8::Isolate* isolate) {
  static base::NoDestructor<std::string> cldr_version([] {
    UErrorCode status = U_ZERO_ERROR;
    UVersionInfo version_array;
    char buf[U_MAX_VERSION_STRING_LENGTH] = {0};
    ulocdata_getCLDRVersion(version_array, &status);
    if (U_SUCCESS(status))
      u_versionToString(version_array, buf);
    return std::string(buf);
  }());
  static base::NoDestructor<std::string> tz_version([] {
    UErrorCode status = U_ZERO_ERROR;
    const char* tz = icu::TimeZone::getTZDataVersion(status);
    return U_SUCCESS(status) && tz ? std::string(tz) : std::string();
  }());

  auto versions = gin_helper::Dictionary::CreateEmpty(isolate);
  for (const auto& [key, value] :
       node::per_process::metadata.versions.pairs()) {
    if (key == "cldr")
      versions.SetReadOnly(key, *cldr_version);
    else if (key == "tz")
      versions.SetReadOnly(key, *tz_version);
    else
      versions.SetReadOnly(key, value);
  }
  versions.SetReadOnly(ELECTRON_PROJECT_NAME, ELECTRON_VERSION_STRING);
  versions.SetReadOnly("chrome", CHROME_VERSION_STRING);
#if BUILDFLAG(HAS_VENDOR_VERSION)
  versions.SetReadOnly(BUILDFLAG(VENDOR_VERSION_NAME),
                       BUILDFLAG(VENDOR_VERSION_VALUE));
#endif
  return versions.GetHandle();
}

}  // namespace

v8::MaybeLocal<v8::Value> BuildStartupData(
    v8::Isolate* isolate,
    const mojom::RendererStartupDataPtr& data) {
  if (!data)
    return {};

  auto out = gin_helper::Dictionary::CreateEmpty(isolate);

  // preloadScripts: [{ id, type, filePath, contents, error }]
  v8::LocalVector<v8::Value> scripts(isolate);
  scripts.reserve(data->preload_scripts.size());
  for (const auto& ps : data->preload_scripts) {
    auto entry = gin_helper::Dictionary::CreateEmpty(isolate);
    entry.Set("id", ps->id);
    entry.Set("type", ps->type);
    entry.Set("filePath", ps->file_path);
    // BigBuffer uses inline bytes below 64 KB and shared memory above; the
    // implicit base::span conversion handles both backing stores. byte_span()
    // would CHECK-fail for the shared-memory case.
    base::span<const uint8_t> bytes = ps->contents;
    entry.Set("contents",
              std::string_view(reinterpret_cast<const char*>(bytes.data()),
                               bytes.size()));
    // Match the legacy IPC handler shape: `error` is an Error object when the
    // file read failed (the legacy path serialized the fs.readFile error
    // through the IPC), and absent otherwise.
    if (ps->error) {
      entry.Set("error", v8::Local<v8::Value>(v8::Exception::Error(
                             gin::StringToV8(isolate, *ps->error))));
    }
    scripts.push_back(entry.GetHandle());
  }
  out.Set("preloadScripts",
          v8::Array::New(isolate, scripts.data(), scripts.size()));

  // process: { arch, platform, env, version, versions, execPath } — same shape
  // as the legacy BROWSER_SANDBOX_LOAD reply. arch/platform/version/versions
  // are filled from this binary's compiled-in metadata instead of being
  // shipped over the wire (they are identical in browser and renderer).
  auto proc = gin_helper::Dictionary::CreateEmpty(isolate);
  proc.Set("arch", node::per_process::metadata.arch);
  proc.Set("platform", node::per_process::metadata.platform);
  proc.Set("version", "v" + node::per_process::metadata.versions.node);
  proc.Set("versions", BuildVersions(isolate));
  auto env = gin_helper::Dictionary::CreateEmpty(isolate);
  for (const auto& [k, v] : data->environment)
    env.Set(k, v);
  proc.Set("env", env);
  proc.Set("execPath", data->helper_exec_path);
  out.Set("process", proc);

  return out.GetHandle();
}

}  // namespace electron::preload_utils
