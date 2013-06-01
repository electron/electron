// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/api/atom_api_crash_reporter.h"

#import <Quincy/BWQuincyManager.h>

#include "base/strings/sys_string_conversions.h"
#include "vendor/node/src/node.h"
#include "vendor/node/src/node_internals.h"

namespace atom {

namespace api {

namespace {

// Converts a V8 value to a string16.
string16 V8ValueToUTF16(v8::Handle<v8::Value> value) {
  v8::String::Value s(value);
  return string16(reinterpret_cast<const char16*>(*s), s.length());
}

}  // namespace

// static
v8::Handle<v8::Value> CrashReporter::SetCompanyName(const v8::Arguments &args) {
  BWQuincyManager *manager = [BWQuincyManager sharedQuincyManager];
  string16 str(V8ValueToUTF16(args[0]));
  [manager setCompanyName:base::SysUTF16ToNSString(str)];
  return v8::Undefined();
}

// static
v8::Handle<v8::Value> CrashReporter::SetSubmissionURL(
    const v8::Arguments &args) {
  BWQuincyManager *manager = [BWQuincyManager sharedQuincyManager];
  string16 str(V8ValueToUTF16(args[0]));
  [manager setSubmissionURL:base::SysUTF16ToNSString(str)];
  return v8::Undefined();
}

// static
v8::Handle<v8::Value> CrashReporter::SetAutoSubmit(const v8::Arguments &args) {
  BWQuincyManager *manager = [BWQuincyManager sharedQuincyManager];
  [manager setAutoSubmitCrashReport:args[0]->BooleanValue()];
  return v8::Undefined();
}

// static
void CrashReporter::Initialize(v8::Handle<v8::Object> target) {
  node::SetMethod(target, "setCompanyName", SetCompanyName);
  node::SetMethod(target, "setSubmissionURL", SetSubmissionURL);
  node::SetMethod(target, "setAutoSubmit", SetAutoSubmit);
}

}  // namespace api

}  // namespace atom

NODE_MODULE(atom_browser_crash_reporter, atom::api::CrashReporter::Initialize)
