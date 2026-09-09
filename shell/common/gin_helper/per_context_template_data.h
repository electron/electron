// Copyright (c) 2026 Microsoft Corporation.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_PER_CONTEXT_TEMPLATE_DATA_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_PER_CONTEXT_TEMPLATE_DATA_H_

#include <memory>
#include <utility>

#include "base/supports_user_data.h"
#include "gin/per_context_data.h"
#include "v8/include/v8-persistent-handle.h"
#include "v8/include/v8-template.h"

namespace gin_helper {

// Templates containing cppgc callback wrappers retain their creation context.
// Release these roots when PerContextData detaches, rather than at isolate
// exit.
class PerContextTemplateData : public base::SupportsUserData::Data {
 public:
  PerContextTemplateData();
  ~PerContextTemplateData() override;

  static PerContextTemplateData* From(v8::Local<v8::Context> context,
                                      const void* key) {
    auto* data = gin::PerContextData::From(context);
    if (!data)
      return nullptr;
    auto* templates =
        static_cast<PerContextTemplateData*>(data->GetUserData(key));
    if (!templates) {
      auto owned_templates = std::make_unique<PerContextTemplateData>();
      templates = owned_templates.get();
      data->SetUserData(key, std::move(owned_templates));
    }
    return templates;
  }

  v8::Global<v8::FunctionTemplate> function_template;
  v8::Global<v8::ObjectTemplate> object_template;
};

}  // namespace gin_helper

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_PER_CONTEXT_TEMPLATE_DATA_H_
