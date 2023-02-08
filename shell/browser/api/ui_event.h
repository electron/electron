// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_UI_EVENT_H_
#define ELECTRON_SHELL_BROWSER_API_UI_EVENT_H_

namespace v8 {
class Object;
template <typename T>
class Local;
}  // namespace v8

namespace electron::api {

v8::Local<v8::Object> CreateEventFromFlags(int flags);

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_UI_EVENT_H_
