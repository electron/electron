// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/common/api/atom_api_crash_reporter.h"

#include <map>
#include <string>

#include "atom/common/crash_reporter/crash_reporter.h"
#include "atom/common/v8/native_type_conversions.h"

#include "atom/common/v8/node_common.h"

namespace atom {

namespace api {

// static
void CrashReporter::Start(const v8::FunctionCallbackInfo<v8::Value>& args) {
  std::string product_name, company_name, submit_url;
  bool auto_submit, skip_system;
  std::map<std::string, std::string> dict;
  if (!FromV8Arguments(args, &product_name, &company_name, &submit_url,
                       &auto_submit, &skip_system, &dict))
    return node::ThrowTypeError("Bad argument");

  crash_reporter::CrashReporter::GetInstance()->Start(
      product_name, company_name, submit_url, auto_submit, skip_system, dict);
}

// static
void CrashReporter::Initialize(v8::Handle<v8::Object> target) {
  NODE_SET_METHOD(target, "start", Start);
}

}  // namespace api

}  // namespace atom

NODE_MODULE(atom_common_crash_reporter, atom::api::CrashReporter::Initialize)
