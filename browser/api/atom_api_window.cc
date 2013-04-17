// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/api/atom_api_window.h"

#include "base/values.h"
#include "browser/atom_browser_context.h"
#include "browser/native_window.h"
#include "common/v8_value_converter_impl.h"
#include "content/public/browser/web_contents.h"

using content::V8ValueConverter;

namespace atom {

namespace api {

Window::Window(v8::Handle<v8::Object> wrapper, base::DictionaryValue* options)
    : EventEmitter(wrapper),
      window_(NativeWindow::Create(AtomBrowserContext::Get(), options)) {
  window_->InitFromOptions(options);
  window_->GetWebContents()->GetController().LoadURL(
      GURL("https://github.com"),
      content::Referrer(),
      content::PAGE_TRANSITION_AUTO_TOPLEVEL,
      std::string());
}

Window::~Window() {
}

// static
v8::Handle<v8::Value> Window::New(const v8::Arguments &args) {
  v8::HandleScope scope;

  if (!args[0]->IsObject())
    return node::ThrowTypeError("Bad argument");

  scoped_ptr<V8ValueConverter> converter(new V8ValueConverterImpl());
  scoped_ptr<base::Value> options(
      converter->FromV8Value(args[0], v8::Context::GetCurrent()));

  if (!options || !options->IsType(base::Value::TYPE_DICTIONARY))
    return node::ThrowTypeError("Bad argument");

  new Window(args.This(), static_cast<base::DictionaryValue*>(options.get()));

  return args.This();
}

// static
void Window::Initialize(v8::Handle<v8::Object> target) {
  v8::HandleScope scope;

  v8::Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New(Window::New);
  t->InstanceTemplate()->SetInternalFieldCount(1);
  t->SetClassName(v8::String::NewSymbol("Window"));
  target->Set(v8::String::NewSymbol("Window"), t->GetFunction());
}

}  // namespace api

}  // namespace atom

NODE_MODULE(atom_api_window, atom::api::Window::Initialize)
