// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_IPC_EVENT_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_IPC_EVENT_H_

#include <cstdint>

#include "content/public/browser/global_routing_id.h"
#include "gin/wrappable.h"
#include "shell/common/api/api.mojom.h"
#include "shell/common/gin_helper/constructible.h"
#include "v8/include/cppgc/member.h"
#include "v8/include/v8-traced-handle.h"

namespace content {
class RenderFrameHost;
}

namespace gin_helper::internal {
class ReplyChannel;
}

namespace electron::api {

class Session;

// The `event` passed to ipcMain / webContents / WebFrameMain.ipc listeners
// for IPC from a frame. Its properties are native data properties of the
// instance template, so making one is a single template instantiation.
class IpcMainEvent final : public gin::Wrappable<IpcMainEvent>,
                           public gin_helper::Constructible<IpcMainEvent> {
 public:
  using ReplyCallback = electron::mojom::ElectronApiIPC::InvokeCallback;

  // |frame| may be null. |callback| is set for sendSync / invoke.
  static IpcMainEvent* Create(v8::Isolate* isolate,
                              v8::Local<v8::Object> sender,
                              content::RenderFrameHost* frame,
                              ReplyCallback callback);
  static IpcMainEvent* FromV8(v8::Isolate* isolate, v8::Local<v8::Value> value);

  // gin_helper::Constructible
  static IpcMainEvent* New(v8::Isolate* isolate);
  static void FillObjectTemplate(v8::Isolate*, v8::Local<v8::ObjectTemplate>);
  static void FillInstanceTemplate(v8::Isolate*, v8::Local<v8::ObjectTemplate>);
  static const char* GetClassName() { return "IpcMainEvent"; }

  // gin::Wrappable
  static gin::WrapperInfo kWrapperInfo;
  const gin::WrapperInfo* wrapper_info() const override;
  const char* GetHumanReadableName() const override;
  void Trace(cppgc::Visitor* visitor) const override;

  IpcMainEvent();
  ~IpcMainEvent() override;

  void PreventDefault() { default_prevented_ = true; }
  bool GetDefaultPrevented() const { return default_prevented_; }
  gin_helper::internal::ReplyChannel* reply_channel() const {
    return reply_channel_.Get();
  }
  int frame_tree_node_id() const { return frame_tree_node_id_; }
  void SetPorts(v8::Isolate* isolate, v8::Local<v8::Value> ports);

 private:
  bool default_prevented_ = false;
  bool has_frame_ = false;
  content::GlobalRenderFrameHostId frame_id_;
  int frame_tree_node_id_ = 0;
  v8::TracedReference<v8::Object> sender_;
  v8::TracedReference<v8::Value> ports_;
  cppgc::Member<gin_helper::internal::ReplyChannel> reply_channel_;
};

// The same for IPC from a service worker (ServiceWorkerMain.ipc).
class IpcMainServiceWorkerEvent final
    : public gin::Wrappable<IpcMainServiceWorkerEvent>,
      public gin_helper::Constructible<IpcMainServiceWorkerEvent> {
 public:
  using ReplyCallback = electron::mojom::ElectronApiIPC::InvokeCallback;

  static IpcMainServiceWorkerEvent* Create(v8::Isolate* isolate,
                                           Session* session,
                                           int64_t version_id,
                                           int process_id,
                                           ReplyCallback callback);
  static IpcMainServiceWorkerEvent* FromV8(v8::Isolate* isolate,
                                           v8::Local<v8::Value> value);

  // gin_helper::Constructible
  static IpcMainServiceWorkerEvent* New(v8::Isolate* isolate);
  static void FillObjectTemplate(v8::Isolate*, v8::Local<v8::ObjectTemplate>);
  static void FillInstanceTemplate(v8::Isolate*, v8::Local<v8::ObjectTemplate>);
  static const char* GetClassName() { return "IpcMainServiceWorkerEvent"; }

  // gin::Wrappable
  static gin::WrapperInfo kWrapperInfo;
  const gin::WrapperInfo* wrapper_info() const override;
  const char* GetHumanReadableName() const override;
  void Trace(cppgc::Visitor* visitor) const override;

  IpcMainServiceWorkerEvent();
  ~IpcMainServiceWorkerEvent() override;

  void PreventDefault() { default_prevented_ = true; }
  bool GetDefaultPrevented() const { return default_prevented_; }
  gin_helper::internal::ReplyChannel* reply_channel() const {
    return reply_channel_.Get();
  }
  Session* session() const { return session_.Get(); }
  int64_t version_id() const { return version_id_; }
  void SetPorts(v8::Isolate* isolate, v8::Local<v8::Value> ports);

 private:
  bool default_prevented_ = false;
  int64_t version_id_ = 0;
  int process_id_ = 0;
  cppgc::Member<Session> session_;
  v8::TracedReference<v8::Value> ports_;
  cppgc::Member<gin_helper::internal::ReplyChannel> reply_channel_;
};

// The reply channel of either kind of event, if it has one.
gin_helper::internal::ReplyChannel* ReplyChannelOf(v8::Isolate* isolate,
                                                   v8::Local<v8::Value> event);

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_IPC_EVENT_H_
