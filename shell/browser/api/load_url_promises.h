// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_LOAD_URL_PROMISES_H_
#define ELECTRON_SHELL_BROWSER_API_LOAD_URL_PROMISES_H_

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "shell/common/gin_helper/promise.h"
#include "v8/include/v8-forward.h"

namespace electron {

// The promises webContents.loadURL()/loadFile() and navigationHistory.restore()
// return. Each is settled from the WebContents' navigation events after it was
// made: resolved when the main frame finishes loading; rejected with
// {errno, code, url} when the main frame fails to load, when another
// (non-same-document) main-frame navigation starts first (ERR_ABORTED), or
// when loading stops or the WebContents is destroyed with neither having
// happened (ERR_FAILED). The rejection is marked handled, so an app that
// ignores the promise gets no unhandled-rejection warning.
//
// WebContents notifies this right before emitting each event, so promise
// reactions are queued before the event's listeners run and run when the emit
// drains microtasks, i.e. straight after the listeners, and a listener that
// navigates again affects only promises still pending.
class LoadURLPromises {
 public:
  LoadURLPromises();
  ~LoadURLPromises();

  // A new pending promise for a load of |url| that is about to start.
  v8::Local<v8::Promise> Add(v8::Isolate* isolate, std::string_view url);

  bool empty() const { return pending_.empty(); }

  // The WebContents is about to emit the corresponding event. Each may settle
  // promises (queuing their reactions as microtasks).
  void DidFinishLoad();
  void DidFailLoad(int error_code,
                   std::string_view error_description,
                   std::string_view validated_url,
                   bool is_main_frame);
  void DidStartNavigation(std::string_view url,
                          bool is_same_document,
                          bool is_main_frame);
  void DidNavigateInPage();
  // Also 'destroyed'.
  void DidStopLoading();

 private:
  struct LoadError {
    int code;
    std::string description;
    std::string url;
  };
  struct Pending {
    Pending(v8::Isolate* isolate, std::string_view url);
    ~Pending();
    gin_helper::Promise<void> promise;
    std::string url;
    std::optional<LoadError> error;
    bool navigation_started = false;
    bool browser_initiated_in_page_navigation = false;
    // Set when taken out of |pending_| to be settled.
    bool resolve = false;
  };
  using PendingList = std::vector<std::unique_ptr<Pending>>;

  // Moves |*it| out of pending_ onto |settle|, to be resolved or rejected
  // with its error.
  PendingList::iterator Take(PendingList::iterator it,
                             bool resolve,
                             PendingList* settle);
  // Resolves/rejects everything in |settle|.
  static void Settle(PendingList settle);

  PendingList pending_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_API_LOAD_URL_PROMISES_H_
