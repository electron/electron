// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include "base/compiler_specific.h"
#include "base/strings/string_util.h"
#include "build/build_config.h"
#include "shell/common/asar/archive.h"
#include "shell/common/asar/asar_util.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"

#if BUILDFLAG(IS_WIN)
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#else
#include <fcntl.h>

#include "base/posix/eintr_wrapper.h"
#endif

namespace {

// Hands |fn| the UTF-8 bytes of |value|. Archive-relative paths are almost
// always ASCII, and for those the flat one-byte string is used in place --
// no copy, no allocation -- which matters because this sits under every fs
// call on a packed path. |fn| must not allocate on the V8 heap.
template <typename Fn>
bool WithUtf8Path(v8::Isolate* isolate, v8::Local<v8::Value> value, Fn&& fn) {
  if (!value->IsString())
    return false;
  {
    v8::String::ValueView view(isolate, value.As<v8::String>());
    if (view.is_one_byte()) {
      // SAFETY: ValueView guarantees data8() points at length() bytes of the
      // flattened string for the lifetime of |view|.
      const std::string_view latin1 = UNSAFE_BUFFERS(std::string_view(
          reinterpret_cast<const char*>(view.data8()), view.length()));
      if (base::IsStringASCII(latin1)) {
        fn(latin1, /*is_ascii=*/true);
        return true;
      }
    }
  }
  std::string utf8;
  if (!gin::ConvertFromV8(isolate, value, &utf8))
    return false;
  fn(std::string_view(utf8), /*is_ascii=*/false);
  return true;
}

// Number of UTF-16 code units (what JS string offsets count) that the first
// |byte_length| bytes of the UTF-8 |utf8| decode to.
int Utf16LengthOfUtf8Prefix(std::string_view utf8, size_t byte_length) {
  int units = 0;
  for (unsigned char c : utf8.substr(0, byte_length)) {
    if ((c & 0xC0) == 0x80)
      continue;  // continuation byte
    units +=
        (c & 0xF8) == 0xF0 ? 2 : 1;  // 4-byte sequences are surrogate pairs
  }
  return units;
}

// Archive offsets and sizes are nearly always Smi-sized; avoid a HeapNumber.
v8::Local<v8::Value> ToV8Size(v8::Isolate* isolate, uint64_t value) {
  if (value <= static_cast<uint64_t>(std::numeric_limits<int32_t>::max()))
    return v8::Integer::New(isolate, static_cast<int32_t>(value));
  return v8::Number::New(isolate, static_cast<double>(value));
}

class Archive : public node::ObjectWrap {
 public:
  static v8::Local<v8::FunctionTemplate> CreateFunctionTemplate(
      v8::Isolate* isolate) {
    auto tpl = v8::FunctionTemplate::New(isolate, Archive::New);
    tpl->SetClassName(v8::String::NewFromUtf8Literal(isolate, "Archive"));
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    NODE_SET_PROTOTYPE_METHOD(tpl, "getFileInfo", &Archive::GetFileInfo);
    NODE_SET_PROTOTYPE_METHOD(tpl, "stat", &Archive::Stat);
    NODE_SET_PROTOTYPE_METHOD(tpl, "readdir", &Archive::Readdir);
    NODE_SET_PROTOTYPE_METHOD(tpl, "readdirWithTypes",
                              &Archive::ReaddirWithTypes);
    NODE_SET_PROTOTYPE_METHOD(tpl, "realpath", &Archive::Realpath);
    NODE_SET_PROTOTYPE_METHOD(tpl, "copyFileOut", &Archive::CopyFileOut);
    NODE_SET_PROTOTYPE_METHOD(tpl, "getFdAndValidateIntegrityLater",
                              &Archive::GetFD);

    return tpl;
  }

  // disable copy
  Archive(const Archive&) = delete;
  Archive& operator=(const Archive&) = delete;

