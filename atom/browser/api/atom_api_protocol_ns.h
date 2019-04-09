// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_PROTOCOL_NS_H_
#define ATOM_BROWSER_API_ATOM_API_PROTOCOL_NS_H_

#include "atom/browser/api/trackable_object.h"
#include "native_mate/handle.h"

namespace atom {

class AtomBrowserContext;

namespace api {

// Protocol implementation based on network services.
class ProtocolNS : public mate::TrackableObject<ProtocolNS> {
 public:
  static mate::Handle<ProtocolNS> Create(v8::Isolate* isolate,
                                         AtomBrowserContext* browser_context);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

 private:
  ProtocolNS(v8::Isolate* isolate, AtomBrowserContext* browser_context);
  ~ProtocolNS() override;
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_PROTOCOL_NS_H_
