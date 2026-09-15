// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_LOAD_URL_PROMISES_H_
#define ELECTRON_SHELL_BROWSER_API_LOAD_URL_PROMISES_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "shell/common/gin_helper/promise.h"
#include "v8/include/v8-forward.h"

namespace electron {

// The promises webContents.loadURL()/loadFile() and navigationHistory.restore()
// return. Each is settled from the navigation events the WebContents emits
// after it was made: resolved when the main frame finishes loading; rejected
// with {errno, code, url} when the main frame fails to load, when another
// (non-same-document) main-frame navigation starts first (ERR_ABORTED), or
// when loading stops or the WebContents is destroyed with neither having
// happened (ERR_FAILED). The rejection is marked handled, so an app that
// ignores the promise gets no unhandled-rejection warning.
class LoadURLPromises {
 public:
  LoadURLPromises();
  ~LoadURLPromises();

  // Events must not affect promises made while that very event was being
  // emitted (e.g. by a listener calling loadURL() again). Take a Mark before
  // emitting and pass it to the notification afterwards.
  using Mark = uint64_t;
  Mark mark() const { return next_id_; }

  // A new pending promise for a load of |url| that is about to start.
  v8::Local<v8::Promise> Add(v8::Isolate* isolate, std::string_view url);

  // The WebContents emitted the corresponding event. Each may settle promises,
  // which runs microtasks and so arbitrary JavaScript.
  void DidFinishLoad(Mark mark);
  void DidFailLoad(Mark mark,
                   int error_code,
                   std::string_view error_description,
                   std::string_view validated_url,
                   bool is_main_frame);
  void DidStartNavigation(Mark mark,
                          std::string_view url,
                          bool is_same_document,
                          bool is_main_frame);
  void DidNavigateInPage(Mark mark);
  // Also 'destroyed'.
  void DidStopLoading(Mark mark);

 private:
  struct LoadError {
    int code;
    std::string description;
    std::string url;
  };
  struct Pending {
    Pending(uint64_t id, v8::Isolate* isolate, std::string_view url);
    ~Pending();
    uint64_t id;
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
  // Resolves/rejects everything in |settle|. Runs JavaScript; |this| may be
  // gone afterwards.
  static void Settle(PendingList settle);

  uint64_t next_id_ = 0;
  PendingList pending_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_API_LOAD_URL_PROMISES_H_
