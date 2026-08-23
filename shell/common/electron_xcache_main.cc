// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.
//
// electron_xcache: host tool that compiles a JavaScript file and emits a V8
// code cache (v8::ScriptCompiler::CachedData) that a target Electron build of
// the same version -- any OS / CPU -- accepts in the process created from the
// supplied V8 startup snapshot blob.
//
// A code cache is arch-neutral except for its header (version hash, flag hash,
// read-only-snapshot checksum) and ReadOnlyHeapRef back-references into the
// read-only heap, all of which are properties of the snapshot blob the
// consuming isolate was created from. The tool therefore creates its isolate
// from the target's blob, runs no JavaScript in it beyond context setup (the
// blob's builtin metadata does not describe this binary's builtins), and
// serializes. Build it from the target version's checkout in a release
// configuration.

#ifdef UNSAFE_BUFFERS_BUILD
#pragma allow_unsafe_buffers
#endif

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "libplatform/libplatform.h"
#include "third_party/zlib/zlib.h"
#include "v8.h"

namespace {

// Non-default V8 flags of the Electron main process; FlagList::Hash() over
// them is part of the code-cache key. Keep in sync with the browser flavor of
// electron_natives_codecache in BUILD.gn.
constexpr char kDefaultV8Flags[] =
    "--rehash-snapshot --no-freeze-flags-after-init";

// v8/src/snapshot/snapshot.cc, SnapshotImpl.
constexpr uint32_t kSnapNumContextsOffset = 0;
constexpr uint32_t kSnapRehashabilityOffset = 4;
constexpr uint32_t kSnapChecksumOffset = 8;
constexpr uint32_t kSnapRoChecksumOffset = 12;
constexpr uint32_t kSnapVersionStringOffset = 16;
constexpr uint32_t kSnapVersionStringLength = 64;
constexpr uint32_t kSnapStartupOffsetOffset =
    kSnapVersionStringOffset + kSnapVersionStringLength;
constexpr uint32_t kSnapFirstContextOffsetOffset =
    kSnapStartupOffsetOffset + 2 * 4;
// v8/src/snapshot/snapshot-data.h, SnapshotData: magic, payload length.
constexpr uint32_t kSnapshotDataHeaderSize = 8;

// v8/src/snapshot/code-serializer.h, SerializedCodeData (64-bit).
constexpr uint32_t kCacheMagicOffset = 0;
constexpr uint32_t kCacheVersionHashOffset = 4;
constexpr uint32_t kCacheSourceHashOffset = 8;
constexpr uint32_t kCacheFlagHashOffset = 12;
constexpr uint32_t kCacheRoChecksumOffset = 16;
constexpr uint32_t kCachePayloadLengthOffset = 20;
constexpr uint32_t kCacheHeaderSize = 32;

uint32_t ReadU32(const char* p) {
  uint32_t v;
  std::memcpy(&v, p, sizeof(v));
  return v;
}

std::string Hex(uint32_t v) {
  char buf[16];
  std::snprintf(buf, sizeof(buf), "0x%08x", v);
  return buf;
}

class MappedFile {
 public:
  bool Open(const std::string& path) {
    fd_ = open(path.c_str(), O_RDONLY);
    if (fd_ < 0)
      return false;
    struct stat st;
    if (fstat(fd_, &st) != 0 || st.st_size == 0)
      return false;
    size_ = static_cast<size_t>(st.st_size);
    void* m = mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, fd_, 0);
    if (m == MAP_FAILED)
      return false;
    data_ = static_cast<const char*>(m);
    return true;
  }
  ~MappedFile() {
    if (data_)
      munmap(const_cast<char*>(data_), size_);
    if (fd_ >= 0)
      close(fd_);
  }
  std::string_view view() const { return {data_, size_}; }

 private:
  int fd_ = -1;
  const char* data_ = nullptr;
  size_t size_ = 0;
};

