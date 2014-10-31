// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_APP_H_
#define ATOM_BROWSER_API_ATOM_API_APP_H_

#include <string>

#include "atom/browser/api/event_emitter.h"
#include "atom/browser/browser_observer.h"
#include "base/callback.h"
#include "native_mate/handle.h"

class GURL;

namespace base {
class FilePath;
}

namespace atom {

namespace api {

class App : public mate::EventEmitter,
            public BrowserObserver {
 public:
  typedef base::Callback<void(std::string)> ResolveProxyCallback;

  static mate::Handle<App> Create(v8::Isolate* isolate);

 protected:
  App();
  virtual ~App();

  // BrowserObserver implementations:
  virtual void OnWillQuit(bool* prevent_default) OVERRIDE;
  virtual void OnWindowAllClosed() OVERRIDE;
  virtual void OnQuit() OVERRIDE;
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
  base::FilePath GetDataPath();
  void ResolveProxy(const GURL& url, ResolveProxyCallback callback);
  void SetDesktopName(const std::string& desktop_name);

  DISALLOW_COPY_AND_ASSIGN(App);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_APP_H_
