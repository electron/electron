// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_BROWSER_BINDINGS_H_
#define ATOM_BROWSER_API_ATOM_BROWSER_BINDINGS_H_

#include "atom/common/api/atom_bindings.h"
#include "base/strings/string16.h"

namespace base {
class ListValue;
}

namespace content {
class WebContents;
}

namespace IPC {
class Message;
}

namespace atom {

class AtomBrowserBindings : public AtomBindings {
 public:
  AtomBrowserBindings();

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
                             content::WebContents* sender,
                             IPC::Message* message);

 private:
  DISALLOW_COPY_AND_ASSIGN(AtomBrowserBindings);
};

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_BROWSER_BINDINGS_H_
