// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include "atom/common/api/object_life_monitor.h"
#include "content/public/browser/web_contents_observer.h"

namespace atom {

class RemoteCallbackFreer : public ObjectLifeMonitor,
                            public content::WebContentsObserver {
 public:
  static void BindTo(v8::Isolate* isolate,
                     v8::Local<v8::Object> target,
                     int object_id,
                     content::WebContents* web_conents);

 protected:
  RemoteCallbackFreer(v8::Isolate* isolate,
                      v8::Local<v8::Object> target,
                      int object_id,
                      content::WebContents* web_conents);
  ~RemoteCallbackFreer() override;

  void RunDestructor() override;

  // content::WebContentsObserver:
  void RenderViewDeleted(content::RenderViewHost*) override;

 private:
  int object_id_;

  DISALLOW_COPY_AND_ASSIGN(RemoteCallbackFreer);
};

}  // namespace atom
