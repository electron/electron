// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_APP_H_
#define ATOM_BROWSER_API_ATOM_API_APP_H_

#include <string>

#include "base/compiler_specific.h"
#include "atom/browser/browser_observer.h"
#include "atom/common/api/atom_api_event_emitter.h"

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
  virtual void OnActivateWithNoOpenWindows() OVERRIDE;
  virtual void OnWillFinishLaunching() OVERRIDE;
  virtual void OnFinishLaunching() OVERRIDE;

 private:
  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void Quit(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void Exit(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void Terminate(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void Focus(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void GetVersion(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void SetVersion(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void GetName(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void SetName(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void AppendSwitch(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void AppendArgument(const v8::FunctionCallbackInfo<v8::Value>& args);

#if defined(OS_MACOSX)
  static void DockBounce(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void DockCancelBounce(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void DockSetBadgeText(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void DockGetBadgeText(const v8::FunctionCallbackInfo<v8::Value>& args);
#endif  // defined(OS_MACOSX)

  DISALLOW_COPY_AND_ASSIGN(App);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_APP_H_
