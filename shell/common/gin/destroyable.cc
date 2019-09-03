// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin/destroyable.h"

#include "native_mate/wrappable_base.h"

namespace gin {

// static
void Destroyable::Destroy(mate::Arguments* args) {
  if (IsDestroyed(args))
    return;

  v8::Local<v8::Object> holder = args->GetHolder();
  delete static_cast<mate::WrappableBase*>(
      holder->GetAlignedPointerFromInternalField(0));
  holder->SetAlignedPointerInInternalField(0, nullptr);
}

// static
bool Destroyable::IsDestroyed(mate::Arguments* args) {
  v8::Local<v8::Object> holder = args->GetHolder();
  return holder->InternalFieldCount() == 0 ||
         holder->GetAlignedPointerFromInternalField(0) == nullptr;
}

}  // namespace gin
