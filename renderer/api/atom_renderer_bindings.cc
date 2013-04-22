// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/api/atom_renderer_bindings.h"

#include "base/logging.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebFrame.h"

namespace atom {

AtomRendererBindings::AtomRendererBindings() {
}

AtomRendererBindings::~AtomRendererBindings() {
}

void AtomRendererBindings::BindToFrame(WebKit::WebFrame* frame) {
  v8::HandleScope handle_scope;

  v8::Handle<v8::Context> context = frame->mainWorldScriptContext();
  if (context.IsEmpty())
    return;

  v8::Context::Scope scope(context);

  v8::Handle<v8::Object> process =
      context->Global()->Get(v8::String::New("process"))->ToObject();
  DCHECK(!process.IsEmpty());

  AtomBindings::BindTo(process);
}

}  // namespace atom
