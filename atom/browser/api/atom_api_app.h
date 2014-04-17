// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_APP_H_
#define ATOM_BROWSER_API_ATOM_API_APP_H_

#include <string>

#include "base/compiler_specific.h"
#include "atom/browser/api/event_emitter.h"
#include "atom/browser/browser_observer.h"
#include "native_mate/handle.h"

namespace atom {

namespace api {

class App : public mate::EventEmitter,
            public BrowserObserver {
 public:
  static mate::Handle<App> Create(v8::Isolate* isolate);

 protected:
  App();
  virtual ~App();

  // BrowserObserver implementations:
  virtual void OnWillQuit(bool* prevent_default) OVERRIDE;
  virtual void OnWindowAllClosed() OVERRIDE;
  virtual void OnOpenFile(bool* prevent_default,
                          const std::string& file_path) OVERRIDE;
  virtual void OnOpenURL(const std::string& url) OVERRIDE;
  virtual void OnActivateWithNoOpenWindows() OVERRIDE;
  virtual void OnWillFinishLaunching() OVERRIDE;
  virtual void OnFinishLaunching() OVERRIDE;

  // mate::Wrappable implementations:
  virtual mate::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate);

 private:
  DISALLOW_COPY_AND_ASSIGN(App);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_APP_H_
