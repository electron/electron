// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <vector>

#include "atom_natives.h"  // NOLINT: This file is generated with coffee2c.
#include "atom/common/asar/archive.h"
#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "atom/common/node_includes.h"
#include "native_mate/arguments.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"
#include "native_mate/wrappable.h"

namespace {

class Archive : public mate::Wrappable<Archive> {
 public:
  static v8::Local<v8::Value> Create(v8::Isolate* isolate,
                                      const base::FilePath& path) {
    scoped_ptr<asar::Archive> archive(new asar::Archive(path));
    if (!archive->Init())
      return v8::False(isolate);
    return (new Archive(isolate, std::move(archive)))->GetWrapper();
  }

  static void BuildPrototype(
      v8::Isolate* isolate, v8::Local<v8::ObjectTemplate> prototype) {
    mate::ObjectTemplateBuilder(isolate, prototype)
        .SetProperty("path", &Archive::GetPath)
        .SetMethod("getFileInfo", &Archive::GetFileInfo)
        .SetMethod("stat", &Archive::Stat)
        .SetMethod("readdir", &Archive::Readdir)
        .SetMethod("realpath", &Archive::Realpath)
        .SetMethod("copyFileOut", &Archive::CopyFileOut)
        .SetMethod("getFd", &Archive::GetFD)
        .SetMethod("destroy", &Archive::Destroy);
  }

 protected:
  Archive(v8::Isolate* isolate, scoped_ptr<asar::Archive> archive)
      : archive_(std::move(archive)) {
    Init(isolate);
  }

  // Returns the path of the file.
  base::FilePath GetPath() {
    return archive_->path();
  }

  // Reads the offset and size of file.
  v8::Local<v8::Value> GetFileInfo(v8::Isolate* isolate,
                                    const base::FilePath& path) {
    asar::Archive::FileInfo info;
    if (!archive_ || !archive_->GetFileInfo(path, &info))
      return v8::False(isolate);
    mate::Dictionary dict(isolate, v8::Object::New(isolate));
    dict.Set("size", info.size);
    dict.Set("unpacked", info.unpacked);
    dict.Set("offset", info.offset);
    return dict.GetHandle();
  }

  // Returns a fake result of fs.stat(path).
  v8::Local<v8::Value> Stat(v8::Isolate* isolate,
                             const base::FilePath& path) {
    asar::Archive::Stats stats;
    if (!archive_ || !archive_->Stat(path, &stats))
      return v8::False(isolate);
    mate::Dictionary dict(isolate, v8::Object::New(isolate));
    dict.Set("size", stats.size);
    dict.Set("offset", stats.offset);
    dict.Set("isFile", stats.is_file);
    dict.Set("isDirectory", stats.is_directory);
    dict.Set("isLink", stats.is_link);
    return dict.GetHandle();
  }

  // Returns all files under a directory.
  v8::Local<v8::Value> Readdir(v8::Isolate* isolate,
                                const base::FilePath& path) {
    std::vector<base::FilePath> files;
    if (!archive_ || !archive_->Readdir(path, &files))
      return v8::False(isolate);
    return mate::ConvertToV8(isolate, files);
  }

  // Returns the path of file with symbol link resolved.
  v8::Local<v8::Value> Realpath(v8::Isolate* isolate,
                                 const base::FilePath& path) {
    base::FilePath realpath;
    if (!archive_ || !archive_->Realpath(path, &realpath))
      return v8::False(isolate);
    return mate::ConvertToV8(isolate, realpath);
  }

  // Copy the file out into a temporary file and returns the new path.
  v8::Local<v8::Value> CopyFileOut(v8::Isolate* isolate,
                                    const base::FilePath& path) {
    base::FilePath new_path;
    if (!archive_ || !archive_->CopyFileOut(path, &new_path))
      return v8::False(isolate);
    return mate::ConvertToV8(isolate, new_path);
  }

  // Return the file descriptor.
  int GetFD() const {
    if (!archive_)
      return -1;
    return archive_->GetFD();
  }

  // Free the resources used by archive.
  void Destroy() {
    archive_.reset();
  }

 private:
  scoped_ptr<asar::Archive> archive_;

  DISALLOW_COPY_AND_ASSIGN(Archive);
};

void InitAsarSupport(v8::Isolate* isolate,
                     v8::Local<v8::Value> process,
                     v8::Local<v8::Value> require) {
  // Evaluate asar_init.coffee.
  const char* asar_init_native = reinterpret_cast<const char*>(
      static_cast<const unsigned char*>(node::asar_init_native));
  v8::Local<v8::Script> asar_init = v8::Script::Compile(v8::String::NewFromUtf8(
      isolate,
      asar_init_native,
      v8::String::kNormalString,
      sizeof(node::asar_init_native) -1));
  v8::Local<v8::Value> result = asar_init->Run();

  // Initialize asar support.
  base::Callback<void(v8::Local<v8::Value>,
                      v8::Local<v8::Value>,
                      std::string)> init;
  if (mate::ConvertFromV8(isolate, result, &init)) {
    const char* asar_native = reinterpret_cast<const char*>(
        static_cast<const unsigned char*>(node::asar_native));
    init.Run(process,
             require,
             std::string(asar_native, sizeof(node::asar_native) - 1));
  }
}

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  mate::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("createArchive", &Archive::Create);
  dict.SetMethod("initAsarSupport", &InitAsarSupport);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_common_asar, Initialize)
