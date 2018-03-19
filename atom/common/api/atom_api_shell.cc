// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>

#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "atom/common/native_mate_converters/gurl_converter.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/common/node_includes.h"
#include "atom/common/platform_util.h"
#include "native_mate/dictionary.h"

#if defined(OS_WIN)
#include "base/win/scoped_com_initializer.h"
#include "base/win/shortcut.h"

namespace mate {

template <>
struct Converter<base::win::ShortcutOperation> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Handle<v8::Value> val,
                     base::win::ShortcutOperation* out) {
    std::string operation;
    if (!ConvertFromV8(isolate, val, &operation))
      return false;
    if (operation.empty() || operation == "create")
      *out = base::win::SHORTCUT_CREATE_ALWAYS;
    else if (operation == "update")
      *out = base::win::SHORTCUT_UPDATE_EXISTING;
    else if (operation == "replace")
      *out = base::win::SHORTCUT_REPLACE_EXISTING;
    else
      return false;
    return true;
  }
};

}  // namespace mate
#endif

namespace {

typedef v8::CopyablePersistentTraits<v8::Promise::Resolver>::CopyablePersistent
  PromiseResolverPersistent;

class MoveItemToTrashRequest {
 public:
  static MoveItemToTrashRequest* Create(v8::Isolate* isolate,
                                        mate::Arguments* args,
                                        const base::FilePath& file_path) {
    return new MoveItemToTrashRequest(isolate, args, file_path);
  }

  MoveItemToTrashRequest(v8::Isolate* isolate,
                         mate::Arguments* args,
                         const base::FilePath& file_path)
    : isolate_(isolate), args_(args), file_path_(file_path) {}

  ~MoveItemToTrashRequest() = default;

  void Start() {
    v8::Locker locker(isolate_);
    v8::Isolate::Scope isolate_scope(isolate_);
    v8::HandleScope handle_scope(isolate_);

    resolver_handle_.Reset(isolate_, v8::Promise::Resolver::New(isolate_));

    args_->Return(resolver_handle_.Get(isolate_)->GetPromise().As<v8::Value>());

    auto callback = base::Bind(
      &MoveItemToTrashRequest::OnMoveItemToTrashFinished,
      base::Unretained(this));
    platform_util::MoveItemToTrash(file_path_, callback);
  }

  void OnMoveItemToTrashFinished(bool result) {
    v8::Locker locker(isolate_);
    v8::Isolate::Scope isolate_scope(isolate_);
    v8::HandleScope handle_scope(isolate_);

    if (result) {
      resolver_handle_.Get(isolate_)->Resolve(v8::Undefined(isolate_));
    } else {
      auto message = "Underlying command returned error message code";
      auto error = v8::Exception::Error(mate::StringToV8(isolate_, message));
      resolver_handle_.Get(isolate_)->Reject(error);
    }

    delete this;
  }

 private:
  v8::Isolate* isolate_;
  mate::Arguments* args_;
  PromiseResolverPersistent resolver_handle_;
  base::FilePath file_path_;

  DISALLOW_COPY_AND_ASSIGN(MoveItemToTrashRequest);
};


void OnOpenExternalFinished(
    v8::Isolate* isolate,
    const base::Callback<void(v8::Local<v8::Value>)>& callback,
    const std::string& error) {
  if (error.empty())
    callback.Run(v8::Null(isolate));
  else
    callback.Run(v8::String::NewFromUtf8(isolate, error.c_str()));
}

bool OpenExternal(
#if defined(OS_WIN)
    const base::string16& url,
#else
    const GURL& url,
#endif
    mate::Arguments* args) {
  bool activate = true;
  if (args->Length() >= 2) {
    mate::Dictionary options;
    if (args->GetNext(&options)) {
      options.Get("activate", &activate);
    }
  }

  if (args->Length() >= 3) {
    base::Callback<void(v8::Local<v8::Value>)> callback;
    if (args->GetNext(&callback)) {
      platform_util::OpenExternal(
          url, activate,
          base::Bind(&OnOpenExternalFinished, args->isolate(), callback));
      return true;
    }
  }

  return platform_util::OpenExternal(url, activate);
}

void MoveItemToTrash(const base::FilePath& url, mate::Arguments* args) {
  v8::Isolate* isolate = args->isolate();
  auto request = MoveItemToTrashRequest::Create(isolate, args, url);
  request->Start();
}

#if defined(OS_WIN)
bool WriteShortcutLink(const base::FilePath& shortcut_path,
                       mate::Arguments* args) {
  base::win::ShortcutOperation operation = base::win::SHORTCUT_CREATE_ALWAYS;
  args->GetNext(&operation);
  mate::Dictionary options = mate::Dictionary::CreateEmpty(args->isolate());
  if (!args->GetNext(&options)) {
    args->ThrowError();
    return false;
  }

  base::win::ShortcutProperties properties;
  base::FilePath path;
  base::string16 str;
  int index;
  if (options.Get("target", &path))
    properties.set_target(path);
  if (options.Get("cwd", &path))
    properties.set_working_dir(path);
  if (options.Get("args", &str))
    properties.set_arguments(str);
  if (options.Get("description", &str))
    properties.set_description(str);
  if (options.Get("icon", &path) && options.Get("iconIndex", &index))
    properties.set_icon(path, index);
  if (options.Get("appUserModelId", &str))
    properties.set_app_id(str);

  base::win::ScopedCOMInitializer com_initializer;
  return base::win::CreateOrUpdateShortcutLink(shortcut_path, properties,
                                               operation);
}

v8::Local<v8::Value> ReadShortcutLink(mate::Arguments* args,
                                      const base::FilePath& path) {
  using base::win::ShortcutProperties;
  mate::Dictionary options = mate::Dictionary::CreateEmpty(args->isolate());
  base::win::ScopedCOMInitializer com_initializer;
  base::win::ShortcutProperties properties;
  if (!base::win::ResolveShortcutProperties(
          path, ShortcutProperties::PROPERTIES_ALL, &properties)) {
    args->ThrowError("Failed to read shortcut link");
    return v8::Null(args->isolate());
  }
  options.Set("target", properties.target);
  options.Set("cwd", properties.working_dir);
  options.Set("args", properties.arguments);
  options.Set("description", properties.description);
  options.Set("icon", properties.icon);
  options.Set("iconIndex", properties.icon_index);
  options.Set("appUserModelId", properties.app_id);
  return options.GetHandle();
}
#endif

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  mate::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("showItemInFolder", &platform_util::ShowItemInFolder);
  dict.SetMethod("openItem", &platform_util::OpenItem);
  dict.SetMethod("openExternal", &OpenExternal);
  dict.SetMethod("moveItemToTrash", &MoveItemToTrash);
  dict.SetMethod("moveItemToTrashSync", &platform_util::MoveItemToTrashSync);
  dict.SetMethod("beep", &platform_util::Beep);
#if defined(OS_WIN)
  dict.SetMethod("writeShortcutLink", &WriteShortcutLink);
  dict.SetMethod("readShortcutLink", &ReadShortcutLink);
#endif
}

}  // namespace

NODE_BUILTIN_MODULE_CONTEXT_AWARE(atom_common_shell, Initialize)
