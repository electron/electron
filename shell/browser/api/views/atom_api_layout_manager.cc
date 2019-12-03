// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/views/atom_api_layout_manager.h"

#include "shell/common/gin_helper/constructor.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"

namespace electron {

namespace api {

LayoutManager::LayoutManager(views::LayoutManager* layout_manager)
    : layout_manager_(layout_manager) {
  DCHECK(layout_manager_);
}

LayoutManager::~LayoutManager() {
  if (managed_by_us_)
    delete layout_manager_;
}

std::unique_ptr<views::LayoutManager> LayoutManager::TakeOver() {
  if (!managed_by_us_)  // already taken over.
    return nullptr;
  managed_by_us_ = false;
  return std::unique_ptr<views::LayoutManager>(layout_manager_);
}

// static
gin_helper::WrappableBase* LayoutManager::New(gin_helper::Arguments* args) {
  args->ThrowError("LayoutManager can not be created directly");
  return nullptr;
}

// static
void LayoutManager::BuildPrototype(v8::Isolate* isolate,
                                   v8::Local<v8::FunctionTemplate> prototype) {}

}  // namespace api

}  // namespace electron

namespace {

using electron::api::LayoutManager;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.Set("LayoutManager",
           gin_helper::CreateConstructor<LayoutManager>(
               isolate, base::BindRepeating(&LayoutManager::New)));
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(atom_browser_layout_manager, Initialize)
