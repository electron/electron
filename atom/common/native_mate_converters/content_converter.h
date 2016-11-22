// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_NATIVE_MATE_CONVERTERS_CONTENT_CONVERTER_H_
#define ATOM_COMMON_NATIVE_MATE_CONVERTERS_CONTENT_CONVERTER_H_

#include <utility>

#include "content/public/browser/permission_type.h"
#include "content/public/common/menu_item.h"
#include "content/public/common/stop_find_action.h"
#include "native_mate/converter.h"
#include "third_party/WebKit/public/platform/modules/permissions/permission_status.mojom.h"

namespace content {
struct ContextMenuParams;
class ResourceRequestBodyImpl;
class WebContents;
}

using ContextMenuParamsWithWebContents =
    std::pair<content::ContextMenuParams, content::WebContents*>;

namespace mate {

template<>
struct Converter<content::MenuItem::Type> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const content::MenuItem::Type& val);
};

template<>
struct Converter<ContextMenuParamsWithWebContents> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const ContextMenuParamsWithWebContents& val);
};

template<>
struct Converter<blink::mojom::PermissionStatus> {
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     blink::mojom::PermissionStatus* out);
};

template<>
struct Converter<content::PermissionType> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const content::PermissionType& val);
};

template<>
struct Converter<scoped_refptr<content::ResourceRequestBodyImpl>> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const scoped_refptr<content::ResourceRequestBodyImpl>& val);
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     scoped_refptr<content::ResourceRequestBodyImpl>* out);
};

template<>
struct Converter<content::StopFindAction> {
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     content::StopFindAction* out);
};

template<>
struct Converter<content::WebContents*> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   content::WebContents* val);
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     content::WebContents** out);
};

}  // namespace mate

#endif  // ATOM_COMMON_NATIVE_MATE_CONVERTERS_CONTENT_CONVERTER_H_