 protected:
  Archive(v8::Isolate* isolate, std::shared_ptr<asar::Archive> archive)
      : archive_(std::move(archive)) {
    // stat() and getFileInfo() sit under every fs call on a packed path, so
    // their results are stamped out from a v8::DictionaryTemplate (one shared
    // hidden class, no per-call property-name strings) instead of being built
    // key by key.
    static constexpr std::string_view kStatKeys[] = {"size", "offset", "type",
                                                     "executable"};
    static constexpr std::string_view kFileInfoKeys[] = {
        "size", "unpacked", "offset", "executable", "integrity"};
    stat_template_.Reset(isolate,
                         v8::DictionaryTemplate::New(isolate, kStatKeys));
    file_info_template_.Reset(
        isolate, v8::DictionaryTemplate::New(isolate, kFileInfoKeys));
  }

  static void New(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto* isolate = args.GetIsolate();

    base::FilePath path;
    if (!gin::ConvertFromV8(isolate, args[0], &path)) {
      isolate->ThrowException(v8::Exception::Error(node::FIXED_ONE_BYTE_STRING(
          isolate, "failed to convert path to V8")));
      return;
    }

    std::shared_ptr<asar::Archive> archive = asar::GetOrCreateAsarArchive(path);
    if (!archive) {
      isolate->ThrowException(v8::Exception::Error(node::FIXED_ONE_BYTE_STRING(
          isolate, "failed to initialize archive")));
      return;
    }

    auto* archive_wrap = new Archive(isolate, std::move(archive));
    archive_wrap->Wrap(args.This());
    args.GetReturnValue().Set(args.This());
  }

  // Reads the offset and size of file.
  static void GetFileInfo(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto* isolate = args.GetIsolate();
    auto* wrap = node::ObjectWrap::Unwrap<Archive>(args.This());

    asar::Archive::FileInfo info;
    bool found = false;
    if (!wrap->archive_ ||
        !WithUtf8Path(isolate, args[0],
                      [&](std::string_view path, bool) {
                        found = wrap->archive_->GetFileInfo(path, &info);
                      }) ||
        !found) {
      args.GetReturnValue().Set(v8::False(isolate));
      return;
    }

    v8::MaybeLocal<v8::Value> integrity_value;
    if (info.integrity.has_value()) {
      const asar::IntegrityPayload& payload = info.integrity.value();
      gin_helper::Dictionary integrity(isolate, v8::Object::New(isolate));
      switch (payload.algorithm) {
        case asar::HashAlgorithm::kSHA256:
          integrity.Set("algorithm", "SHA256");
          break;
        case asar::HashAlgorithm::kNone:
          NOTREACHED();
      }
      integrity.Set("hash", payload.hash);
      integrity.Set("blockSize", payload.block_size);
      integrity.Set("blocks", payload.blocks);
      integrity_value = integrity.GetHandle();
    }
    v8::MaybeLocal<v8::Value> values[] = {
        ToV8Size(isolate, info.size), v8::Boolean::New(isolate, info.unpacked),
        ToV8Size(isolate, info.offset),
        v8::Boolean::New(isolate, info.executable), integrity_value};
    args.GetReturnValue().Set(
        wrap->file_info_template_.Get(isolate)->NewInstance(
            isolate->GetCurrentContext(), values));
  }

  // Returns a fake result of fs.stat(path).
  static void Stat(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto* isolate = args.GetIsolate();
    auto* wrap = node::ObjectWrap::Unwrap<Archive>(args.This());
    asar::Archive::Stats stats;
    bool found = false;
    if (!wrap->archive_ ||
        !WithUtf8Path(isolate, args[0],
                      [&](std::string_view path, bool) {
                        found = wrap->archive_->Stat(path, &stats);
                      }) ||
        !found) {
      args.GetReturnValue().Set(v8::False(isolate));
      return;
    }

    v8::MaybeLocal<v8::Value> values[] = {
        ToV8Size(isolate, stats.size), ToV8Size(isolate, stats.offset),
        v8::Integer::New(isolate, static_cast<int>(stats.type)),
        v8::Boolean::New(isolate, stats.executable)};
    args.GetReturnValue().Set(wrap->stat_template_.Get(isolate)->NewInstance(
        isolate->GetCurrentContext(), values));
  }

