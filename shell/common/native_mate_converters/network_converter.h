// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_NATIVE_MATE_CONVERTERS_NETWORK_CONVERTER_H_
#define ATOM_COMMON_NATIVE_MATE_CONVERTERS_NETWORK_CONVERTER_H_

#include "base/memory/scoped_refptr.h"
#include "native_mate/converter.h"

namespace network {
class ResourceRequestBody;
}

namespace mate {

template <>
struct Converter<scoped_refptr<network::ResourceRequestBody>> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const scoped_refptr<network::ResourceRequestBody>& val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     scoped_refptr<network::ResourceRequestBody>* out);
};

}  // namespace mate

#endif  // ATOM_COMMON_NATIVE_MATE_CONVERTERS_NETWORK_CONVERTER_H_
