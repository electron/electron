// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/strings/string_util.h"
#include "services/network/public/cpp/network_switches.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"

namespace {

bool HasSwitch(const std::string& name) {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(name.c_str());
}

base::CommandLine::StringType GetSwitchValue(const std::string& name) {
  return base::CommandLine::ForCurrentProcess()->GetSwitchValueNative(
      name.c_str());
}

void AppendSwitch(const std::string& switch_string,
                  gin_helper::Arguments* args) {
  auto* command_line = base::CommandLine::ForCurrentProcess();

  if (base::EndsWith(switch_string, "-path",
                     base::CompareCase::INSENSITIVE_ASCII) ||
      switch_string == network::switches::kLogNetLog) {
    base::FilePath path;
    args->GetNext(&path);
    command_line->AppendSwitchPath(switch_string, path);
    return;
  }

  base::CommandLine::StringType value;
  if (args->GetNext(&value))
    command_line->AppendSwitchNative(switch_string, value);
  else
    command_line->AppendSwitch(switch_string);
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  auto* command_line = base::CommandLine::ForCurrentProcess();
  gin_helper::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("hasSwitch", &HasSwitch);
  dict.SetMethod("getSwitchValue", &GetSwitchValue);
  dict.SetMethod("appendSwitch", &AppendSwitch);
  dict.SetMethod("appendArgument",
                 base::BindRepeating(&base::CommandLine::AppendArg,
                                     base::Unretained(command_line)));
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(atom_common_command_line, Initialize)
