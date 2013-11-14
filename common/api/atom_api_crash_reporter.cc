// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/api/atom_api_crash_reporter.h"

#include "common/crash_reporter/crash_reporter.h"
#include "common/v8_conversions.h"
#include "vendor/node/src/node.h"
#include "vendor/node/src/node_internals.h"

namespace atom {

namespace api {

// static
v8::Handle<v8::Value> CrashReporter::Start(const v8::Arguments& args) {
  std::string product_name, company_name, submit_url;
  bool auto_submit, skip_system;
  if (!FromV8Arguments(args, &product_name, &company_name, &submit_url,
                       &auto_submit, &skip_system))
    return node::ThrowTypeError("Bad argument");

  crash_reporter::CrashReporter::GetInstance()->Start(
      product_name, company_name, submit_url, auto_submit, skip_system);

  return v8::Undefined();
}

// static
void CrashReporter::Initialize(v8::Handle<v8::Object> target) {
  node::SetMethod(target, "start", Start);
}

}  // namespace api

}  // namespace atom

NODE_MODULE(atom_common_crash_reporter, atom::api::CrashReporter::Initialize)
