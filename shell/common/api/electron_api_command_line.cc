// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/api/electron_api_command_line.h"

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/strings/string_util.h"
#include "net/base/switches.h"
#include "shell/common/gin_converters/base_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"

namespace {

// The switch and value arguments are coerced to strings.
bool GetNextString(gin::Arguments* args, std::string* out) {
  v8::Local<v8::Value> value;
  v8::Local<v8::String> str;
  if (!args->GetNext(&value) || value->IsUndefined() ||
      !value->ToString(args->isolate()->GetCurrentContext()).ToLocal(&str)) {
    return false;
  }
  *out = gin::V8ToString(args->isolate(), str);
  return true;
}

// Reads the switch name; throws and returns false when it is missing.
bool GetSwitchName(gin::Arguments* args, std::string* name) {
  if (!GetNextString(args, name)) {
    args->ThrowError();
    return false;
  }
  *name = base::ToLowerASCII(*name);
  return true;
}

bool HasSwitch(gin::Arguments* args) {
  std::string name;
  return GetSwitchName(args, &name) &&
         base::CommandLine::ForCurrentProcess()->HasSwitch(name);
}

base::CommandLine::StringType GetSwitchValue(gin::Arguments* args) {
  std::string name;
  if (!GetSwitchName(args, &name))
    return {};
  return base::CommandLine::ForCurrentProcess()->GetSwitchValueNative(name);
}

void AppendSwitch(gin::Arguments* args) {
  std::string name;
  if (!GetSwitchName(args, &name))
    return;
  auto* command_line = base::CommandLine::ForCurrentProcess();
  std::string value;
  if (!GetNextString(args, &value)) {
    command_line->AppendSwitch(name);
  } else if (base::EndsWith(name, "-path") ||
             name == net::switches::kLogNetLog) {
    command_line->AppendSwitchPath(name, base::FilePath::FromUTF8Unsafe(value));
  } else {
    command_line->AppendSwitchUTF8(name, value);
  }
}

void RemoveSwitch(gin::Arguments* args) {
  std::string name;
  if (GetSwitchName(args, &name))
    base::CommandLine::ForCurrentProcess()->RemoveSwitch(name);
}

void AppendArg(gin::Arguments* args) {
  std::string arg;
  if (!GetNextString(args, &arg)) {
    args->ThrowError();
    return;
  }
  base::CommandLine::ForCurrentProcess()->AppendArg(arg);
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  gin_helper::Dictionary dict{v8::Isolate::GetCurrent(), exports};
  electron::api::FillCommandLine(&dict);
}

}  // namespace

namespace electron::api {

void FillCommandLine(gin_helper::Dictionary* dict) {
  dict->SetMethod("hasSwitch", &HasSwitch);
  dict->SetMethod("getSwitchValue", &GetSwitchValue);
  dict->SetMethod("appendSwitch", &AppendSwitch);
  dict->SetMethod("removeSwitch", &RemoveSwitch);
  dict->SetMethod("appendArgument", &AppendArg);
}

}  // namespace electron::api

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_common_command_line, Initialize)
