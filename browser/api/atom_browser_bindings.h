// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_BROWSER_BINDINGS_
#define ATOM_BROWSER_API_ATOM_BROWSER_BINDINGS_

#include "base/string16.h"
#include "common/api/atom_bindings.h"

namespace base {
class ListValue;
}

namespace IPC {
class Message;
class Sender;
}

namespace atom {

class AtomBrowserBindings : public AtomBindings {
 public:
  AtomBrowserBindings();
  virtual ~AtomBrowserBindings();

  // Called when the node.js main script has been loaded.
  virtual void AfterLoad();

  // Called when received a message from renderer.
  void OnRendererMessage(int process_id,
                         int routing_id,
                         const string16& channel,
                         const base::ListValue& args);

  // Called when received a synchronous message from renderer.
  void OnRendererMessageSync(int process_id,
                             int routing_id,
                             const string16& channel,
                             const base::ListValue& args,
                             IPC::Sender* sender,
                             IPC::Message* message);

  // The require('atom').browserMainParts object.
  v8::Handle<v8::Object> browser_main_parts() {
    return browser_main_parts_;
  }

 private:
  v8::Persistent<v8::Object> browser_main_parts_;

  DISALLOW_COPY_AND_ASSIGN(AtomBrowserBindings);
};

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_BINDINGS_
