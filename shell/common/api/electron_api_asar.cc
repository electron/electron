// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <vector>

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
  explicit Archive(std::shared_ptr<asar::Archive> archive)
      : archive_(std::move(archive)) {}

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

    auto* archive_wrap = new Archive(std::move(archive));
    archive_wrap->Wrap(args.This());
    args.GetReturnValue().Set(args.This());
  }

  // Reads the offset and size of file.
  static void GetFileInfo(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto* isolate = args.GetIsolate();
    auto* wrap = node::ObjectWrap::Unwrap<Archive>(args.This());

    base::FilePath path;
    if (!gin::ConvertFromV8(isolate, args[0], &path)) {
      args.GetReturnValue().Set(v8::False(isolate));
      return;
    }

    asar::Archive::FileInfo info;
    if (!wrap->archive_ || !wrap->archive_->GetFileInfo(path, &info)) {
      args.GetReturnValue().Set(v8::False(isolate));
      return;
    }

    gin_helper::Dictionary dict(isolate, v8::Object::New(isolate));
    dict.Set("size", info.size);
    dict.Set("unpacked", info.unpacked);
    dict.Set("offset", info.offset);
    dict.Set("executable", info.executable);
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
      dict.Set("integrity", integrity);
    }
    args.GetReturnValue().Set(dict.GetHandle());
  }

  // Returns a fake result of fs.stat(path).
  static void Stat(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto* isolate = args.GetIsolate();
    auto* wrap = node::ObjectWrap::Unwrap<Archive>(args.This());
    base::FilePath path;
    if (!gin::ConvertFromV8(isolate, args[0], &path)) {
      args.GetReturnValue().Set(v8::False(isolate));
      return;
    }

    asar::Archive::Stats stats;
    if (!wrap->archive_ || !wrap->archive_->Stat(path, &stats)) {
      args.GetReturnValue().Set(v8::False(isolate));
      return;
    }

    gin_helper::Dictionary dict(isolate, v8::Object::New(isolate));
    dict.Set("size", stats.size);
    dict.Set("offset", stats.offset);
    dict.Set("type", static_cast<int>(stats.type));
    dict.Set("executable", stats.executable);
    args.GetReturnValue().Set(dict.GetHandle());
  }

  // Returns all files under a directory.
  static void Readdir(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto* isolate = args.GetIsolate();
    auto* wrap = node::ObjectWrap::Unwrap<Archive>(args.This());
    base::FilePath path;
    if (!gin::ConvertFromV8(isolate, args[0], &path)) {
      args.GetReturnValue().Set(v8::False(isolate));
      return;
    }

    std::vector<base::FilePath> files;
    if (!wrap->archive_ || !wrap->archive_->Readdir(path, &files)) {
      args.GetReturnValue().Set(v8::False(isolate));
      return;
    }
    args.GetReturnValue().Set(gin::ConvertToV8(isolate, files));
  }

  // Returns the path of file with symbol link resolved.
  static void Realpath(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto* isolate = args.GetIsolate();
    auto* wrap = node::ObjectWrap::Unwrap<Archive>(args.This());
    base::FilePath path;
    if (!gin::ConvertFromV8(isolate, args[0], &path)) {
      args.GetReturnValue().Set(v8::False(isolate));
      return;
    }

    base::FilePath realpath;
    if (!wrap->archive_ || !wrap->archive_->Realpath(path, &realpath)) {
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

static void SplitPath(const v8::FunctionCallbackInfo<v8::Value>& args) {
  auto* isolate = args.GetIsolate();

  auto dict = gin_helper::Dictionary::CreateEmpty(isolate);
  args.GetReturnValue().Set(dict.GetHandle());

  base::FilePath path;
  if (!gin::ConvertFromV8(isolate, args[0], &path)) {
    dict.Set("isAsar", false);
    return;
  }

  base::FilePath asar_path, file_path;
  if (asar::GetAsarArchivePath(path, &asar_path, &file_path, true)) {
    dict.Set("isAsar", true);
    dict.Set("asarPath", asar_path);
    dict.Set("filePath", file_path);
  } else {
    dict.Set("isAsar", false);
  }
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
