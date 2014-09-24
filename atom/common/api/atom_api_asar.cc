// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/asar/archive.h"
#include "atom/common/asar/archive_factory.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "native_mate/arguments.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"
#include "native_mate/wrappable.h"

#include "atom/common/node_includes.h"

namespace {

class Archive : public mate::Wrappable {
 public:
  static v8::Handle<v8::Value> Create(mate::Arguments* args,
                                      const base::FilePath& path) {
    static asar::ArchiveFactory archive_factory;
    scoped_refptr<asar::Archive> archive = archive_factory.GetOrCreate(path);
    if (!archive)
      return v8::False(args->isolate());
    return (new Archive(archive))->GetWrapper(args->isolate());
  }

 protected:
  explicit Archive(scoped_refptr<asar::Archive> archive) : archive_(archive) {}
  virtual ~Archive() {}

  // Reads the offset and size of file.
  v8::Handle<v8::Value> GetFileInfo(mate::Arguments* args,
                                    const base::FilePath& path) {
    asar::Archive::FileInfo info;
    if (!archive_->GetFileInfo(path, &info))
      return v8::False(args->isolate());
    mate::Dictionary dict(args->isolate(), v8::Object::New(args->isolate()));
    dict.Set("size", info.size);
    dict.Set("offset", info.offset);
    return dict.GetHandle();
  }

  // Returns a fake result of fs.stat(path).
  v8::Handle<v8::Value> Stat(mate::Arguments* args,
                             const base::FilePath& path) {
    asar::Archive::Stats stats;
    if (!archive_->Stat(path, &stats))
      return v8::False(args->isolate());
    mate::Dictionary dict(args->isolate(), v8::Object::New(args->isolate()));
    dict.Set("size", stats.size);
    dict.Set("offset", stats.offset);
    dict.Set("isFile", stats.is_file);
    dict.Set("isDirectory", stats.is_directory);
    dict.Set("isLink", stats.is_link);
    return dict.GetHandle();
  }

  // mate::Wrappable:
  mate::ObjectTemplateBuilder GetObjectTemplateBuilder(v8::Isolate* isolate) {
    return mate::ObjectTemplateBuilder(isolate)
        .SetValue("path", archive_->path())
        .SetMethod("getFileInfo", &Archive::GetFileInfo)
        .SetMethod("stat", &Archive::Stat);
  }

 private:
  scoped_refptr<asar::Archive> archive_;

  DISALLOW_COPY_AND_ASSIGN(Archive);
};

void Initialize(v8::Handle<v8::Object> exports, v8::Handle<v8::Value> unused,
                v8::Handle<v8::Context> context, void* priv) {
  mate::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("createArchive", &Archive::Create);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_common_asar, Initialize)
