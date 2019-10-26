// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_API_REMOTE_REMOTE_CALLBACK_FREER_H_
#define SHELL_COMMON_API_REMOTE_REMOTE_CALLBACK_FREER_H_

#include <string>

#include "content/public/browser/web_contents_observer.h"
#include "shell/common/api/remote/object_life_monitor.h"
#include "third_party/blink/public/common/messaging/cloneable_message.h"

namespace electron {

class RemoteCallbackFreer : public ObjectLifeMonitor,
                            public content::WebContentsObserver {
 public:
  static void BindTo(v8::Isolate* isolate,
                     v8::Local<v8::Object> target,
                     int frame_id,
                     const std::string& channel,
                     v8::Local<v8::Value> args,
                     content::WebContents* web_conents);

 protected:
  RemoteCallbackFreer(v8::Isolate* isolate,
                      v8::Local<v8::Object> target,
                      int frame_id,
                      const std::string& channel,
                      blink::CloneableMessage args,
                      content::WebContents* web_conents);
  ~RemoteCallbackFreer() override;

  void RunDestructor() override;

  // content::WebContentsObserver:
  void RenderViewDeleted(content::RenderViewHost*) override;

 private:
  int frame_id_;
  std::string channel_;
  blink::CloneableMessage args_;

  DISALLOW_COPY_AND_ASSIGN(RemoteCallbackFreer);
};

}  // namespace electron

#endif  // SHELL_COMMON_API_REMOTE_REMOTE_CALLBACK_FREER_H_