bool ReadFile(const std::string& path, std::string* out) {
  std::ifstream in(path, std::ios::binary);
  if (!in)
    return false;
  std::ostringstream ss;
  ss << in.rdbuf();
  *out = ss.str();
  return true;
}

struct BlobInfo {
  size_t offset = 0;
  size_t size = 0;
  uint32_t num_contexts = 0;
  uint32_t ro_checksum = 0;
  std::string version;
  bool checksum_ok = false;
};

// Parses a SnapshotImpl header at data[offset]; the blob's length is derived
// from its last context's SnapshotData header.
bool ParseBlobAt(std::string_view data, size_t offset, BlobInfo* info) {
  if (offset + kSnapFirstContextOffsetOffset + 4 > data.size())
    return false;
  const char* p = data.data() + offset;
  BlobInfo b;
  b.offset = offset;
  b.num_contexts = ReadU32(p + kSnapNumContextsOffset);
  const uint32_t rehashability = ReadU32(p + kSnapRehashabilityOffset);
  if (b.num_contexts == 0 || b.num_contexts > 32 || rehashability > 1)
    return false;
  b.version.assign(
      p + kSnapVersionStringOffset,
      strnlen(p + kSnapVersionStringOffset, kSnapVersionStringLength));
  const size_t ctx_table_end =
      kSnapFirstContextOffsetOffset + 4 * static_cast<size_t>(b.num_contexts);
  if (offset + ctx_table_end > data.size())
    return false;
  const uint32_t startup_off = ReadU32(p + kSnapStartupOffsetOffset);
  const uint32_t last_ctx_off =
      ReadU32(p + kSnapFirstContextOffsetOffset + 4 * (b.num_contexts - 1));
  if (startup_off < ctx_table_end || last_ctx_off < startup_off ||
      offset + last_ctx_off + kSnapshotDataHeaderSize > data.size())
    return false;
  b.size = static_cast<size_t>(last_ctx_off) + kSnapshotDataHeaderSize +
           ReadU32(p + last_ctx_off + 4);
  if (offset + b.size > data.size())
    return false;
  // Covers [ro checksum field, end); V8 seeds adler32 with 0.
  const uLong a =
      adler32(0L, reinterpret_cast<const Bytef*>(p + kSnapRoChecksumOffset),
              static_cast<uInt>(b.size - kSnapRoChecksumOffset));
  b.checksum_ok = static_cast<uint32_t>(a) == ReadU32(p + kSnapChecksumOffset);
  b.ro_checksum = ReadU32(p + kSnapRoChecksumOffset);
  *info = b;
  return true;
}

// Every V8 snapshot blob for `version` in `data`: a bare blob file, or a
// binary that embeds one (the Node startup snapshot in electron / electron.exe
// / "Electron Framework").
std::vector<BlobInfo> FindBlobs(std::string_view data,
                                const std::string& version) {
  std::vector<BlobInfo> out;
  const std::string needle = version + '\0';
  size_t pos = 0;
  while ((pos = data.find(needle, pos)) != std::string_view::npos) {
    BlobInfo b;
    if (pos >= kSnapVersionStringOffset &&
        ParseBlobAt(data, pos - kSnapVersionStringOffset, &b)) {
      out.push_back(b);
    }
    ++pos;
  }
  return out;
}

const char* KindOf(const BlobInfo& b) {
  if (b.num_contexts >= 4)
    return "node-startup-snapshot";
  if (b.num_contexts == 3)
    return "v8_context_snapshot";
  if (b.num_contexts == 1)
    return "bare-v8-snapshot";
  return "unknown";
}

void PrintBlob(std::ostream& os, const BlobInfo& b, size_t index) {
  os << "  [" << index << "] offset=" << b.offset << " size=" << b.size
     << " contexts=" << b.num_contexts << " kind=" << KindOf(b)
     << " ro_checksum=" << Hex(b.ro_checksum)
     << " checksum=" << (b.checksum_ok ? "ok" : "MISMATCH") << " version=\""
     << b.version << "\"\n";
}

