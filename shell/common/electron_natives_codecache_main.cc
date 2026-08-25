// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.
//
// Build-time host tool that emits the V8 code cache for the embedded
// electron/js2c/* bundles -- and, for flavors whose processes host a Node.js
// environment that is not deserialized from the embedded Node startup
// snapshot, Node's own lib/**/*.js builtins -- for one process flavor, as a
// generated .cc defining electron::internal::Js2cCache<Flavor>(). Run once per
// flavor with that flavor's V8 flag set and snapshot blob (both feed V8's
// code-cache key), compiling each bundle through the same BuiltinLoader path
// its runtime consumer uses so the source/params hash matches by construction.

// Host build tool, not shipped code.
#ifdef UNSAFE_BUFFERS_BUILD
#pragma allow_unsafe_buffers
#endif

#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "libplatform/libplatform.h"
#include "node_builtins.h"
#include "node_snapshot_builder.h"
#include "shell/common/js2c_bundle_ids.h"
#include "v8.h"

namespace {

namespace js2c = electron::js2c;

// `four_arg` = compiled via util::CompileAndCall (4-arg
// LookupAndCompileFunction with explicit `params`); else via
// LoadEnvironment's 3-arg path (BuiltinInfo::parameter_map).
struct Bundle {
  const char* id;
  bool four_arg;
  std::span<const std::string_view> params;
};

// Only the bundles each flavor's processes actually compile. On builds that
// embed a Node startup snapshot the browser flavor is keyed to the isolate that
// snapshot produces and serves every process created from it, which includes
// the node service's utility process.
std::vector<Bundle> BundlesForFlavor(std::string_view flavor,
                                     bool from_node_snapshot) {
  const Bundle kSandbox{js2c::kSandboxBundleId, true,
                        js2c::kSandboxBundleParams};
  const Bundle kIsolated{js2c::kIsolatedBundleId, true,
                         js2c::kIsolatedBundleParams};
  const Bundle kPreloadRealm{js2c::kPreloadRealmBundleId, true,
                             js2c::kPreloadRealmBundleParams};
  const Bundle kNodeInit{js2c::kNodeInitId, true, js2c::kNodeInitParams};
  const Bundle kBrowserInit{js2c::kBrowserInitId, false, {}};
  const Bundle kRendererInit{js2c::kRendererInitId, false, {}};
  const Bundle kUtilityInit{js2c::kUtilityInitId, false, {}};
  const Bundle kWorkerInit{js2c::kWorkerInitId, false, {}};

  if (flavor == "sandbox")
    return {kSandbox, kIsolated, kPreloadRealm};
  if (flavor == "renderer")
    return {kRendererInit, kNodeInit, kIsolated};
  if (flavor == "browser" && from_node_snapshot)
    return {kBrowserInit, kUtilityInit, kNodeInit};
  if (flavor == "browser")
    return {kBrowserInit, kNodeInit};
  if (flavor == "utility")
    return {kUtilityInit, kNodeInit};
  if (flavor == "worker")
    return {kWorkerInit, kNodeInit};
  return {};
}

const char* FlavorFn(std::string_view flavor) {
  if (flavor == "sandbox")
    return "Js2cCacheSandbox";
  if (flavor == "renderer")
    return "Js2cCacheRenderer";
  if (flavor == "browser")
    return "Js2cCacheBrowser";
  if (flavor == "utility")
    return "Js2cCacheUtility";
  if (flavor == "worker")
    return "Js2cCacheWorker";
  return nullptr;
}

// Node builtins left out of the cache: internal/deps/* are large bundled
// third-party libraries (undici, acorn, amaro, minimatch, ...) that no
// process type loads during bootstrap -- only on demand from specific APIs --
// and would roughly double the per-flavor cache (~5.5 of ~12 MB) for no
// startup benefit. They keep compiling from source on first use, as before.
bool ShouldCacheNodeBuiltin(std::string_view id) {
  constexpr std::string_view kJs2cPrefix = "electron/js2c/";
  constexpr std::string_view kDepsPrefix = "internal/deps/";
  return id.substr(0, kJs2cPrefix.size()) != kJs2cPrefix &&
         id.substr(0, kDepsPrefix.size()) != kDepsPrefix;
}

void EmitBytes(std::ofstream& out,
               const std::string& name,
               std::string_view bytes) {
  out << "static const uint8_t " << name << "[] = {";
  std::size_t i = 0;
  for (char c : bytes) {
    if (i++ % 32 == 0)
      out << "\n  ";
    out << static_cast<unsigned>(static_cast<unsigned char>(c)) << ",";
  }
  out << "\n};\n";
}

}  // namespace

