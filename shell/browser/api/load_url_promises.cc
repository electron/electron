// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/load_url_promises.h"

#include <optional>
#include <utility>

#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "gin/converter.h"
#include "gin/dictionary.h"
#include "shell/common/node_includes.h"
#include "v8/include/v8-exception.h"
#include "v8/include/v8-promise.h"

namespace electron {

namespace {

constexpr int kErrAborted = -3;
constexpr int kErrFailed = -2;

}  // namespace

LoadURLPromises::Pending::Pending(uint64_t id,
                                  v8::Isolate* isolate,
                                  std::string_view url)
    : id(id), promise(isolate), url(url) {}
LoadURLPromises::Pending::~Pending() = default;

LoadURLPromises::LoadURLPromises() = default;
LoadURLPromises::~LoadURLPromises() = default;

v8::Local<v8::Promise> LoadURLPromises::Add(v8::Isolate* isolate,
                                            std::string_view url) {
  pending_.push_back(std::make_unique<Pending>(next_id_++, isolate, url));
  v8::Local<v8::Promise> handle = pending_.back()->promise.GetHandle();
  handle->MarkAsHandled();
  return handle;
}

void LoadURLPromises::DidFinishLoad(Mark mark) {
  PendingList settle;
  for (auto it = pending_.begin(); it != pending_.end();) {
    if ((*it)->id < mark)
      it = Take(it, !(*it)->error, &settle);
    else
      ++it;
  }
  Settle(std::move(settle));
}

void LoadURLPromises::DidFailLoad(Mark mark,
                                  int error_code,
                                  std::string_view error_description,
                                  std::string_view validated_url,
                                  bool is_main_frame) {
  if (!is_main_frame)
    return;
  PendingList settle;
  for (auto it = pending_.begin(); it != pending_.end();) {
    Pending& p = **it;
    if (p.id >= mark) {
      ++it;
      continue;
    }
    if (!p.error) {
      p.error = LoadError{error_code, std::string(error_description),
                          std::string(validated_url)};
    }
    // A failure before any navigation started is final; otherwise wait for
    // did-finish-load / did-stop-loading to report it.
    if (!p.navigation_started)
      it = Take(it, false, &settle);
    else
      ++it;
  }
  Settle(std::move(settle));
}

void LoadURLPromises::DidStartNavigation(Mark mark,
                                         std::string_view url,
                                         bool is_same_document,
                                         bool is_main_frame) {
  if (!is_main_frame)
    return;
  PendingList settle;
  for (auto it = pending_.begin(); it != pending_.end();) {
    Pending& p = **it;
    if (p.id >= mark) {
      ++it;
      continue;
    }
    if (p.navigation_started && !is_same_document) {
      // Another navigation replaced this one. Same-document navigations
      // (pushState, location.hash) do not count, so a page may route while it
      // loads.
      p.error = LoadError{kErrAborted, "ERR_ABORTED", std::string(url)};
      it = Take(it, false, &settle);
      continue;
    }
    p.browser_initiated_in_page_navigation =
        p.navigation_started && is_same_document;
    p.navigation_started = true;
    ++it;
  }
  Settle(std::move(settle));
}

void LoadURLPromises::DidNavigateInPage(Mark mark) {
  PendingList settle;
  for (auto it = pending_.begin(); it != pending_.end();) {
    if ((*it)->id < mark && !(*it)->browser_initiated_in_page_navigation)
      it = Take(it, !(*it)->error, &settle);
    else
      ++it;
  }
  Settle(std::move(settle));
}

void LoadURLPromises::DidStopLoading(Mark mark) {
  // By now did-finish-load or did-fail-load has normally settled it; loading
  // can stop with neither (e.g. a URL with a scheme nothing handles).
  PendingList settle;
  for (auto it = pending_.begin(); it != pending_.end();) {
    Pending& p = **it;
    if (p.id >= mark) {
      ++it;
      continue;
    }
    if (!p.error)
      p.error = LoadError{kErrFailed, "ERR_FAILED", p.url};
    it = Take(it, false, &settle);
  }
  Settle(std::move(settle));
}

LoadURLPromises::PendingList::iterator LoadURLPromises::Take(
    PendingList::iterator it,
    bool resolve,
    PendingList* settle) {
  (*it)->resolve = resolve;
  settle->push_back(std::move(*it));
  return pending_.erase(it);
}

// static
void LoadURLPromises::Settle(PendingList settle) {
  if (settle.empty())
    return;
  v8::Isolate* isolate = settle.front()->promise.isolate();
  v8::HandleScope handle_scope(isolate);
  // These used to be settled by JS listeners of the event just emitted, so
  // reactions (which may navigate again) ran, with process.nextTick, before
  // the emit returned to content/; keep it that way.
  std::optional<node::CallbackScope> callback_scope;
  if (node::Environment* env = node::Environment::GetCurrent(isolate)) {
    callback_scope.emplace(env, v8::Object::New(isolate),
                           node::async_context{0, 0});
  }
  for (std::unique_ptr<Pending>& p : settle) {
    if (p->resolve) {
      p->promise.Resolve();
      continue;
    }
    const LoadError& error = *p->error;
    v8::Local<v8::Context> context = p->promise.GetContext();
    v8::Context::Scope context_scope(context);
    std::string message = base::StrCat(
        {error.description, " (", base::NumberToString(error.code),
         ") loading '", std::string_view(error.url).substr(0, 2048), "'"});
    v8::Local<v8::Object> exception =
        v8::Exception::Error(gin::StringToV8(isolate, message))
            .As<v8::Object>();
    gin::Dictionary dict(isolate, exception);
    dict.Set("errno", error.code);
    dict.Set("code", error.description);
    dict.Set("url", error.url);
    p->promise.Reject(exception);
  }
}

}  // namespace electron