std::vector<std::string> Split(const std::string& s, char sep) {
  std::vector<std::string> out;
  std::string cur;
  for (char c : s) {
    if (c == sep) {
      if (!cur.empty())
        out.push_back(cur);
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  if (!cur.empty())
    out.push_back(cur);
  return out;
}

// The blob refers to its embedder's external references (Node's or Blink's C++
// callbacks and external string resources) by index into a table only that
// embedder has. Callbacks are never invoked here, but external strings are
// wired to their resource during deserialization, so every index must resolve
// to a string-resource-shaped object; nothing reads those strings. One-byte
// and two-byte resources declare data()/length() at the same vtable positions.
class InertStringResource final
    : public v8::String::ExternalOneByteStringResource {
 public:
  const char* data() const override { return kEmpty; }
  size_t length() const override { return 0; }
  bool IsCacheable() const override { return false; }

 protected:
  void Dispose() override {}

 private:
  static constexpr char kEmpty[1] = {0};
};

std::vector<intptr_t> MakeInertExternalReferences() {
  static InertStringResource resource;
  std::vector<intptr_t> refs(1 << 18, reinterpret_cast<intptr_t>(&resource));
  refs.push_back(0);
  return refs;
}

void Usage() {
  std::cerr
      << "usage: electron_xcache --snapshot <blob-or-binary> --in <file.js> "
         "--out <file> [options]\n"
         "       electron_xcache --snapshot <blob-or-binary> --list\n\n"
         "  --snapshot <file>   V8 startup snapshot the TARGET process is "
         "created "
         "from: a bare blob\n"
         "                      (v8_context_snapshot[.<arch>].bin) or a binary "
         "embedding one\n"
         "                      (electron, electron.exe, 'Electron Framework' "
         "-> "
         "its Node startup\n"
         "                      snapshot). Any OS/arch; must match this tool's "
         "V8 "
         "version.\n"
         "  --blob-index <n>    which blob when --snapshot holds several (see "
         "--list)\n"
         "  --list              print the blobs found in --snapshot and exit\n"
         "  --in <file.js>      source to compile\n"
         "  --out <file>        cache output path\n"
         "  --mode script|function|module   (default script)\n"
         "  --params a,b,c      parameters for --mode function\n"
         "                      (default exports,require,module,__filename,"
         "__dirname)\n"
         "  --eager             eagerly compile all inner functions\n"
         "  --filename <name>   script origin name (default: basename of "
         "--in)\n"
         "  --v8-flags \"...\"    non-default V8 flags of the consuming "
         "process\n"
         "                      (default \""
      << kDefaultV8Flags
      << "\")\n"
         "  --expect-flag-hash 0x..  fail unless the produced flag hash "
         "matches\n"
         "  --icu-data <file>   icudtl.dat (default: next to this binary)\n"
         "  --json              print a JSON summary to stdout\n";
}

}  // namespace

int main(int argc, char* argv[]) {
  std::string snapshot_path, in_path, out_path, filename, icu_data;
  std::string mode = "script";
  std::string v8_flags = kDefaultV8Flags;
  std::string extra_v8_flags;
  std::string params_csv = "exports,require,module,__filename,__dirname";
  int blob_index = -1;
  bool list = false, eager = false, json = false;
  int64_t expect_flag_hash = -1;

  for (int i = 1; i < argc; ++i) {
    const std::string_view a = argv[i];
    auto value = [&](std::string* dst) {
      if (i + 1 >= argc) {
        std::cerr << "missing value for " << a << "\n";
        exit(2);
      }
      *dst = argv[++i];
    };
    std::string tmp;
    if (a == "--snapshot") {
      value(&snapshot_path);
    } else if (a == "--in") {
      value(&in_path);
    } else if (a == "--out") {
      value(&out_path);
    } else if (a == "--mode") {
      value(&mode);
    } else if (a == "--params") {
      value(&params_csv);
    } else if (a == "--filename") {
      value(&filename);
    } else if (a == "--v8-flags") {
      value(&v8_flags);
    } else if (a == "--extra-v8-flags") {
      value(&extra_v8_flags);
    } else if (a == "--icu-data") {
      value(&icu_data);
    } else if (a == "--blob-index") {
      value(&tmp);
      blob_index = std::stoi(tmp);
    } else if (a == "--expect-flag-hash") {
      value(&tmp);
      expect_flag_hash = static_cast<int64_t>(std::stoul(tmp, nullptr, 0));
    } else if (a == "--list") {
      list = true;
    } else if (a == "--eager") {
      eager = true;
    } else if (a == "--json") {
      json = true;
    } else if (a == "--help" || a == "-h") {
      Usage();
      return 0;
    } else {
      std::cerr << "unknown argument: " << a << "\n";
      Usage();
      return 2;
    }
  }
  if (snapshot_path.empty() ||
      (!list && (in_path.empty() || out_path.empty()))) {
    Usage();
    return 2;
  }
  if (mode != "script" && mode != "function" && mode != "module") {
    std::cerr << "--mode must be script, function or module\n";
    return 2;
  }

  const std::string version = v8::V8::GetVersion();

  MappedFile container_file;
  if (!container_file.Open(snapshot_path)) {
    std::cerr << "cannot read " << snapshot_path << "\n";
    return 1;
  }
  const std::string_view container = container_file.view();
  const std::vector<BlobInfo> blobs = FindBlobs(container, version);
  if (list) {
    std::cout << "V8 " << version << "; snapshot blobs in " << snapshot_path
              << ":\n";
    for (size_t i = 0; i < blobs.size(); ++i)
      PrintBlob(std::cout, blobs[i], i);
  }
  if (blobs.empty()) {
    BlobInfo other;
    if (ParseBlobAt(container, 0, &other)) {
      std::cerr << snapshot_path << " is a snapshot for V8 \"" << other.version
                << "\"; this tool is V8 \"" << version << "\"\n";
    } else {
      std::cerr << "no V8 " << version << " snapshot blob found in "
                << snapshot_path << "\n";
    }
    return 1;
  }
  if (list)
    return 0;

  const BlobInfo* chosen = nullptr;
  if (blob_index >= 0) {
    if (static_cast<size_t>(blob_index) >= blobs.size()) {
      std::cerr << "--blob-index out of range (" << blobs.size() << " found)\n";
      return 1;
    }
    chosen = &blobs[blob_index];
  } else {
    // Default: the only Node startup snapshot if present, else the only blob.
    size_t node_count = 0;
    for (const BlobInfo& b : blobs) {
      if (b.num_contexts >= 4) {
        chosen = &b;
        ++node_count;
      }
    }
    if (node_count != 1)
      chosen = blobs.size() == 1 ? &blobs[0] : nullptr;
    if (!chosen) {
      std::cerr << "several candidate blobs; pick one with --blob-index:\n";
      for (size_t i = 0; i < blobs.size(); ++i)
        PrintBlob(std::cerr, blobs[i], i);
      return 1;
    }
  }
  if (!chosen->checksum_ok)
    std::cerr << "warning: blob checksum mismatch -- continuing\n";
  const BlobInfo chosen_info = *chosen;
  std::vector<uint64_t> blob_storage((chosen_info.size + 7) / 8);
  std::memcpy(blob_storage.data(), container.data() + chosen_info.offset,
              chosen_info.size);
  v8::StartupData blob{reinterpret_cast<const char*>(blob_storage.data()),
                       static_cast<int>(chosen_info.size)};

  std::string source;
  if (!ReadFile(in_path, &source)) {
    std::cerr << "cannot read " << in_path << "\n";
    return 1;
  }
  if (filename.empty()) {
    const size_t slash = in_path.find_last_of("/\\");
    filename = slash == std::string::npos ? in_path : in_path.substr(slash + 1);
  }

  if (!v8::V8::InitializeICUDefaultLocation(
          argv[0], icu_data.empty() ? nullptr : icu_data.c_str())) {
    std::cerr << "warning: ICU data not loaded; pass --icu-data <icudtl.dat> "
                 "if non-ASCII identifiers fail to parse\n";
  }
  if (!extra_v8_flags.empty())
    v8_flags += " " + extra_v8_flags;
  if (!v8_flags.empty())
    v8::V8::SetFlagsFromString(v8_flags.c_str(), v8_flags.size());
  // Deterministic output; --random-seed is excluded from FlagList::Hash().
  v8::V8::SetFlagsFromString("--random-seed=314159265");
  std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
  v8::V8::InitializePlatform(platform.get());
  v8::V8::SetSnapshotDataBlob(&blob);
  v8::V8::Initialize();

  v8::Isolate::CreateParams create_params;
  create_params.array_buffer_allocator =
      v8::ArrayBuffer::Allocator::NewDefaultAllocator();
  create_params.snapshot_blob = &blob;
  std::vector<intptr_t> external_references = MakeInertExternalReferences();
  create_params.external_references = external_references.data();
  v8::Isolate* isolate = v8::Isolate::New(create_params);

  std::vector<uint8_t> cache;
  int rc = 0;
  {
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);
    // Index 0 is a plain Context::New() in both the Node startup snapshot and
    // Blink's v8_context_snapshot. Flags that install extensions (--expose-gc)
    // run their setup script here; nothing may run after this point.
    v8::Local<v8::Context> context = v8::Context::New(isolate);
    v8::Context::Scope context_scope(context);
    v8::Isolate::DisallowJavascriptExecutionScope no_js(
        isolate,
        v8::Isolate::DisallowJavascriptExecutionScope::CRASH_ON_FAILURE);
    v8::TryCatch try_catch(isolate);

    v8::Local<v8::String> code;
    if (!v8::String::NewFromUtf8(isolate, source.data(),
                                 v8::NewStringType::kNormal,
                                 static_cast<int>(source.size()))
             .ToLocal(&code)) {
      std::cerr << "source is not valid UTF-8 or too large\n";
      return 1;
    }
    v8::Local<v8::String> name =
        v8::String::NewFromUtf8(isolate, filename.c_str()).ToLocalChecked();
    const bool is_module = mode == "module";
    v8::ScriptOrigin origin(name, 0, 0, false, -1, v8::Local<v8::Value>(),
                            false, false, is_module);
    const auto options = eager ? v8::ScriptCompiler::kEagerCompile
                               : v8::ScriptCompiler::kNoCompileOptions;
    v8::ScriptCompiler::Source src(code, origin);
    std::unique_ptr<v8::ScriptCompiler::CachedData> cd;
    if (mode == "script") {
      v8::Local<v8::UnboundScript> script;
      if (v8::ScriptCompiler::CompileUnboundScript(isolate, &src, options)
              .ToLocal(&script)) {
        cd.reset(v8::ScriptCompiler::CreateCodeCache(script));
      }
    } else if (mode == "function") {
      v8::LocalVector<v8::String> params(isolate);
      for (const std::string& p : Split(params_csv, ','))
        params.push_back(
            v8::String::NewFromUtf8(isolate, p.c_str()).ToLocalChecked());
      v8::Local<v8::Function> fn;
      if (v8::ScriptCompiler::CompileFunction(
              context, &src, params.size(), params.data(), 0, nullptr, options)
              .ToLocal(&fn)) {
        cd.reset(v8::ScriptCompiler::CreateCodeCacheForFunction(fn));
      }
    } else {
      v8::Local<v8::Module> module;
      if (v8::ScriptCompiler::CompileModule(isolate, &src, options)
              .ToLocal(&module)) {
        cd.reset(v8::ScriptCompiler::CreateCodeCache(
            module->GetUnboundModuleScript()));
      }
    }

    if (try_catch.HasCaught()) {
      v8::String::Utf8Value msg(isolate, try_catch.Exception());
      v8::Local<v8::Message> m = try_catch.Message();
      std::cerr << "compile error: " << (*msg ? *msg : "?");
      if (!m.IsEmpty())
        std::cerr << " (" << filename << ":"
                  << m->GetLineNumber(context).FromMaybe(0) << ")";
      std::cerr << "\n";
      rc = 3;
    } else if (!cd || cd->length < static_cast<int>(kCacheHeaderSize)) {
      std::cerr << "V8 produced no code cache\n";
      rc = 4;
    } else {
      cache.assign(cd->data, cd->data + cd->length);
    }
  }
  isolate->Dispose();
  v8::V8::Dispose();
  v8::V8::DisposePlatform();
  delete create_params.array_buffer_allocator;
  if (rc)
    return rc;

  const char* h = reinterpret_cast<const char*>(cache.data());
  const uint32_t flag_hash = ReadU32(h + kCacheFlagHashOffset);
  const uint32_t ro_checksum = ReadU32(h + kCacheRoChecksumOffset);
  if (ro_checksum != chosen_info.ro_checksum) {
    std::cerr << "internal error: cache ro checksum " << Hex(ro_checksum)
              << " != blob's " << Hex(chosen_info.ro_checksum) << "\n";
    return 5;
  }
  if (expect_flag_hash >= 0 &&
      static_cast<uint32_t>(expect_flag_hash) != flag_hash) {
    std::cerr << "flag hash " << Hex(flag_hash) << " != expected "
              << Hex(static_cast<uint32_t>(expect_flag_hash))
              << "; the consuming process runs with different non-default "
                 "V8 flags (see --v8-flags)\n";
    return 6;
  }
  {
    std::ofstream out(out_path, std::ios::binary | std::ios::trunc);
    if (!out || !out.write(h, static_cast<std::streamsize>(cache.size()))) {
      std::cerr << "cannot write " << out_path << "\n";
      return 1;
    }
  }
  if (json) {
    std::cout << "{\"v8\":\"" << version << "\",\"mode\":\"" << mode
              << "\",\"eager\":" << (eager ? "true" : "false")
              << ",\"flags\":\"" << v8_flags << "\",\"size\":" << cache.size()
              << ",\"header\":{\"magic\":\""
              << Hex(ReadU32(h + kCacheMagicOffset)) << "\",\"versionHash\":\""
              << Hex(ReadU32(h + kCacheVersionHashOffset))
              << "\",\"sourceHash\":\""
              << Hex(ReadU32(h + kCacheSourceHashOffset))
              << "\",\"flagHash\":\"" << Hex(flag_hash)
              << "\",\"roSnapshotChecksum\":\"" << Hex(ro_checksum)
              << "\",\"payloadLength\":"
              << ReadU32(h + kCachePayloadLengthOffset)
              << "},\"snapshot\":{\"file\":\"" << snapshot_path
              << "\",\"offset\":" << chosen_info.offset
              << ",\"size\":" << chosen_info.size
              << ",\"contexts\":" << chosen_info.num_contexts << ",\"kind\":\""
              << KindOf(chosen_info) << "\"},\"out\":\"" << out_path << "\"}\n";
  } else {
    std::cerr << "electron_xcache: " << out_path << " (" << cache.size()
              << " B, " << mode << (eager ? ", eager" : "")
              << ") v8=" << version << " flagHash=" << Hex(flag_hash)
              << " ro=" << Hex(ro_checksum) << " from " << KindOf(chosen_info)
              << "@" << snapshot_path << "+" << chosen_info.offset << "\n";
  }
  return 0;
}
