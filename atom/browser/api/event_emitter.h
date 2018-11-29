// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_EVENT_EMITTER_H_
#define ATOM_BROWSER_API_EVENT_EMITTER_H_

#include <vector>

#include "atom/common/api/event_emitter_caller.h"
#include "content/public/browser/browser_thread.h"
#include "native_mate/wrappable.h"

namespace content {
class RenderFrameHost;
}

namespace IPC {
class Message;
}

namespace mate {

namespace internal {

v8::Local<v8::Object> CreateJSEvent(v8::Isolate* isolate,
                                    v8::Local<v8::Object> object,
                                    content::RenderFrameHost* sender,
                                    IPC::Message* message);
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

  // this.emit(name, event, args...);
  template <typename... Args>
  bool EmitCustomEvent(const base::StringPiece& name,
                       v8::Local<v8::Object> event,
                       const Args&... args) {
    return EmitWithEvent(
        name, internal::CreateCustomEvent(isolate(), GetWrapper(), event),
        args...);
  }

  // this.emit(name, new Event(flags), args...);
  template <typename... Args>
  bool EmitWithFlags(const base::StringPiece& name,
                     int flags,
                     const Args&... args) {
    return EmitCustomEvent(
        name, internal::CreateEventFromFlags(isolate(), flags), args...);
  }

  // this.emit(name, new Event(), args...);
  template <typename... Args>
  bool Emit(const base::StringPiece& name, const Args&... args) {
    return EmitWithSender(name, nullptr, nullptr, args...);
  }

  // this.emit(name, new Event(sender, message), args...);
  template <typename... Args>
  bool EmitWithSender(const base::StringPiece& name,
                      content::RenderFrameHost* sender,
                      IPC::Message* message,
                      const Args&... args) {
    v8::Locker locker(isolate());
    v8::HandleScope handle_scope(isolate());
    v8::Local<v8::Object> wrapper = GetWrapper();
    if (wrapper.IsEmpty()) {
      return false;
    }
    v8::Local<v8::Object> event =
        internal::CreateJSEvent(isolate(), wrapper, sender, message);
    return EmitWithEvent(name, event, args...);
  }

 protected:
  EventEmitter() {}

 private:
  // this.emit(name, event, args...);
  template <typename... Args>
  bool EmitWithEvent(const base::StringPiece& name,
                     v8::Local<v8::Object> event,
                     const Args&... args) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    v8::Locker locker(isolate());
    v8::HandleScope handle_scope(isolate());
    EmitEvent(isolate(), GetWrapper(), name, event, args...);
    return event->Get(StringToV8(isolate(), "defaultPrevented"))
        ->BooleanValue(isolate()->GetCurrentContext())
        .ToChecked();
  }

  DISALLOW_COPY_AND_ASSIGN(EventEmitter);
};

}  // namespace mate

#endif  // ATOM_BROWSER_API_EVENT_EMITTER_H_