  static v8::Local<v8::Array> NamesToV8(v8::Isolate* isolate,
                                        const std::vector<std::string>& names) {
    v8::LocalVector<v8::Value> elements(isolate);
    elements.reserve(names.size());
    for (const std::string& name : names) {
      elements.push_back(v8::String::NewFromUtf8(isolate, name.data(),
                                                 v8::NewStringType::kNormal,
                                                 static_cast<int>(name.size()))
                             .ToLocalChecked());
    }
    return v8::Array::New(isolate, elements.data(), elements.size());
  }

  // Returns all files under a directory.
  static void Readdir(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto* isolate = args.GetIsolate();
    auto* wrap = node::ObjectWrap::Unwrap<Archive>(args.This());
    std::vector<std::string> names;
    bool found = false;
    if (!wrap->archive_ ||
        !WithUtf8Path(isolate, args[0],
                      [&](std::string_view path, bool) {
                        found = wrap->archive_->Readdir(path, &names, nullptr);
                      }) ||
        !found) {
      args.GetReturnValue().Set(v8::False(isolate));
      return;
    }
    args.GetReturnValue().Set(NamesToV8(isolate, names));
  }

  // Returns [names, types] for a directory, the shape node's own
  // fs binding produces for readdir(withFileTypes).
  static void ReaddirWithTypes(
      const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto* isolate = args.GetIsolate();
    auto* wrap = node::ObjectWrap::Unwrap<Archive>(args.This());
    std::vector<std::string> names;
    std::vector<asar::Archive::FileType> types;
    bool found = false;
    if (!wrap->archive_ ||
        !WithUtf8Path(isolate, args[0],
                      [&](std::string_view path, bool) {
                        found = wrap->archive_->Readdir(path, &names, &types);
                      }) ||
        !found) {
      args.GetReturnValue().Set(v8::False(isolate));
      return;
    }
    v8::LocalVector<v8::Value> type_elements(isolate);
    type_elements.reserve(types.size());
    for (asar::Archive::FileType type : types)
      type_elements.push_back(
          v8::Integer::New(isolate, static_cast<int>(type)));
    v8::Local<v8::Value> result[] = {
        NamesToV8(isolate, names),
        v8::Array::New(isolate, type_elements.data(), type_elements.size())};
    args.GetReturnValue().Set(v8::Array::New(isolate, result, 2));
  }

  // Returns the path of file with symbol link resolved.
  static void Realpath(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto* isolate = args.GetIsolate();
    auto* wrap = node::ObjectWrap::Unwrap<Archive>(args.This());
    std::string realpath;
    bool found = false;
    if (!wrap->archive_ ||
        !WithUtf8Path(isolate, args[0],
                      [&](std::string_view path, bool) {
                        found = wrap->archive_->Realpath(path, &realpath);
                      }) ||
        !found) {
      args.GetReturnValue().Set(v8::False(isolate));
      return;
    }
    args.GetReturnValue().Set(gin::ConvertToV8(isolate, realpath));
  }

  // Copy the file out into a temporary file and returns the new path.
  static void CopyFileOut(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto* isolate = args.GetIsolate();
    auto* wrap = node::ObjectWrap::Unwrap<Archive>(args.This());
    base::FilePath path;
    if (!gin::ConvertFromV8(isolate, args[0], &path)) {
      args.GetReturnValue().Set(v8::False(isolate));
      return;
    }

    base::FilePath new_path;
    if (!wrap->archive_ || !wrap->archive_->CopyFileOut(path, &new_path)) {
      args.GetReturnValue().Set(v8::False(isolate));
      return;
    }
    args.GetReturnValue().Set(gin::ConvertToV8(isolate, new_path));
  }

