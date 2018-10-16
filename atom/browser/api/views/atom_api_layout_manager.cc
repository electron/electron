// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/views/atom_api_layout_manager.h"

#include "atom/common/api/constructor.h"
#include "native_mate/dictionary.h"

#include "atom/common/node_includes.h"

namespace atom {

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
mate::WrappableBase* LayoutManager::New(mate::Arguments* args) {
  args->ThrowError("LayoutManager can not be created directly");
  return nullptr;
}

// static
void LayoutManager::BuildPrototype(v8::Isolate* isolate,
                                   v8::Local<v8::FunctionTemplate> prototype) {}

}  // namespace api

}  // namespace atom

namespace {

using atom::api::LayoutManager;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.Set("LayoutManager", mate::CreateConstructor<LayoutManager>(
                                isolate, base::Bind(&LayoutManager::New)));
}

}  // namespace

NODE_BUILTIN_MODULE_CONTEXT_AWARE(atom_browser_layout_manager, Initialize)
