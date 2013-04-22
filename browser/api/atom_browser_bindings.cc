// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/api/atom_browser_bindings.h"

#include "base/logging.h"
#include "base/values.h"
#include "content/public/browser/browser_thread.h"
#include "vendor/node/src/node.h"
#include "vendor/node/src/node_internals.h"

using node::node_isolate;

namespace atom {

AtomBrowserBindings::AtomBrowserBindings() {
}

AtomBrowserBindings::~AtomBrowserBindings() {
}

void AtomBrowserBindings::AfterLoad() {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  v8::HandleScope scope;

  v8::Handle<v8::Object> global = node::g_context->Global();
  v8::Handle<v8::Object> atom =
      global->Get(v8::String::New("__atom"))->ToObject();
  DCHECK(!atom.IsEmpty());

  browser_main_parts_ = v8::Persistent<v8::Object>::New(
      node_isolate,
      atom->Get(v8::String::New("browserMainParts"))->ToObject());
  DCHECK(!browser_main_parts_.IsEmpty());
}

void AtomBrowserBindings::OnRendererMessage(
    int routing_id, const base::ListValue& args) {
  LOG(ERROR) << "OnRendererMessage routing_id:" << routing_id
             << " args:" << args;
}

}  // namespace atom
