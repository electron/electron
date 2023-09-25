// Copyright (c) 2020 Samuel Maddock <sam@samuelmaddock.com>.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_converters/frame_converter.h"

#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "shell/browser/api/electron_api_web_frame_main.h"
#include "shell/common/gin_helper/accessor.h"

namespace gin {

namespace {

v8::Persistent<v8::ObjectTemplate> rfh_templ;

}  // namespace

// static
v8::Local<v8::Value> Converter<content::RenderFrameHost*>::ToV8(
    v8::Isolate* isolate,
    content::RenderFrameHost* val) {
  if (!val)
    return v8::Null(isolate);
  return electron::api::WebFrameMain::From(isolate, val).ToV8();
}

// static
bool Converter<content::RenderFrameHost*>::FromV8(
    v8::Isolate* isolate,
    v8::Local<v8::Value> val,
    content::RenderFrameHost** out) {
  electron::api::WebFrameMain* web_frame_main = nullptr;
  if (!ConvertFromV8(isolate, val, &web_frame_main))
    return false;
  *out = web_frame_main->render_frame_host();

  return true;
}

// static
v8::Local<v8::Value>
Converter<gin_helper::AccessorValue<content::RenderFrameHost*>>::ToV8(
    v8::Isolate* isolate,
    gin_helper::AccessorValue<content::RenderFrameHost*> val) {
  content::RenderFrameHost* rfh = val.Value;
  if (!rfh)
    return v8::Null(isolate);

  const int process_id = rfh->GetProcess()->GetID();
  const int routing_id = rfh->GetRoutingID();

  if (rfh_templ.IsEmpty()) {
    v8::EscapableHandleScope inner(isolate);
    v8::Local<v8::ObjectTemplate> local = v8::ObjectTemplate::New(isolate);
    local->SetInternalFieldCount(2);
    rfh_templ.Reset(isolate, inner.Escape(local));
  }

  v8::Local<v8::Object> rfh_obj =
      v8::Local<v8::ObjectTemplate>::New(isolate, rfh_templ)
          ->NewInstance(isolate->GetCurrentContext())
          .ToLocalChecked();

  rfh_obj->SetInternalField(0, v8::Number::New(isolate, process_id));
  rfh_obj->SetInternalField(1, v8::Number::New(isolate, routing_id));

  return rfh_obj;
}

// static
bool Converter<gin_helper::AccessorValue<content::RenderFrameHost*>>::FromV8(
    v8::Isolate* isolate,
    v8::Local<v8::Value> val,
    gin_helper::AccessorValue<content::RenderFrameHost*>* out) {
  v8::Local<v8::Object> rfh_obj;
  if (!ConvertFromV8(isolate, val, &rfh_obj))
    return false;

  if (rfh_obj->InternalFieldCount() != 2)
    return false;

  v8::Local<v8::Value> process_id_wrapper =
      rfh_obj->GetInternalField(0).As<v8::Value>();
  v8::Local<v8::Value> routing_id_wrapper =
      rfh_obj->GetInternalField(1).As<v8::Value>();

  if (process_id_wrapper.IsEmpty() || !process_id_wrapper->IsNumber() ||
      routing_id_wrapper.IsEmpty() || !routing_id_wrapper->IsNumber())
    return false;

  const int process_id = process_id_wrapper.As<v8::Number>()->Value();
  const int routing_id = routing_id_wrapper.As<v8::Number>()->Value();

  auto* rfh = content::RenderFrameHost::FromID(process_id, routing_id);
  if (!rfh)
    return false;

  out->Value = rfh;
  return true;
}

}  // namespace gin