int main(int argc, char* argv[]) {
  std::vector<std::string> args;
  args.reserve(static_cast<std::size_t>(argc));
  for (int i = 0; i < argc; ++i)
    args.emplace_back(argv[i]);
  if (args.size() < 6) {
    std::cerr << "usage: electron_natives_codecache <out.cc> <snapshot_blob> "
                 "<flavor> <v8_flags> <node_builtins:0|1>\n";
    return 1;
  }
  const std::string out_path = args[1];
  const std::string snapshot_blob_path = args[2];
  const std::string flavor = args[3];
  const std::string v8_flags = args[4];
  // Whether processes of this flavor bootstrap a node::Environment from
  // scratch on this build, and so compile Node's internal builtins
  // (internal/bootstrap/*, everything the bootstrap and the flavor's init
  // bundle require, and whatever app code requires later) rather than
  // deserialize them from the embedded Node startup snapshot. Decided in GN:
  // always for the renderer flavor (Node-enabled web workers live in renderer
  // processes and consume it too), for utility/browser only on builds without
  // a Node snapshot, never for sandbox (no Node environment) or worker.
  const bool with_node_builtins = args[5] == "1";

  const char* flavor_fn = FlavorFn(flavor);
  if (!flavor_fn) {
    std::cerr << "unknown flavor: " << flavor << "\n";
    return 1;
  }

  // The blob the consuming process's isolate is created from -- the cache
  // embeds its read-only checksum.
  v8::V8::InitializeICUDefaultLocation(argv[0]);
  v8::V8::InitializeExternalStartupDataFromFile(snapshot_blob_path.c_str());

  // Match the consuming process's non-default flag set (FlagList::Hash is
  // part of V8's code-cache key). Passed in from GN per flavor.
  if (!v8_flags.empty())
    v8::V8::SetFlagsFromString(v8_flags.c_str(), v8_flags.size());

  std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
  v8::V8::InitializePlatform(platform.get());
  v8::V8::Initialize();

  std::vector<std::pair<std::string, std::vector<uint8_t>>> caches;
  v8::Isolate::CreateParams create_params;
  create_params.array_buffer_allocator =
      v8::ArrayBuffer::Allocator::NewDefaultAllocator();

  // Electron: when this tool is linked with the real node_snapshot (the
  // browser flavor), create the isolate FROM the embedded Node startup
  // snapshot -- the browser process boots from that same snapshot, and the
  // V8 code-cache key embeds the isolate's read-only-heap checksum, which a
  // SnapshotCreator-produced snapshot owns uniquely (it differs from every
  // standalone blob). Building the cache here, against the shipped snapshot,
  // is the only way its read-only checksum matches the runtime browser
  // isolate. Other flavors link the weak stub (null here) and create their
  // isolate from the external startup blob loaded above.
  const node::SnapshotData* node_sd =
      node::SnapshotBuilder::GetEmbeddedSnapshotData();
  const bool from_node_snapshot = node_sd != nullptr;
  if (from_node_snapshot)
    node::SnapshotBuilder::InitializeIsolateParams(node_sd, &create_params);

  v8::Isolate* isolate = v8::Isolate::New(create_params);
  {
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);
    // From a Node snapshot isolate the main context is at index 0; otherwise
    // create a fresh one (matches each consuming process).
    v8::Local<v8::Context> context =
        from_node_snapshot
            ? v8::Context::FromSnapshot(isolate, 0).ToLocalChecked()
            : v8::Context::New(isolate);
    v8::Context::Scope context_scope(context);

    // The ctor self-registers all builtins, incl. electron/js2c/*.
    node::builtins::BuiltinLoader loader;
    // Eagerly compile every inner function, not just the top-level wrapper,
    // so the cache covers the whole bundle and the consuming process never
    // lazily compiles. The framework bundles run in full at startup, so the
    // lazy compiles happen anyway -- this just front-loads them.
    loader.SetEagerCompile();
    for (const auto& b : BundlesForFlavor(flavor, from_node_snapshot)) {
      v8::Local<v8::Function> fn;
      if (b.four_arg) {
        v8::LocalVector<v8::String> params(isolate);
        params.reserve(b.params.size());
        for (std::string_view p : b.params) {
          params.push_back(v8::String::NewFromUtf8(isolate, p.data(),
                                                   v8::NewStringType::kNormal,
                                                   static_cast<int>(p.size()))
                               .ToLocalChecked());
        }
        if (!loader
                 .LookupAndCompileFunction(context, b.id, &params,
                                           /*optional_realm=*/nullptr)
                 .ToLocal(&fn)) {
          std::cerr << "4-arg compile failed: " << b.id << "\n";
          return 1;
        }
      } else {
        if (!loader
                 .LookupAndCompileFunction(context, b.id,
                                           /*optional_realm=*/nullptr)
                 .ToLocal(&fn)) {
          std::cerr << "3-arg compile failed: " << b.id << "\n";
          return 1;
        }
      }
      std::unique_ptr<v8::ScriptCompiler::CachedData> cd(
          v8::ScriptCompiler::CreateCodeCacheForFunction(fn));
      if (!cd || cd->length == 0) {
        std::cerr << "no code cache produced: " << b.id << "\n";
        return 1;
      }
      caches.emplace_back(
          b.id, std::vector<uint8_t>(cd->data, cd->data + cd->length));
    }

    // Node's own builtins. A node::Environment deserialized from the embedded
    // Node startup snapshot gets a code cache for every builtin from the
    // snapshot blob (Environment's constructor feeds it), but an Environment
    // bootstrapped from scratch -- every renderer / web worker environment,
    // and utility / ELECTRON_RUN_AS_NODE / browser environments on builds
    // without a Node snapshot -- otherwise compiles the ~120-140 builtins its
    // bootstrap loads from source on every start (and then serializes a fresh
    // cache for each that nothing consumes). Emit a cache for them here, keyed
    // to this flavor's isolate, so the process-wide default cache
    // (util::InstallProcessCodeCache) covers the whole bootstrap.
    //
    // Uses a second loader without SetEagerCompile so the eagerness policy is
    // node_mksnapshot's: per_context/bootstrap/main scripts eagerly, the rest
    // top-level only (inner functions compile lazily on first call). The blob
    // is still ~2.4x the snapshot's cache for the same ids (~6.5 MB per
    // flavor): a cache built against a SnapshotCreator-produced isolate encodes
    // Node's internalized strings as read-only-heap references, one built
    // against the plain v8 context snapshot has to carry them.
    if (with_node_builtins && !from_node_snapshot) {
      node::builtins::BuiltinLoader node_loader;
      std::vector<node::builtins::CodeCacheInfo> node_caches;
      if (!node_loader.CompileAllBuiltinsAndCopyCodeCache(
              context, /*eager_builtins=*/{}, &node_caches)) {
        std::cerr << "compiling Node builtins failed\n";
        return 1;
      }
      for (const auto& info : node_caches) {
        // (The electron/js2c/* ids are registered as builtins too; the ones
        // this flavor needs were compiled above with their real parameters.)
        if (!ShouldCacheNodeBuiltin(info.id))
          continue;
        caches.emplace_back(
            info.id, std::vector<uint8_t>(info.data.data,
                                          info.data.data + info.data.length));
      }
    }
  }
  isolate->Dispose();
  v8::V8::Dispose();
  v8::V8::DisposePlatform();
  delete create_params.array_buffer_allocator;

  std::sort(caches.begin(), caches.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });

  std::ofstream out(out_path, std::ios::binary | std::ios::trunc);
  if (!out) {
    std::cerr << "cannot open " << out_path << "\n";
    return 1;
  }
  out << "// Generated by electron_natives_codecache (" << flavor
      << "). Do not edit.\n"
      << "#include \"shell/common/node_natives_code_cache_internal.h\"\n\n"
      << "#include \"base/no_destructor.h\"\n"
      << "#include \"shell/common/node_includes.h\"\n\n"
      << "namespace electron::internal {\n\n";
  for (std::size_t i = 0; i < caches.size(); ++i) {
    EmitBytes(
        out, "kJs2cCache" + std::to_string(i),
        std::string_view(reinterpret_cast<const char*>(caches[i].second.data()),
                         caches[i].second.size()));
  }
  out << "\nconst std::vector<node::builtins::CodeCacheInfo>& " << flavor_fn
      << "() {\n"
      << "  static const "
         "base::NoDestructor<std::vector<node::builtins::CodeCacheInfo>>\n"
      << "      kCache([] {\n"
      << "        std::vector<node::builtins::CodeCacheInfo> v;\n"
      << "        v.reserve(" << caches.size() << ");\n";
  for (std::size_t i = 0; i < caches.size(); ++i) {
    out << "        v.push_back({\"" << caches[i].first
        << "\", node::builtins::BuiltinCodeCacheData(kJs2cCache" << i
        << ", sizeof(kJs2cCache" << i << "))});\n";
  }
  out << "        return v;\n"
      << "      }());\n"
      << "  return *kCache;\n"
      << "}\n\n"
      << "}  // namespace electron::internal\n";
  out.close();

  std::size_t total_bytes = 0;
  for (const auto& c : caches)
    total_bytes += c.second.size();
  std::cerr << "electron_natives_codecache[" << flavor << "]: wrote "
            << caches.size() << " entries (" << total_bytes << " bytes) to "
            << out_path << "\n";
  return 0;
}
