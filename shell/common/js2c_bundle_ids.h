// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_JS2C_BUNDLE_IDS_H_
#define ELECTRON_SHELL_COMMON_JS2C_BUNDLE_IDS_H_

#include <array>
#include <string_view>

#include "v8/include/v8-container.h"
#include "v8/include/v8-isolate.h"
#include "v8/include/v8-primitive.h"

// The electron/js2c/* framework bundle ids and the parameter names each
// bundle's wrapper function is compiled with. The ids and params feed V8's
// code-cache key (source + params hash), so the build-time codecache
// generator (electron_natives_codecache_main.cc) must compile each bundle
// with the exact same params its runtime CompileAndCall site uses -- this
// header keeps both sides in sync. spec/api-js2c-code-cache-spec.ts also
// asserts every bundle's cache is consumed, catching any drift.
namespace electron::js2c {

inline constexpr char kSandboxBundleId[] = "electron/js2c/sandbox_bundle";
inline constexpr char kIsolatedBundleId[] = "electron/js2c/isolated_bundle";
inline constexpr char kPreloadRealmBundleId[] =
    "electron/js2c/preload_realm_bundle";
inline constexpr char kNodeInitId[] = "electron/js2c/node_init";
inline constexpr char kBrowserInitId[] = "electron/js2c/browser_init";
inline constexpr char kRendererInitId[] = "electron/js2c/renderer_init";
inline constexpr char kUtilityInitId[] = "electron/js2c/utility_init";
inline constexpr char kWorkerInitId[] = "electron/js2c/worker_init";

// Wrapper function parameter names for the bundles compiled via
// util::CompileAndCall. The *_init bundles run by node::LoadEnvironment use
// the BuiltinLoader's parameter map, not these.
inline constexpr std::array<std::string_view, 1> kSandboxBundleParams = {
    "binding"};
inline constexpr std::array<std::string_view, 1> kIsolatedBundleParams = {
    "isolatedApi"};
inline constexpr std::array<std::string_view, 1> kPreloadRealmBundleParams = {
    "binding"};
inline constexpr std::array<std::string_view, 2> kNodeInitParams = {"process",
                                                                    "require"};

// Builds the `parameters` arg for util::CompileAndCall from one of the
// constexpr param arrays above.
template <size_t N>
v8::LocalVector<v8::String> MakeBundleParams(
    v8::Isolate* isolate,
    const std::array<std::string_view, N>& params) {
  v8::LocalVector<v8::String> local_params(isolate);
  local_params.reserve(N);
  for (std::string_view param : params) {
    local_params.push_back(
        v8::String::NewFromUtf8(isolate, param.data(),
                                v8::NewStringType::kInternalized,
                                static_cast<int>(param.size()))
            .ToLocalChecked());
  }
  return local_params;
}

}  // namespace electron::js2c

#endif  // ELECTRON_SHELL_COMMON_JS2C_BUNDLE_IDS_H_
