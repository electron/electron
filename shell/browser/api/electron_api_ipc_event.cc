// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_ipc_event.h"

#include <string_view>
#include <utility>

#include "content/public/browser/render_frame_host.h"
#include "gin/converter.h"
#include "gin/object_template_builder.h"
#include "gin/per_context_data.h"
#include "shell/browser/api/electron_api_service_worker_context.h"
#include "shell/browser/api/electron_api_session.h"
#include "shell/common/gin_converters/frame_converter.h"
#include "shell/common/gin_helper/reply_channel.h"
#include "shell/common/gin_helper/wrappable_pointer_tags.h"
#include "shell/common/node_util.h"
#include "v8/include/cppgc/allocation.h"
#include "v8/include/v8-cppgc.h"

namespace electron::api {

namespace {

v8::Local<v8::String> Symbol(v8::Isolate* isolate, std::string_view name) {
  return gin::StringToSymbol(isolate, name);
}

// Makes sure T's templates exist in the current context; they are otherwise
// only built when the constructor is first exposed.
template <typename T>
void EnsureTemplate(v8::Isolate* isolate) {
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  gin::PerContextData* data = gin::PerContextData::From(context);
  if (data && data->GetObjectTemplate(&T::kWrapperInfo).IsEmpty())
    T::GetConstructor(isolate, context, &T::kWrapperInfo);
}

template <typename T>
T* HolderOf(const v8::PropertyCallbackInfo<v8::Value>& info) {
  T* self = nullptr;
  gin::ConvertFromV8(info.GetIsolate(), info.Holder(), &self);
  return self;
}

using Getter = v8::AccessorNameGetterCallback;

// With no setter of its own, assigning to one of these replaces it with a
// plain data property, as assigning to the data properties they used to be
// did.
void Define(v8::Isolate* isolate,
            v8::Local<v8::ObjectTemplate> templ,
            std::string_view name,
            Getter getter,
            v8::PropertyAttribute attribute = v8::None) {
  templ->SetNativeDataProperty(Symbol(isolate, name), getter, nullptr,
                               v8::Local<v8::Value>(), attribute,
                               v8::SideEffectType::kHasNoSideEffect);
}

v8::Local<v8::Value> ReplyChannelWrapper(
    v8::Isolate* isolate,
    gin_helper::internal::ReplyChannel* reply_channel) {
  v8::Local<v8::Object> wrapper;
  if (reply_channel && reply_channel->GetWrapper(isolate).ToLocal(&wrapper))
    return wrapper;
  return v8::Undefined(isolate);
}

}  // namespace

// ---- IpcMainEvent -----------------------------------------------------------

gin::WrapperInfo IpcMainEvent::kWrapperInfo =
    electron::MakeWrapperInfo(electron::kElectronIpcMainEvent);

IpcMainEvent::IpcMainEvent() = default;
IpcMainEvent::~IpcMainEvent() = default;

// static
IpcMainEvent* IpcMainEvent::New(v8::Isolate* isolate) {
  return cppgc::MakeGarbageCollected<IpcMainEvent>(
      isolate->GetCppHeap()->GetAllocationHandle());
}

// static
IpcMainEvent* IpcMainEvent::Create(v8::Isolate* isolate,
                                   v8::Local<v8::Object> sender,
                                   content::RenderFrameHost* frame,
                                   ReplyCallback callback) {
  EnsureTemplate<IpcMainEvent>(isolate);
  IpcMainEvent* event = New(isolate);
  event->sender_.Reset(isolate, sender);
  if (frame) {
    event->has_frame_ = true;
    event->frame_id_ = frame->GetGlobalId();
    event->frame_tree_node_id_ = frame->GetFrameTreeNodeId().value();
  }
  if (callback) {
    event->reply_channel_ = gin_helper::internal::ReplyChannel::Create(
        isolate, std::move(callback));
  }
  return event;
}

// static
IpcMainEvent* IpcMainEvent::FromV8(v8::Isolate* isolate,
                                   v8::Local<v8::Value> value) {
  IpcMainEvent* event = nullptr;
  if (value.IsEmpty() || !value->IsObject() ||
      !gin::ConvertFromV8(isolate, value, &event)) {
    return nullptr;
  }
  return event;
}

void IpcMainEvent::SetPorts(v8::Isolate* isolate, v8::Local<v8::Value> ports) {
  ports_.Reset(isolate, ports);
}

// static
void IpcMainEvent::FillObjectTemplate(v8::Isolate* isolate,
                                      v8::Local<v8::ObjectTemplate> templ) {
  gin::ObjectTemplateBuilder(isolate, GetClassName(), templ)
      .SetMethod("preventDefault", &IpcMainEvent::PreventDefault)
      .SetProperty("defaultPrevented", &IpcMainEvent::GetDefaultPrevented)
      .Build();
}

// static
void IpcMainEvent::FillInstanceTemplate(v8::Isolate* isolate,
                                        v8::Local<v8::ObjectTemplate> templ) {
  // Defined in reverse: template properties enumerate last-defined first.
  Define(
      isolate, templ, "_replyChannel",
      [](v8::Local<v8::Name>, const v8::PropertyCallbackInfo<v8::Value>& info) {
        if (IpcMainEvent* self = HolderOf<IpcMainEvent>(info)) {
          info.GetReturnValue().Set(
              ReplyChannelWrapper(info.GetIsolate(), self->reply_channel()));
        }
      },
      v8::DontEnum);
  Define(
      isolate, templ, "ports",
      [](v8::Local<v8::Name>, const v8::PropertyCallbackInfo<v8::Value>& info) {
        if (IpcMainEvent* self = HolderOf<IpcMainEvent>(info);
            self && !self->ports_.IsEmpty()) {
          info.GetReturnValue().Set(self->ports_.Get(info.GetIsolate()));
        }
      });
  Define(
      isolate, templ, "frameTreeNodeId",
      [](v8::Local<v8::Name>, const v8::PropertyCallbackInfo<v8::Value>& info) {
        IpcMainEvent* self = HolderOf<IpcMainEvent>(info);
        if (self && self->has_frame_)
          info.GetReturnValue().Set(self->frame_tree_node_id_);
      });
  Define(
      isolate, templ, "processId",
      [](v8::Local<v8::Name>, const v8::PropertyCallbackInfo<v8::Value>& info) {
        IpcMainEvent* self = HolderOf<IpcMainEvent>(info);
        if (self && self->has_frame_) {
          info.GetReturnValue().Set(self->frame_id_.child_id.GetUnsafeValue());
        }
      });
  Define(
      isolate, templ, "frameId",
      [](v8::Local<v8::Name>, const v8::PropertyCallbackInfo<v8::Value>& info) {
        IpcMainEvent* self = HolderOf<IpcMainEvent>(info);
        if (self && self->has_frame_)
          info.GetReturnValue().Set(self->frame_id_.frame_routing_id);
      });
  Define(
      isolate, templ, "senderFrame",
      [](v8::Local<v8::Name>, const v8::PropertyCallbackInfo<v8::Value>& info) {
        IpcMainEvent* self = HolderOf<IpcMainEvent>(info);
        if (!self || !self->has_frame_)
          return;
        // Resolved on each read: the frame may since have gone away.
        content::RenderFrameHost* frame =
            content::RenderFrameHost::FromID(self->frame_id_);
        if (!frame) {
          electron::util::EmitWarning(
              info.GetIsolate(),
              "Frame property was accessed after it navigated or was "
              "destroyed. Avoid asynchronous tasks prior to indexing.",
              "electron");
        }
        info.GetReturnValue().Set(gin::ConvertToV8(info.GetIsolate(), frame));
      });
  Define(
      isolate, templ, "sender",
      [](v8::Local<v8::Name>, const v8::PropertyCallbackInfo<v8::Value>& info) {
        if (IpcMainEvent* self = HolderOf<IpcMainEvent>(info);
            self && !self->sender_.IsEmpty()) {
          info.GetReturnValue().Set(self->sender_.Get(info.GetIsolate()));
        }
      });
  Define(
      isolate, templ, "type",
      [](v8::Local<v8::Name>, const v8::PropertyCallbackInfo<v8::Value>& info) {
        info.GetReturnValue().Set(Symbol(info.GetIsolate(), "frame"));
      });
}

const gin::WrapperInfo* IpcMainEvent::wrapper_info() const {
  return &kWrapperInfo;
}

const char* IpcMainEvent::GetHumanReadableName() const {
  return "Electron / IpcMainEvent";
}

void IpcMainEvent::Trace(cppgc::Visitor* visitor) const {
  gin::Wrappable<IpcMainEvent>::Trace(visitor);
  visitor->Trace(sender_);
  visitor->Trace(ports_);
  visitor->Trace(reply_channel_);
}

// ---- IpcMainServiceWorkerEvent
// ------------------------------------------------

gin::WrapperInfo IpcMainServiceWorkerEvent::kWrapperInfo =
    electron::MakeWrapperInfo(electron::kElectronIpcMainServiceWorkerEvent);

IpcMainServiceWorkerEvent::IpcMainServiceWorkerEvent() = default;
IpcMainServiceWorkerEvent::~IpcMainServiceWorkerEvent() = default;

// static
IpcMainServiceWorkerEvent* IpcMainServiceWorkerEvent::New(
    v8::Isolate* isolate) {
  return cppgc::MakeGarbageCollected<IpcMainServiceWorkerEvent>(
      isolate->GetCppHeap()->GetAllocationHandle());
}

// static
IpcMainServiceWorkerEvent* IpcMainServiceWorkerEvent::Create(
    v8::Isolate* isolate,
    Session* session,
    int64_t version_id,
    int process_id,
    ReplyCallback callback) {
  EnsureTemplate<IpcMainServiceWorkerEvent>(isolate);
  IpcMainServiceWorkerEvent* event = New(isolate);
  event->session_ = session;
  event->version_id_ = version_id;
  event->process_id_ = process_id;
  if (callback) {
    event->reply_channel_ = gin_helper::internal::ReplyChannel::Create(
        isolate, std::move(callback));
  }
  return event;
}

// static
IpcMainServiceWorkerEvent* IpcMainServiceWorkerEvent::FromV8(
    v8::Isolate* isolate,
    v8::Local<v8::Value> value) {
  IpcMainServiceWorkerEvent* event = nullptr;
  if (value.IsEmpty() || !value->IsObject() ||
      !gin::ConvertFromV8(isolate, value, &event)) {
    return nullptr;
  }
  return event;
}

void IpcMainServiceWorkerEvent::SetPorts(v8::Isolate* isolate,
                                         v8::Local<v8::Value> ports) {
  ports_.Reset(isolate, ports);
}

// static
void IpcMainServiceWorkerEvent::FillObjectTemplate(
    v8::Isolate* isolate,
    v8::Local<v8::ObjectTemplate> templ) {
  gin::ObjectTemplateBuilder(isolate, GetClassName(), templ)
      .SetMethod("preventDefault", &IpcMainServiceWorkerEvent::PreventDefault)
      .SetProperty("defaultPrevented",
                   &IpcMainServiceWorkerEvent::GetDefaultPrevented)
      .Build();
}

// static
void IpcMainServiceWorkerEvent::FillInstanceTemplate(
    v8::Isolate* isolate,
    v8::Local<v8::ObjectTemplate> templ) {
  using Self = IpcMainServiceWorkerEvent;
  // Defined in reverse: template properties enumerate last-defined first.
  Define(
      isolate, templ, "_replyChannel",
      [](v8::Local<v8::Name>, const v8::PropertyCallbackInfo<v8::Value>& info) {
        if (Self* self = HolderOf<Self>(info)) {
          info.GetReturnValue().Set(
              ReplyChannelWrapper(info.GetIsolate(), self->reply_channel()));
        }
      },
      v8::DontEnum);  // event.session.serviceWorkers.getWorkerFromVersionID(event.versionId)
  Define(
      isolate, templ, "serviceWorker",
      [](v8::Local<v8::Name>, const v8::PropertyCallbackInfo<v8::Value>& info) {
        Self* self = HolderOf<Self>(info);
        if (!self || !self->session_)
          return;
        if (ServiceWorkerContext* workers =
                self->session_->ServiceWorkerContext()) {
          v8::Local<v8::Value> worker = workers->GetWorkerFromVersionID(
              info.GetIsolate(), self->version_id_);
          if (!worker.IsEmpty())
            info.GetReturnValue().Set(worker);
        }
      },
      v8::DontEnum);
  Define(
      isolate, templ, "ports",
      [](v8::Local<v8::Name>, const v8::PropertyCallbackInfo<v8::Value>& info) {
        if (Self* self = HolderOf<Self>(info);
            self && !self->ports_.IsEmpty()) {
          info.GetReturnValue().Set(self->ports_.Get(info.GetIsolate()));
        }
      });
  Define(
      isolate, templ, "session",
      [](v8::Local<v8::Name>, const v8::PropertyCallbackInfo<v8::Value>& info) {
        Self* self = HolderOf<Self>(info);
        v8::Local<v8::Object> wrapper;
        if (self && self->session_ &&
            self->session_->GetWrapper(info.GetIsolate()).ToLocal(&wrapper)) {
          info.GetReturnValue().Set(wrapper);
        }
      });
  Define(
      isolate, templ, "processId",
      [](v8::Local<v8::Name>, const v8::PropertyCallbackInfo<v8::Value>& info) {
        if (Self* self = HolderOf<Self>(info))
          info.GetReturnValue().Set(self->process_id_);
      });
  Define(
      isolate, templ, "versionId",
      [](v8::Local<v8::Name>, const v8::PropertyCallbackInfo<v8::Value>& info) {
        if (Self* self = HolderOf<Self>(info)) {
          info.GetReturnValue().Set(
              gin::ConvertToV8(info.GetIsolate(), self->version_id_));
        }
      });
  Define(
      isolate, templ, "type",
      [](v8::Local<v8::Name>, const v8::PropertyCallbackInfo<v8::Value>& info) {
        info.GetReturnValue().Set(Symbol(info.GetIsolate(), "service-worker"));
      });
}

const gin::WrapperInfo* IpcMainServiceWorkerEvent::wrapper_info() const {
  return &kWrapperInfo;
}

const char* IpcMainServiceWorkerEvent::GetHumanReadableName() const {
  return "Electron / IpcMainServiceWorkerEvent";
}

void IpcMainServiceWorkerEvent::Trace(cppgc::Visitor* visitor) const {
  gin::Wrappable<IpcMainServiceWorkerEvent>::Trace(visitor);
  visitor->Trace(session_);
  visitor->Trace(ports_);
  visitor->Trace(reply_channel_);
}

gin_helper::internal::ReplyChannel* ReplyChannelOf(v8::Isolate* isolate,
                                                   v8::Local<v8::Value> event) {
  if (IpcMainEvent* e = IpcMainEvent::FromV8(isolate, event))
    return e->reply_channel();
  if (IpcMainServiceWorkerEvent* e =
          IpcMainServiceWorkerEvent::FromV8(isolate, event)) {
    return e->reply_channel();
  }
  return nullptr;
}

}  // namespace electron::api
