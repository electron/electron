// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_APP_H_
#define ATOM_BROWSER_API_ATOM_API_APP_H_

#include "base/compiler_specific.h"
#include "browser/api/atom_api_event_emitter.h"
#include "browser/browser_observer.h"

namespace atom {

namespace api {

class App : public EventEmitter,
            public BrowserObserver {
 public:
  virtual ~App();

  static void Initialize(v8::Handle<v8::Object> target);

 protected:
  explicit App(v8::Handle<v8::Object> wrapper);

  // BrowserObserver implementations:
  virtual void OnWillQuit(bool* prevent_default) OVERRIDE;
  virtual void OnWindowAllClosed() OVERRIDE;
  virtual void OnOpenFile(bool* prevent_default,
                          const std::string& file_path) OVERRIDE;
  virtual void OnOpenURL(const std::string& url) OVERRIDE;
  virtual void OnWillFinishLaunching() OVERRIDE;
  virtual void OnFinishLaunching() OVERRIDE;

 private:
  static v8::Handle<v8::Value> New(const v8::Arguments &args);

  static v8::Handle<v8::Value> Quit(const v8::Arguments &args);
  static v8::Handle<v8::Value> Exit(const v8::Arguments &args);
  static v8::Handle<v8::Value> Terminate(const v8::Arguments &args);
  static v8::Handle<v8::Value> Focus(const v8::Arguments &args);
  static v8::Handle<v8::Value> GetVersion(const v8::Arguments &args);
  static v8::Handle<v8::Value> SetVersion(const v8::Arguments &args);
  static v8::Handle<v8::Value> AppendSwitch(const v8::Arguments &args);
  static v8::Handle<v8::Value> AppendArgument(const v8::Arguments &args);

#if defined(OS_MACOSX)
  static v8::Handle<v8::Value> DockBounce(const v8::Arguments& args);
  static v8::Handle<v8::Value> DockCancelBounce(const v8::Arguments& args);
  static v8::Handle<v8::Value> DockSetBadgeText(const v8::Arguments& args);
  static v8::Handle<v8::Value> DockGetBadgeText(const v8::Arguments& args);
#endif  // defined(OS_MACOSX)

  DISALLOW_COPY_AND_ASSIGN(App);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_APP_H_
