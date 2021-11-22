// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_EVENT_EMITTER_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_EVENT_EMITTER_H_

namespace v8 {
template <typename T>
class Local;
class Object;
class Isolate;
}  // namespace v8

namespace electron {

v8::Local<v8::Object> GetEventEmitterPrototype(v8::Isolate* isolate);

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_EVENT_EMITTER_H_
