// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/api/atom_api_crash_reporter.h"

#include "browser/crash_reporter.h"
#include "common/v8_conversions.h"
#include "vendor/node/src/node.h"
#include "vendor/node/src/node_internals.h"

namespace atom {

namespace api {

// static
v8::Handle<v8::Value> CrashReporter::SetCompanyName(const v8::Arguments &args) {
  crash_reporter::CrashReporter::SetCompanyName(FromV8Value(args[0]));
  return v8::Undefined();
}

// static
v8::Handle<v8::Value> CrashReporter::SetSubmissionURL(
    const v8::Arguments &args) {
  crash_reporter::CrashReporter::SetSubmissionURL(FromV8Value(args[0]));
  return v8::Undefined();
}

// static
v8::Handle<v8::Value> CrashReporter::SetAutoSubmit(const v8::Arguments &args) {
  crash_reporter::CrashReporter::SetAutoSubmit(FromV8Value(args[0]));
  return v8::Undefined();
}

// static
void CrashReporter::Initialize(v8::Handle<v8::Object> target) {
  node::SetMethod(target, "setCompanyName", SetCompanyName);
  node::SetMethod(target, "setSubmissionUrl", SetSubmissionURL);
  node::SetMethod(target, "setAutoSubmit", SetAutoSubmit);
}

}  // namespace api

}  // namespace atom

NODE_MODULE(atom_browser_crash_reporter, atom::api::CrashReporter::Initialize)
