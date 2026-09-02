// Copyright (c) 2020 Samuel Maddock <sam@samuelmaddock.com>.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_converters/frame_converter.h"

#include <cstdint>

#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "shell/browser/api/electron_api_web_frame_main.h"
#include "shell/common/gin_helper/accessor.h"
#include "shell/common/gin_helper/handle.h"
#include "shell/common/node_util.h"
#include "v8/include/v8-primitive.h"

namespace gin {

// static
v8::Local<v8::Value> Converter<content::FrameTreeNodeId>::ToV8(
    v8::Isolate* isolate,
    const content::FrameTreeNodeId& val) {
  return v8::Number::New(isolate, val.value());
}

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

  // The two ids are packed into a BigInt rather than stored in the internal
  // fields of an object because building such an object requires an
  // ObjectTemplate and caching via gin::PerContextData which is not
  // necessary here as the token never reaches JS, it is only the payload
  // of the native data property installed by gin_helper::Dictionary::SetGetter.
  const uint64_t token = (static_cast<uint64_t>(static_cast<uint32_t>(
                              rfh->GetProcess()->GetID().GetUnsafeValue()))
                          << 32) |
                         static_cast<uint32_t>(rfh->GetRoutingID());

  return v8::BigInt::NewFromUnsigned(isolate, token);
}

// static
bool Converter<gin_helper::AccessorValue<content::RenderFrameHost*>>::FromV8(
    v8::Isolate* isolate,
    v8::Local<v8::Value> val,
    gin_helper::AccessorValue<content::RenderFrameHost*>* out) {
  if (!val->IsBigInt())
    return false;

  bool lossless = false;
  const uint64_t token = val.As<v8::BigInt>()->Uint64Value(&lossless);
  if (!lossless)
    return false;

  // Both ids are signed (e.g. MSG_ROUTING_NONE), so go back
  // through uint32_t to undo the packing before reinterpreting the sign bit.
  const int process_id =
      static_cast<int32_t>(static_cast<uint32_t>(token >> 32));
  const int routing_id = static_cast<int32_t>(static_cast<uint32_t>(token));

  auto* rfh = content::RenderFrameHost::FromID(process_id, routing_id);

  if (!rfh) {
    // Lazily evaluated property accessed after RFH has been destroyed.
    // Continue to return nullptr, but emit warning to inform developers
    // what occurred.
    electron::util::EmitWarning(
        isolate,
        "Frame property was accessed after it navigated or was destroyed. "
        "Avoid asynchronous tasks prior to indexing.",
        "electron");
  }

  out->Value = rfh;
  return true;
}

}  // namespace gin
