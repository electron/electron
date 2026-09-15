// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/load_url_promises.h"

#include <algorithm>
#include <utility>

#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "gin/converter.h"
#include "gin/dictionary.h"
#include "v8/include/v8-exception.h"
#include "v8/include/v8-promise.h"

namespace electron {

namespace {

constexpr int kErrAborted = -3;
constexpr int kErrFailed = -2;

}  // namespace

LoadURLPromises::Pending::Pending(v8::Isolate* isolate, std::string_view url)
    : promise(isolate), url(url) {}
LoadURLPromises::Pending::~Pending() = default;

LoadURLPromises::LoadURLPromises() = default;
LoadURLPromises::~LoadURLPromises() = default;

v8::Local<v8::Promise> LoadURLPromises::Add(v8::Isolate* isolate,
                                            std::string_view url) {
  pending_.push_back(std::make_unique<Pending>(isolate, url));
  v8::Local<v8::Promise> handle = pending_.back()->promise.GetHandle();
  // As `promise.catch(() => {})` did: no unhandled-rejection report, and no
  // pause-on-uncaught in an attached debugger.
  handle->MarkAsHandled();
  handle->MarkAsSilent();
  return handle;
}

void LoadURLPromises::DidFinishLoad() {
  PendingList settle;
  for (auto it = pending_.begin(); it != pending_.end();)
    it = Take(it, !(*it)->error, &settle);
  Settle(std::move(settle));
}

void LoadURLPromises::DidFailLoad(int error_code,
                                  std::string_view error_description,
                                  std::string_view validated_url,
                                  bool is_main_frame) {
  if (!is_main_frame)
    return;
  PendingList settle;
  for (auto it = pending_.begin(); it != pending_.end();) {
    Pending& p = **it;
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

void LoadURLPromises::DidStartNavigation(std::string_view url,
                                         bool is_same_document,
                                         bool is_main_frame) {
  if (!is_main_frame)
    return;
  PendingList settle;
  for (auto it = pending_.begin(); it != pending_.end();) {
    Pending& p = **it;
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

void LoadURLPromises::DidNavigateInPage() {
  PendingList settle;
  for (auto it = pending_.begin(); it != pending_.end();) {
    if (!(*it)->browser_initiated_in_page_navigation)
      it = Take(it, !(*it)->error, &settle);
    else
      ++it;
  }
  Settle(std::move(settle));
}

void LoadURLPromises::DidStopLoading() {
  // By now did-finish-load or did-fail-load has normally settled it; loading
  // can stop with neither (e.g. a URL with a scheme nothing handles).
  PendingList settle;
  for (auto it = pending_.begin(); it != pending_.end();) {
    Pending& p = **it;
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
  for (std::unique_ptr<Pending>& p : settle) {
    if (p->resolve) {
      p->promise.Resolve();
      continue;
    }
    const LoadError& error = *p->error;
    v8::Isolate* isolate = p->promise.isolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = p->promise.GetContext();
    v8::Context::Scope context_scope(context);
    // url.substr(0, 2048), in UTF-16 code units as String.prototype.substr.
    std::u16string url = base::UTF8ToUTF16(error.url);
    url.resize(std::min<size_t>(url.size(), 2048));
    std::u16string message = base::StrCat(
        {base::UTF8ToUTF16(error.description), u" (",
         base::NumberToString16(error.code), u") loading '", url, u"'"});
    v8::Local<v8::Object> exception =
        v8::Exception::Error(
            gin::ConvertToV8(isolate, message).As<v8::String>())
            .As<v8::Object>();
    gin::Dictionary dict(isolate, exception);
    dict.Set("errno", error.code);
    dict.Set("code", error.description);
    dict.Set("url", error.url);
    p->promise.Reject(exception);
  }
}

}  // namespace electron
