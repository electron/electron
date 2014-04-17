// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include <vector>

#include "base/bind.h"
#include "atom/browser/api/atom_api_window.h"
#include "atom/browser/native_window.h"
#include "atom/browser/ui/file_dialog.h"
#include "atom/browser/ui/message_box.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "native_mate/dictionary.h"
#include "native_mate/scoped_persistent.h"

#include "atom/common/node_includes.h"

namespace mate {

template<>
struct Converter<atom::NativeWindow*> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Handle<v8::Value> val,
                     atom::NativeWindow** out) {
    using atom::api::Window;
    if (val->IsNull()) {
      *out = NULL;
      return true;  // NULL is a valid value for NativeWindow*.
    } else if (val->IsObject()) {
      Window* window = Window::Unwrap<Window>(val->ToObject());
      *out = window->window();
      return true;
    } else {
      return false;
    }
  }
};


typedef scoped_refptr<RefCountedPersistent<v8::Function>> RefCountedV8Function;

template<>
struct Converter<mate::RefCountedV8Function> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Handle<v8::Value> val,
                     RefCountedV8Function* out) {
    if (!val->IsFunction())
      return false;

    v8::Handle<v8::Function> function = v8::Handle<v8::Function>::Cast(val);
    *out = new mate::RefCountedPersistent<v8::Function>(function);
    return true;
  }
};

}  // namespace mate


namespace {

template<typename T>
void CallV8Function(const mate::RefCountedV8Function& callback, T arg) {
  v8::Locker locker(node_isolate);
  v8::HandleScope handle_scope(node_isolate);

  v8::Handle<v8::Value> value = mate::Converter<T>::ToV8(node_isolate, arg);
  callback->NewHandle(node_isolate)->Call(
      v8::Context::GetCurrent()->Global(), 1, &value);
}

template<typename T>
void CallV8Function2(
    const mate::RefCountedV8Function& callback, bool result, const T& arg) {
  if (result)
    return CallV8Function<T>(callback, arg);
  else
    return CallV8Function<void*>(callback, NULL);
}


void ShowMessageBox(int type,
                    const std::vector<std::string>& buttons,
                    const std::string& title,
                    const std::string& message,
                    const std::string& detail,
                    atom::NativeWindow* window,
                    mate::Arguments* args) {
  v8::Handle<v8::Value> peek = args->PeekNext();
  mate::RefCountedV8Function callback;
  if (mate::Converter<mate::RefCountedV8Function>::FromV8(node_isolate,
                                                          peek,
                                                          &callback)) {
    atom::ShowMessageBox(
        window,
        (atom::MessageBoxType)type,
        buttons,
        title,
        message,
        detail,
        base::Bind(&CallV8Function<int>, callback));
  } else {
    int chosen = atom::ShowMessageBox(
        window,
        (atom::MessageBoxType)type,
        buttons,
        title,
        message,
        detail);
    args->Return(chosen);
  }
}

void ShowOpenDialog(const std::string& title,
                    const base::FilePath& default_path,
                    int properties,
                    atom::NativeWindow* window,
                    mate::Arguments* args) {
  v8::Handle<v8::Value> peek = args->PeekNext();
  mate::RefCountedV8Function callback;
  if (mate::Converter<mate::RefCountedV8Function>::FromV8(node_isolate,
                                                          peek,
                                                          &callback)) {
    file_dialog::ShowOpenDialog(
        window,
        title,
        default_path,
        properties,
        base::Bind(&CallV8Function2<std::vector<base::FilePath>>,
                   callback));
  } else {
    std::vector<base::FilePath> paths;
    if (file_dialog::ShowOpenDialog(window,
                                    title,
                                    default_path,
                                    properties,
                                    &paths))
      args->Return(paths);
  }
}

void ShowSaveDialog(const std::string& title,
                    const base::FilePath& default_path,
                    atom::NativeWindow* window,
                    mate::Arguments* args) {
  v8::Handle<v8::Value> peek = args->PeekNext();
  mate::RefCountedV8Function callback;
  if (mate::Converter<mate::RefCountedV8Function>::FromV8(node_isolate,
                                                          peek,
                                                          &callback)) {
    file_dialog::ShowSaveDialog(
        window,
        title,
        default_path,
        base::Bind(&CallV8Function2<base::FilePath>, callback));
  } else {
    base::FilePath path;
    if (file_dialog::ShowSaveDialog(window,
                                    title,
                                    default_path,
                                    &path))
      args->Return(path);
  }
}

void Initialize(v8::Handle<v8::Object> exports) {
  mate::Dictionary dict(v8::Isolate::GetCurrent(), exports);
  dict.SetMethod("showMessageBox", &ShowMessageBox);
  dict.SetMethod("showOpenDialog", &ShowOpenDialog);
  dict.SetMethod("showSaveDialog", &ShowSaveDialog);
}

}  // namespace

NODE_MODULE(atom_browser_dialog, Initialize)
