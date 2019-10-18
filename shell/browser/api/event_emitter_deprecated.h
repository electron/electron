// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_EVENT_EMITTER_DEPRECATED_H_
#define SHELL_BROWSER_API_EVENT_EMITTER_DEPRECATED_H_

#include <utility>
#include <vector>

#include "base/optional.h"
#include "content/public/browser/browser_thread.h"
#include "electron/shell/common/api/api.mojom.h"
#include "native_mate/wrappable.h"
#include "shell/common/api/event_emitter_caller_deprecated.h"

namespace content {
class RenderFrameHost;
}

namespace mate {

namespace internal {

v8::Local<v8::Object> CreateJSEvent(
    v8::Isolate* isolate,
    v8::Local<v8::Object> object,
    content::RenderFrameHost* sender,
    base::Optional<electron::mojom::ElectronBrowser::MessageSyncCallback>
        callback);
v8::Local<v8::Object> CreateEmptyJSEvent(v8::Isolate* isolate);
v8::Local<v8::Object> CreateCustomEvent(v8::Isolate* isolate,
                                        v8::Local<v8::Object> object,
                                        v8::Local<v8::Object> event);
v8::Local<v8::Object> CreateEventFromFlags(v8::Isolate* isolate, int flags);

}  // namespace internal

// Provide helperers to emit event in JavaScript.
template <typename T>
class EventEmitter : public Wrappable<T> {
 public:
  typedef std::vector<v8::Local<v8::Value>> ValueArray;

  // Make the convinient methods visible:
  // https://isocpp.org/wiki/faq/templates#nondependent-name-lookup-members
  v8::Isolate* isolate() const { return Wrappable<T>::isolate(); }
  v8::Local<v8::Object> GetWrapper() const {
    return Wrappable<T>::GetWrapper();
  }
  v8::MaybeLocal<v8::Object> GetWrapper(v8::Isolate* isolate) const {
    return Wrappable<T>::GetWrapper(isolate);
  }

  // this.emit(name, event, args...);
  template <typename... Args>
  bool EmitCustomEvent(base::StringPiece name,
                       v8::Local<v8::Object> event,
                       Args&&... args) {
    return EmitWithEvent(
        name, internal::CreateCustomEvent(isolate(), GetWrapper(), event),
        std::forward<Args>(args)...);
  }

  // this.emit(name, new Event(flags), args...);
  template <typename... Args>
  bool EmitWithFlags(base::StringPiece name, int flags, Args&&... args) {
    return EmitCustomEvent(name,
                           internal::CreateEventFromFlags(isolate(), flags),
                           std::forward<Args>(args)...);
  }

  // this.emit(name, new Event(), args...);
  template <typename... Args>
  bool Emit(base::StringPiece name, Args&&... args) {
    return EmitWithSender(name, nullptr, base::nullopt,
                          std::forward<Args>(args)...);
  }

  // this.emit(name, new Event(sender, message), args...);
  template <typename... Args>
  bool EmitWithSender(
      base::StringPiece name,
      content::RenderFrameHost* sender,
      base::Optional<electron::mojom::ElectronBrowser::InvokeCallback> callback,
      Args&&... args) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    v8::Locker locker(isolate());
    v8::HandleScope handle_scope(isolate());
    v8::Local<v8::Object> wrapper = GetWrapper();
    if (wrapper.IsEmpty()) {
      return false;
    }
    v8::Local<v8::Object> event = internal::CreateJSEvent(
        isolate(), wrapper, sender, std::move(callback));
    return EmitWithEvent(name, event, std::forward<Args>(args)...);
  }

 protected:
  EventEmitter() {}

 private:
  // this.emit(name, event, args...);
  template <typename... Args>
  bool EmitWithEvent(base::StringPiece name,
                     v8::Local<v8::Object> event,
                     Args&&... args) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    // It's possible that |this| will be deleted by EmitEvent, so save anything
    // we need from |this| before calling EmitEvent.
    auto* isolate = this->isolate();
    v8::Locker locker(isolate);
    v8::HandleScope handle_scope(isolate);
    auto context = isolate->GetCurrentContext();
    EmitEvent(isolate, GetWrapper(), name, event, std::forward<Args>(args)...);
    v8::Local<v8::Value> defaultPrevented;
    if (event->Get(context, StringToV8(isolate, "defaultPrevented"))
            .ToLocal(&defaultPrevented)) {
      return defaultPrevented->BooleanValue(isolate);
    }
    return false;
  }

  DISALLOW_COPY_AND_ASSIGN(EventEmitter);
};

}  // namespace mate

#endif  // SHELL_BROWSER_API_EVENT_EMITTER_DEPRECATED_H_
