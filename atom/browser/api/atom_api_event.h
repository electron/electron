// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_EVENT_H_
#define ATOM_BROWSER_API_ATOM_API_EVENT_H_

#include "atom/common/v8/scoped_persistent.h"
#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/strings/string16.h"
#include "content/public/browser/web_contents_observer.h"
#include "vendor/node/src/node_object_wrap.h"

namespace IPC {
class Message;
}

namespace atom {

namespace api {

class Event : public node::ObjectWrap,
              public content::WebContentsObserver {
 public:
  virtual ~Event();

  // Create a V8 Event object.
  static v8::Handle<v8::Object> CreateV8Object();

  // Pass the sender and message to be replied.
  void SetSenderAndMessage(content::WebContents* sender, IPC::Message* message);

  // Whether event.preventDefault() is called.
  bool prevent_default() const { return prevent_default_; }

 protected:
  Event();

  // content::WebContentsObserver implementations:
  virtual void WebContentsDestroyed(content::WebContents*) OVERRIDE;

 private:
  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void PreventDefault(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void SendReply(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void Destroy(const v8::FunctionCallbackInfo<v8::Value>& args);

  static ScopedPersistent<v8::Function> constructor_template_;

  // Replyer for the synchronous messages.
  content::WebContents* sender_;
  IPC::Message* message_;

  bool prevent_default_;

  DISALLOW_COPY_AND_ASSIGN(Event);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_EVENT_H_
