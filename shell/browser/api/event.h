// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_EVENT_H_
#define SHELL_BROWSER_API_EVENT_H_

#include "base/optional.h"
#include "electron/shell/common/api/api.mojom.h"
#include "native_mate/handle.h"
#include "native_mate/wrappable.h"

namespace IPC {
class Message;
}

namespace mate {

class Event : public Wrappable<Event> {
 public:
  using MessageSyncCallback =
      electron::mojom::ElectronBrowser::MessageSyncCallback;
  static Handle<Event> Create(v8::Isolate* isolate);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  // Pass the callback to be invoked.
  void SetCallback(base::Optional<MessageSyncCallback> callback);

  // event.PreventDefault().
  void PreventDefault(v8::Isolate* isolate);

  // event.sendReply(value), used for replying to synchronous messages and
  // `invoke` calls.
  bool SendReply(const base::Value& result);

 protected:
  explicit Event(v8::Isolate* isolate);
  ~Event() override;

 private:
  // Replyer for the synchronous messages.
  base::Optional<MessageSyncCallback> callback_;

  DISALLOW_COPY_AND_ASSIGN(Event);
};

}  // namespace mate

#endif  // SHELL_BROWSER_API_EVENT_H_
