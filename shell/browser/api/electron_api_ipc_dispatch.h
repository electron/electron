// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_IPC_DISPATCH_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_IPC_DISPATCH_H_

#include <cstdint>
#include <string>

#include "v8/include/v8-forward.h"
#include "v8/include/v8-local-handle.h"

namespace electron::api {
class Session;
class WebContents;
}  // namespace electron::api

// Delivers IPC from renderers to its JavaScript listeners without a JS
// trampoline. Frame IPC goes to ipcMainInternal for internal channels,
// otherwise to the sending WebContents ('ipc-message' / 'ipc-message-sync'),
// the sending frame's WebFrameMain.ipc, the WebContents' ipc and ipcMain, in
// that order; service worker IPC goes to ipcMainInternal or the worker's
// ServiceWorkerMain.ipc. Each dispatch runs inside one callback scope so
// microtasks run once afterwards, as they did with the JS trampoline.
namespace electron::ipc_dispatch {

// Whether lib/browser/ipc-dispatch.ts has registered ipcMain,
// ipcMainInternal and MessagePortMain. IPC that arrives earlier is dropped
// (nothing could be listening yet).
bool IsReady();

// Frame IPC. |event| already carries type/sender/senderFrame/frameId/
// processId/frameTreeNodeId and, for sendSync/invoke, _replyChannel.
void Message(v8::Isolate* isolate,
             api::WebContents* sender,
             v8::Local<v8::Object> event,
             bool internal,
             int frame_tree_node_id,
             const std::string& channel,
             v8::Local<v8::Value> args,
             bool sync);
void Invoke(v8::Isolate* isolate,
            api::WebContents* sender,
            v8::Local<v8::Object> event,
            bool internal,
            int frame_tree_node_id,
            const std::string& channel,
            v8::Local<v8::Value> args);
void PostMessage(v8::Isolate* isolate,
                 api::WebContents* sender,
                 v8::Local<v8::Object> event,
                 int frame_tree_node_id,
                 const std::string& channel,
                 v8::Local<v8::Value> message,
                 v8::LocalVector<v8::Value> ports);
// ipcRenderer.sendToHost() from a <webview> guest: re-emitted on the guest
// WebContents for the guest view manager.
void MessageHost(v8::Isolate* isolate,
                 api::WebContents* sender,
                 v8::Local<v8::Object> event,
                 const std::string& channel,
                 v8::Local<v8::Value> args);

// Service worker IPC. |event| carries type/versionId/processId/session and,
// for sendSync/invoke, _replyChannel.
void ServiceWorkerMessage(v8::Isolate* isolate,
                          api::Session* session,
                          int64_t version_id,
                          v8::Local<v8::Object> event,
                          bool internal,
                          const std::string& channel,
                          v8::Local<v8::Value> args,
                          bool sync);
void ServiceWorkerInvoke(v8::Isolate* isolate,
                         api::Session* session,
                         int64_t version_id,
                         v8::Local<v8::Object> event,
                         bool internal,
                         const std::string& channel,
                         v8::Local<v8::Value> args);
void ServiceWorkerPostMessage(v8::Isolate* isolate,
                              api::Session* session,
                              int64_t version_id,
                              v8::Local<v8::Object> event,
                              const std::string& channel,
                              v8::Local<v8::Value> message,
                              v8::LocalVector<v8::Value> ports);

}  // namespace electron::ipc_dispatch

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_IPC_DISPATCH_H_
