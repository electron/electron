// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_EVENT_EMITTER_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_EVENT_EMITTER_H_

#include <string_view>

namespace v8 {
template <typename T>
class Local;
class Object;
class Isolate;
}  // namespace v8

namespace electron {

v8::Local<v8::Object> GetEventEmitterPrototype(v8::Isolate* isolate);

// Whether |emitter|.emit(|name|, ...) can do anything at all. False only when
// that is provably a no-op: |emitter| still uses Node's own emit(), nothing
// listens for |name|, and |name| is not 'error'. The answer is read from the
// emitter's listener table on every call, without entering JavaScript, so
// there is no native copy of it to fall out of date.
bool MayHaveEventListeners(v8::Isolate* isolate,
                           v8::Local<v8::Object> emitter,
                           std::string_view name);

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_EVENT_EMITTER_H_
