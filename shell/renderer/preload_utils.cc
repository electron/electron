// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/preload_utils.h"

#include <array>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "base/containers/span.h"
#include "base/no_destructor.h"
#include "base/numerics/safe_conversions.h"
#include "base/process/process.h"
#include "base/strings/strcat.h"
#include "chrome/common/chrome_version.h"
#include "content/public/renderer/render_frame.h"
#include "crypto/hash.h"
#include "electron/buildflags/buildflags.h"
#include "electron/electron_version.h"
#include "mojo/public/cpp/base/big_buffer.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"
#include "shell/common/web_contents_utility.mojom.h"
#include "shell/renderer/electron_api_service_impl.h"
#include "shell/renderer/service_worker_data.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/electron_node/src/node_metadata.h"
#include "third_party/icu/source/common/unicode/uvernum.h"
#include "third_party/icu/source/i18n/unicode/timezone.h"
#include "third_party/icu/source/i18n/unicode/ulocdata.h"
#include "v8/include/v8-context.h"
#include "v8/include/v8-exception.h"
#include "v8/include/v8-script.h"

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

namespace {

// Looks up a preload's contents and code cache from the mojo-cached startup
// data so they can be consumed directly without entering V8.
const mojom::PreloadScriptData* LookupPreloadScript(
    content::RenderFrame* render_frame,
    ServiceWorkerData* service_worker_data,
    const std::string& script_id) {
  const mojom::RendererStartupDataPtr* data = nullptr;
  if (render_frame) {
    if (auto* api_service = ElectronApiServiceImpl::Get(render_frame))
      data = &api_service->startup_data();
  } else if (service_worker_data) {
    data = &service_worker_data->worker_startup_data();
  }
  if (!data || !*data)
    return nullptr;
  for (const auto& ps : (*data)->preload_scripts) {
    if (ps->id == script_id)
      return ps.get();
  }
  return nullptr;
}

}  // namespace

v8::Local<v8::Value> CreatePreloadScript(
    content::RenderFrame* render_frame,
    ServiceWorkerData* service_worker_data,
    v8::Isolate* isolate,
    const std::string& script_id,
    const std::vector<std::string>& param_name_strings) {
  auto context = isolate->GetCurrentContext();

  // Contents/cache stay in native buffers; only the single NewFromUtf8 copy
  // for the compile touches V8.
  const mojom::PreloadScriptData* ps =
      LookupPreloadScript(render_frame, service_worker_data, script_id);
  if (!ps)
    return {};

  // Converted only after the lookup succeeds — a missing script bails above
  // without paying for the V8 string conversions.
  std::vector<v8::Local<v8::String>> param_names;
  param_names.reserve(param_name_strings.size());
  for (const auto& n : param_name_strings)
    param_names.push_back(gin::StringToV8(isolate, n));

  base::span<const uint8_t> contents = ps->contents;
  v8::Local<v8::String> body;
  if (!v8::String::NewFromUtf8(
           isolate, reinterpret_cast<const char*>(contents.data()),
           v8::NewStringType::kNormal, base::checked_cast<int>(contents.size()))
           .ToLocal(&body)) {
    return {};
  }

  std::unique_ptr<v8::ScriptCompiler::CachedData> cached_data;
  if (ps->code_cache) {
    base::span<const uint8_t> cache = *ps->code_cache;
    cached_data = std::make_unique<v8::ScriptCompiler::CachedData>(
        cache.data(), base::checked_cast<int>(cache.size()),
        v8::ScriptCompiler::CachedData::BufferNotOwned);
  }
  bool had_cache = !!cached_data;
  // ScriptOrigin gives stack traces the real file path instead of <anonymous>.
  // It's metadata, not part of V8's CachedData hash. Source takes ownership of
  // cached_data.
  v8::ScriptOrigin origin(gin::StringToV8(isolate, ps->file_path));
  v8::ScriptCompiler::Source compiler_source(body, origin,
                                             cached_data.release());
  auto options = had_cache ? v8::ScriptCompiler::kConsumeCodeCache
                           : v8::ScriptCompiler::kNoCompileOptions;

  // CompileFunction() takes the bare body + parameter names — no wrapper
  // string, and stack traces get correct line numbers.
  v8::Local<v8::Function> fn;
  if (!v8::ScriptCompiler::CompileFunction(
           context, &compiler_source, param_names.size(), param_names.data(), 0,
           nullptr, options)
           .ToLocal(&fn)) {
    return {};
  }

  // No usable cache (cold, or V8 rejected a stale blob): produce one and ship
  // it to the browser for persistence. SW realms have no ship-back channel.
  bool consumed = had_cache && compiler_source.GetCachedData() &&
                  !compiler_source.GetCachedData()->rejected;
  if (!consumed && render_frame) {
    std::unique_ptr<v8::ScriptCompiler::CachedData> produced(
        v8::ScriptCompiler::CreateCodeCacheForFunction(fn));
    if (produced && produced->length > 0) {
      mojo::AssociatedRemote<mojom::ElectronWebContentsUtility> remote;
      render_frame->GetRemoteAssociatedInterfaces()->GetInterface(&remote);
      // Hash of the exact source compiled — the browser only serves the
      // blob back for matching contents.
      std::array<uint8_t, crypto::hash::kSha256Size> source_hash =
          crypto::hash::Sha256(contents);
      // SAFETY: produced->data points to produced->length contiguous bytes
      // owned by the CachedData.
      remote->SetPreloadCodeCache(
          script_id,
          std::vector<uint8_t>(source_hash.begin(), source_hash.end()),
          mojo_base::BigBuffer(UNSAFE_BUFFERS(base::span(
              produced->data, base::checked_cast<size_t>(produced->length)))));
    }
  }
  return fn;
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
    entry.Set("filePath", ps->file_path);
    // The contents are not marshaled to V8 — createPreloadScript() looks them
    // up from the mojo-cached startup data by id, avoiding a ~150 KB heap
    // allocation per preload per navigation. JS only needs to know whether
    // there is anything to run.
    base::span<const uint8_t> bytes = ps->contents;
    entry.Set("hasContents", !bytes.empty());
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