  // Return the file descriptor.
  static void GetFD(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto* isolate = args.GetIsolate();
    auto* wrap = node::ObjectWrap::Unwrap<Archive>(args.This());

    args.GetReturnValue().Set(gin::ConvertToV8(
        isolate, wrap->archive_ ? wrap->archive_->GetUnsafeFD() : -1));
  }

  std::shared_ptr<asar::Archive> archive_;
  v8::Global<v8::DictionaryTemplate> stat_template_;
  v8::Global<v8::DictionaryTemplate> file_info_template_;
};

// Returns a new, caller-owned, non-inheritable file descriptor that refers
// to the null device opened write-only, or -1 on failure.
//
// The fs wrapper hands one of these out for every open() of a packed entry
// and keeps the mapping from it to the entry on the JavaScript side, so the
// descriptor a caller sees is only meaningful through Node's fs module. Code
// that reads the raw descriptor itself (a native addon, a child's stdio, a
// socket, ...) fails with EBADF instead of being handed bytes of the archive
// at the wrong offset, and no handle to the archive itself ever escapes.
static void CreateSentinelFd(const v8::FunctionCallbackInfo<v8::Value>& args) {
  auto* isolate = args.GetIsolate();
#if BUILDFLAG(IS_WIN)
  static const HANDLE null_device =
      ::CreateFileW(L"NUL", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                    nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  int fd = -1;
  if (null_device != INVALID_HANDLE_VALUE) {
    HANDLE handle = nullptr;
    if (::DuplicateHandle(::GetCurrentProcess(), null_device,
                          ::GetCurrentProcess(), &handle, 0,
                          /*bInheritHandle=*/FALSE, DUPLICATE_SAME_ACCESS)) {
      fd = _open_osfhandle(reinterpret_cast<intptr_t>(handle), _O_WRONLY);
      if (fd == -1)
        ::CloseHandle(handle);
    }
  }
#else
  static const int null_device =
      HANDLE_EINTR(open("/dev/null", O_WRONLY | O_CLOEXEC));
  int fd = -1;
  if (null_device >= 0)
    fd = HANDLE_EINTR(fcntl(null_device, F_DUPFD_CLOEXEC, 0));
#endif
  args.GetReturnValue().Set(gin::ConvertToV8(isolate, fd));
}

// splitPath(path, requireNormalized) -> archive prefix length, or
// asar::kNotInArchive / asar::kNeedsNormalization. See
// asar::FindArchivePrefixLength().
static void SplitPath(const v8::FunctionCallbackInfo<v8::Value>& args) {
  auto* isolate = args.GetIsolate();
  const bool require_normalized = args[1]->IsTrue();
  int result = asar::kNotInArchive;
  WithUtf8Path(isolate, args[0], [&](std::string_view path, bool is_ascii) {
    result = asar::FindArchivePrefixLength(path, require_normalized);
    // The caller slices a JS string with this, so report UTF-16 code units.
    if (result > 0 && !is_ascii)
      result = Utf16LengthOfUtf8Prefix(path, result);
  });
  args.GetReturnValue().Set(result);
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = v8::Isolate::GetCurrent();

  auto cons = Archive::CreateFunctionTemplate(isolate)
                  ->GetFunction(context)
                  .ToLocalChecked();
  cons->SetName(node::FIXED_ONE_BYTE_STRING(isolate, "Archive"));

  exports->Set(context, node::FIXED_ONE_BYTE_STRING(isolate, "Archive"), cons)
      .Check();
  NODE_SET_METHOD(exports, "splitPath", &SplitPath);
  NODE_SET_METHOD(exports, "createSentinelFd", &CreateSentinelFd);
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_common_asar, Initialize)
